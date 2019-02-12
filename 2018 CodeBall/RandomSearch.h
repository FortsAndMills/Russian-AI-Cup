#pragma once
#include "Simulation.h"

class RandomSearch
{
public:
    Simulation * world;    // for each robot, we have BATCH_SIZE simulations.
    ActionPlan *** plans;  // for each simulation and robot we have actions planned for HORIZON ticks

    double * quality;      // reward for each simulation
    int * chosen_path;     // for every robot, we store id of simulation from his personal batch with best quality
    int * recompute_times; // remember tick when batch for this robot was recomputed

    bool require_init = true;  // is first tick

    RandomSearch(const model::Game& game)
    {
        // first BATCH_SIZE worlds are for robot with id 0, then for id 1, etc.
        world = new Simulation[2*NUMBER_OF_ROBOTS*BATCH_SIZE];
        for (int b = 0; b < 2*NUMBER_OF_ROBOTS*BATCH_SIZE; ++b)
        {
            for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
            {
                world[b].robots[game.robots[r].id - 1].is_teammate = game.robots[r].is_teammate;
            }
        }

        quality = new double[2*NUMBER_OF_ROBOTS*BATCH_SIZE];
        chosen_path = new int[2 * NUMBER_OF_ROBOTS];
        recompute_times = new int[2 * NUMBER_OF_ROBOTS];

        // After initialization, all actions are empty.
        plans = new ActionPlan**[2 * NUMBER_OF_ROBOTS*BATCH_SIZE];
        for (int b = 0; b < 2 * NUMBER_OF_ROBOTS*BATCH_SIZE; ++b)
        {
            plans[b] = new ActionPlan*[HORIZON];
            for (int t = 0; t < HORIZON; ++t)            
            {
                plans[b][t] = new ActionPlan[2*NUMBER_OF_ROBOTS];
            }
        }
    }

    void replan(const model::Game& game, int time)
    {
        // create new generation
        new_generation(time);

        // simulating
        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            if (recompute_times[r] == time)  // if is set to be recomputed now
            {
                double best_quality = -100000000;
                for (int b = BATCH_SIZE * r; b < BATCH_SIZE * (r + 1); ++b)
                {
                    // initialize world with current game situation
                    world[b].set(game);
                    quality[b] = 0;

                    // simulation is more precise if jumps and collisions are computed by microticks
                    // yet between events (collisions) we may calculate many microticks at once
                    // we allow HORIZON + OPTIMIZE_CHARGES steps at most to prevent TL.
                    int optimize_charges = OPTIMIZE_CHARGES;
                    for (int t = 0; t < HORIZON; ++t)
                    {
                        // simulating one tick
                        world[b].tick(plans[b][t], 1, optimize_charges);

                        // estimating reward
                        quality[b] += everytick_reward(world[b], !world[b].robots[r].is_teammate) / HORIZON;
                    }

                    // selecting best path
                    if (quality[b] > best_quality)
                    {
                        chosen_path[r] = b;
                        best_quality = quality[b];
                    }
                }
            }
        }
    }

    void execute(const model::Robot& me, model::Action& action, int time) const
    {
        // set best action for robot me
        ArenaObject hero;
        hero.set(me);  // required to geet touch_normal

        plans[chosen_path[me.id - 1]][time - recompute_times[me.id - 1]][me.id - 1].reparametrize(hero.touch_normal);
        plans[chosen_path[me.id - 1]][time - recompute_times[me.id - 1]][me.id - 1].set_action(action);
    }

private:
    void new_generation(int time)  // creating new generation
    {
        if (require_init) // first tick: resample all trajectories
        {
            cout << "FULL_REGENERATION" << endl;
            for (int b = 0; b < 2 * NUMBER_OF_ROBOTS*BATCH_SIZE; ++b)
            {
                for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
                {
                    if (r == b / BATCH_SIZE)  // if this batch is intended for this robot, create new random action plan
                        resample_trajectory(b, r);
                    else
                    {
                        // otherwise plan is empty
                        for (int t = 0; t < HORIZON; ++t)
                        {
                            plans[b][t][r].empty();
                        }
                    }
                }
            }

            // we recompute our robots at first tick;
            // then we recompute enemy robots at first + RECOMPUTE_TICKS / 2 tick
            // then again ours.
            for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
                recompute_times[r] = world[0].robots[r].is_teammate ? time : time + RECOMPUTE_TICK / 2;
            
            require_init = false;
        }
        else
        {
            // updating all other robots action plans by just copying their current best plans
            set_opponent_strategies(time);

            for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
            {
                if (time - recompute_times[r] >= RECOMPUTE_TICK)  // if it is time to update
                {
                    // preparing selection
                    double sampling_probabilities[BATCH_SIZE];
                    for (int b = 0; b < BATCH_SIZE; ++b)
                        sampling_probabilities[b] = quality[b + r*BATCH_SIZE];

                    // getting SELECTION_QUANTILE-th quantile
                    sort(quality + r*BATCH_SIZE, quality + (r + 1)*BATCH_SIZE);
                    double threshold = quality[r*BATCH_SIZE + int(SELECTION_QUANTILE * BATCH_SIZE)];

                    for (int b = 0; b < BATCH_SIZE; ++b)
                    {
                        // probability of being selected is proportional to benefit compared to quantile
                        if (sampling_probabilities[b] > threshold)
                            sampling_probabilities[b] -= threshold;
                        else // probability is zero if trajectory is not in top-N
                            sampling_probabilities[b] = 0;
                    }

                    // selecting best trajectories
                    std::discrete_distribution<int> sampler(sampling_probabilities, sampling_probabilities + BATCH_SIZE);

                    int selected = SELECTED * BATCH_SIZE;
                    for (int b = 0; b < BATCH_SIZE; ++b)
                    {
                        // if this trajectory is not in top-N, copy from selected
                        if (sampling_probabilities[b] == 0)
                        {
                            if (selected > 0)
                            {
                                int id = sampler(random_generator);
                                copy_and_modify(b + r*BATCH_SIZE, r, id + r*BATCH_SIZE, time);
                                selected -= 1;
                            }
                            else
                                resample_trajectory(b + r*BATCH_SIZE, r);
                        }
                    }

                    // resample top-N trajectories
                    for (int b = 0; b < BATCH_SIZE; ++b)
                    {
                        if (sampling_probabilities[b] > 0)
                        {
                            resample_trajectory(b + r*BATCH_SIZE, r);
                        }
                    }
                }
            }

            // setting time of recomputation
            for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
                if (time - recompute_times[r] >= RECOMPUTE_TICK)
                    recompute_times[r] = time;
        }
    }

    void set_opponent_strategies(int time)
    {
        for (int main_r = 0; main_r < 2 * NUMBER_OF_ROBOTS; ++main_r)
        {  // if it is time to update this robot
            if (time - recompute_times[main_r] >= RECOMPUTE_TICK)
            {  // for every world in his batch
                for (int b = main_r*BATCH_SIZE; b < (main_r + 1)*BATCH_SIZE; ++b)
                {
                    for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
                    {  // for every other robot
                        if (r != main_r)
                        {
                            // just copy his best action plan taking into account time when it was created
                            int shift = time - recompute_times[r];
                            for (int t = 0; t < HORIZON - shift; ++t)
                            {
                                plans[b][t][r] = plans[chosen_path[r]][t + shift][r];
                            }

                            // the "tail" of action plan is sampled
                            for (int t = HORIZON - shift; t < HORIZON; ++t)
                            {
                                plans[b][t][r].continue_trajectory(plans[b][t - 1][r]);
                            }
                        }
                    }
                }
            }
        }
    }
        
    void resample_trajectory(int b, int r)
    {   // complete resampling of trajectory
        plans[b][0][r].sample();

        for (int t = 1; t < HORIZON; ++t)
        {
            plans[b][t][r].continue_trajectory(plans[b][t - 1][r]);
        }
    }

    void copy_and_modify(int b, int r, int source, int time)
    {   // copying other trajectory (with index source) and mutating it
        // considering time when source trajectory was created
        int shift = time - recompute_times[r];

        // copying
        plans[b][0][r].copy(plans[source][shift][r]);
        for (int t = 1; t < HORIZON - shift; ++t)
        {
            plans[b][t][r].continue_copy(plans[source][t + shift][r], plans[b][t - 1][r]);
        }

        // jump shifts mutation
        for (int t = 1; t < HORIZON - shift; ++t)
        {
            // jump start found
            if (plans[source][t + shift][r].jump_activated &&
                !plans[source][t - 1 + shift][r].jump_activated)
            {
                if (coin(MUTATION_JUMP_SHIFT_PROBABILITY))
                {
                    int power = Power(MUTATION_JUMP_SHIFT_POWER);
                    if (power < 0)  // jump starts earlier
                    {
                        for (int tt = max(0, t + power); tt < t; ++tt)
                            plans[b][tt][r].jump_activated = true;
                    }
                    else            // jump starts later
                    {
                        for (int tt = t; tt < min(t + power, HORIZON - shift); ++tt)
                            plans[b][tt][r].jump_activated = false;
                        t += power;
                    }
                }
            }
            else if (plans[source][t - 1 + shift][r].jump_activated &&
                !plans[source][t + shift][r].jump_activated)
            {  // jump end found
                if (coin(MUTATION_JUMP_SHIFT_PROBABILITY))
                {
                    int power = Power(MUTATION_JUMP_SHIFT_POWER);
                    if (power < 0)  // jump ends earlier
                    {
                        for (int tt = max(0, t + power); tt < t; ++tt)
                            plans[b][tt][r].jump_activated = false;
                    }
                    else            // jump ends later
                    {
                        for (int tt = t; tt < min(t + power, HORIZON - shift); ++tt)
                            plans[b][tt][r].jump_activated = true;
                        t += power;
                    }
                }
            }
        }

        // transforming random process representation to action plan;
        // just in order not to do that after each mutation
        for (int t = 0; t < HORIZON - shift; ++t)
        {
            plans[b][t][r].action_from_trajectory();
        }

        // continuing trajectories as in usual generation procedure
        for (int t = HORIZON - shift; t < HORIZON; ++t)
        {
            plans[b][t][r].continue_trajectory(plans[b][t - 1][r]);
        }
    }

    double everytick_reward(const Simulation & world, bool enemy)
    {
        // KEY FUNCTION (as you can guess...)
        // enemy=True means we estimate reward for enemy robots.
        int team_sign = enemy ? -1 : 1;

        // PROTECTION PENALTY -------------------------------------------------------------------
        // Defense. This is applied only for my robots.
        // If the ball is near enemy robot, he might strike and we need to be close to gates.

        double protection_penalty = 0;
        for (int e = 0; e < 2 * NUMBER_OF_ROBOTS; ++e)
        {
            // Enemy robot may strike if his z cordinate is bigger than ball's.
            if (world.robots[e].is_teammate == enemy &&
                team_sign * (world.robots[e].position.z - world.ball.position.z) > 0)
            {
                // coordinates of where line, joining ball and striker, intersects goal plane.
                double z = -team_sign * DEPTH / 2;
                double y = 1;
                double x = world.ball.position.x + (world.robots[e].position.x - world.ball.position.x) / (world.robots[e].position.z - world.ball.position.z) * (z - world.ball.position.z);

                // farer enemy from ball, less the penalty must be!
                double enemy_dist = (world.robots[e].position - world.ball.position).length;
                
                // if x is not inside goal gates, we will multiply penalty on a coeff less than 1
                // if x is really far from goal gates, this coeff will be 0
                double far_away_c = 1;
                if (x > GOAL_WIDTH / 2)
                {
                    far_away_c = max(0.0, 1 - ATTACK_GOAL_MARGIN_COEFF*(x - GOAL_WIDTH / 2)*(x - GOAL_WIDTH / 2));
                    x = GOAL_WIDTH / 2;
                }
                else if (x < -GOAL_WIDTH / 2)
                {
                    far_away_c = max(0.0, 1 - ATTACK_GOAL_MARGIN_COEFF*(x + GOAL_WIDTH / 2)*(x + GOAL_WIDTH / 2));
                    x = -GOAL_WIDTH / 2;
                }                

                // so, we want to protect the gates at point (x, y, z)
                Vec3D target_pos = Vec3D(x, y, z);
                // we will find robot, who is the closest to this point
                double min_protection = 100;

                for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
                {
                    // robots with touch=false are NOT considered defenders!
                    if (world.robots[r].is_teammate != enemy && world.robots[r].touch)
                    {
                        double pr_dist = (target_pos - world.robots[r].position).length;
                        min_protection = min(min_protection, pr_dist);
                    }
                }

                // resulting formula
                protection_penalty += min_protection*min_protection*far_away_c / (enemy_dist*enemy_dist);
            }
        }

        // POTENTIALS ------------------------------------------------------------------------
        // Closer a robot is to ball, better for him
        // We will calculate some kind of "influence" of robot on ball and add it to ball velocity.
        
        double ball_potential = world.ball.velocity.z;

        // Each team's max influence will be considered with big coeffecient
        // Thus we attempt to favor distribution across the field to be close to ball at every moment
        double my_max_influence = 0;
        double enemy_max_influence = 0;
                
        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)  // for every robot
        {
            Vec3D force = (world.ball.position - world.robots[r].position);
            double dist = force.length;  // distance to ball
            
            // normalize direction to ball
            force.normalize();
            // let call power a projection of velocity to line, joining ball and robot
            double power = world.robots[r].velocity ^ force;
            
            double influence = 0;
            if (world.robots[r].is_teammate == (force.z > 0))
            {   // if robot is to the right side of ball: no bonus for velocity
                // bonus for velocity: velocity / dist
                influence += force.z * (max(0.0, power) / dist);
            }

            double direction = force.z + (world.robots[r].is_teammate ? 1 : -1);  // TODO projection on direction to goals
            if (world.robots[r].touch)
            {
                // bonus for being close to ball: 100 / dist^dist
                // multiplied at touch_normal.y as it limits our maneuverability.
                // for closeness, we don't care if we are to the right side of the ball
                influence += direction * (POTENTIAL_COEFF * world.robots[r].touch_normal.y / (dist*dist));
            }

            // adding influence to ball_potential with small coefficient
            ball_potential += influence / NOT_MAX_INFLUENCE_COEFF;
            
            // influence for me is always >= 0; for enemy is always <= 0
            if (influence > my_max_influence)
                my_max_influence = influence;
            if (influence < enemy_max_influence)
                enemy_max_influence = influence;
        }
        ball_potential += my_max_influence + enemy_max_influence;
        
        // MAIN REWARD --------------------------------------------------------------------------
        // the higher is ball velocity better for me; lower is ball velocity better for enemy
        // that's why influence tweaked ball velocity: it is a good reward
        // yet the closer we are to the gates the more important this reward becomes
        // so we are going to multiply ball velocity on a coeffecient of closeness to gates

        // ball.position.z is almost a distance to the goals;
        // yet we need to take into account sides and roof.
        double dist_x = 0;
        if (world.ball.position.x < -GOAL_WIDTH / 2)
            dist_x = -GOAL_WIDTH / 2 - world.ball.position.x;
        else if (world.ball.position.x > GOAL_WIDTH / 2)
            dist_x = world.ball.position.x - GOAL_WIDTH / 2;

        double dist_y = 0;
        if (world.ball.position.y > GOAL_HEIGHT)
            dist_y = world.ball.position.y - GOAL_HEIGHT;

        double ball_speed = ball_potential;
        if (ball_speed > 0)  // ball flies to enemy gates
        {
            double dist_z = DEPTH / 2 - world.ball.position.z;
            double dist = Vec3D(dist_x, dist_y, dist_z).length;

            // if ball coordiante is 0, coeff is 1
            // if ball is close to my gates and flies to enemy's, coeff is close to 0
            // if ball is close to enemy gates and flies into it, coeff if really high
            ball_speed *= exp(BALL_SPEED_R * (DEPTH / 2 - dist));
        }
        else                 // ball flies to my gates
        {
            double dist_z = world.ball.position.z - DEPTH / 2;
            double dist = Vec3D(dist_x, dist_y, dist_z).length;

            // -||-
            ball_speed *= exp(-BALL_SPEED_R * (-DEPTH / 2 + dist));
        }

        // NITRO REWARD --------------------------------------------------------------------------
        // just sum of nitro with small coeff 1/40
        double nitro = 0;
        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            if (world.robots[r].is_teammate != enemy)
                nitro += world.robots[r].nitro_amount * NITRO_COEFF;
            else
                nitro -= world.robots[r].nitro_amount * NITRO_COEFF;
        }

        // ENEMY_HIT_BALL_AND_I_AM_FLYING --------------------------------------------------------
        double ehbaiaf = 0;
        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            if (world.robots[r].enemy_hit_ball_and_I_am_flying)
            {
                if (world.robots[r].is_teammate == enemy)
                    ehbaiaf += EHBAIAF_PENALTY;
                else
                    ehbaiaf -= EHBAIAF_PENALTY;
            }
        }


        // ALL TOGETHER --------------------------------------------------------------------------
        if (enemy)
        {
            return (-ball_speed
                + nitro + ehbaiaf
                - 1000000 * (world.my_goals > 0)
                + 1000000 * (world.enemy_goals > 0)
                );
        }

        return (ball_speed - protection_penalty / NUMBER_OF_ROBOTS
            + nitro + ehbaiaf
            + 1000000 * (world.my_goals > 0)
            - 1000000 * (world.enemy_goals > 0)
            );
    }
};
