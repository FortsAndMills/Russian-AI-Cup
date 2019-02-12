#pragma once
#include "ArenaObject.h"

class Simulation
{
public:    
    ArenaObject * robots;
    ArenaObject ball;

    int * nitro_packs;  // storing ticks to respawn or 0 if the pack is alive.
    
    int my_goals;
    int enemy_goals;

    Simulation()
    {
        robots = new ArenaObject[2 * NUMBER_OF_ROBOTS];
        nitro_packs = new int[4];
        ball.set_ball_parameters();  // setting ball's mass, arena_e, radius.
    }

    void set(const model::Game& game)
    {
        ball.set(game.ball);
        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            robots[game.robots[r].id - 1].set(game.robots[r]);
        }

        my_goals = 0;
        enemy_goals = 0;

        if (NITRO_IN_GAME)
        {
            for (int n = 0; n < 4; ++n)
            {
                // n-th pack has coordinates from NITRO_PACK_POSITIONS vector.
                nitro_packs[2 * (game.nitro_packs[n].z < 0) + (game.nitro_packs[n].x < 0)] = game.nitro_packs[n].respawn_ticks;
            }
        }
    }

private:
    int update(ActionPlan * actions, int steps, int require_steps)
    {
        // steps - minimal number of microticks to perform
        // require_steps - number of microticks till tick end

        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            robots[r].make_action(actions[r]);
        }

        if (steps > require_steps)
            steps = require_steps;
        else if (steps < require_steps)
        {
            int time_to_events = 1000000;
            for (int r = 1; r < 2 * NUMBER_OF_ROBOTS; ++r)
            {
                for (int k = 0; k < r; ++k)
                {
                    time_to_events = min(time_to_events, robots[r].time_till_collision(robots[k]));
                }
            }

            for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
            {
                time_to_events = min(time_to_events, robots[r].time_till_collision(ball));
                time_to_events = min(time_to_events, robots[r].time_to_arena_collision());
            }
            time_to_events = min(time_to_events, ball.time_to_arena_collision());

            steps = max(steps, min(require_steps, time_to_events));
        }

        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            robots[r].move(steps);
        }
        ball.move(steps);

        for (int r = 1; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            for (int k = 0; k < r; ++k)
            {
                robots[r].collide_entities(robots[k]);
            }
        }

        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            if (robots[r].collide_entities(ball))
            {
                for (int en_r = 0; en_r < 2 * NUMBER_OF_ROBOTS; ++en_r)
                    if (!robots[en_r].touch)
                        robots[en_r].enemy_hit_ball_and_I_am_flying = true;
            }
            robots[r].collide_with_arena(steps);
        }
        ball.collide_with_arena(steps);

        my_goals += (ball.position.z > DEPTH / 2 + BALL_RADIUS);
        enemy_goals += (-ball.position.z > DEPTH / 2 + BALL_RADIUS);

        if (NITRO_IN_GAME)
        {
            for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
            {
                if (robots[r].nitro_amount < MAX_NITRO_AMOUNT)
                {
                    for (int n = 0; n < 4; ++n)
                    {
                        if (nitro_packs[n] == 0 &&
                            (robots[r].position - NITRO_PACK_POSITIONS[n]).length <= robots[r].radius + NITRO_PACK_RADIUS)
                        {
                            robots[r].nitro_amount = MAX_NITRO_AMOUNT;
                            nitro_packs[n] = NITRO_PACK_RESPAWN_TICKS;
                        }
                    }
                }
            }
        }

        return steps;
    }

public:
    void tick(ActionPlan * actions, int steps, int & optimize_charges)
    {
        if (my_goals > 0 || enemy_goals > 0)
            return;

        int require_steps = MICROTICKS_PER_TICK;
        while (true)
        {
            require_steps -= update(actions, optimize_charges > 0 ? steps : require_steps, require_steps);
            if (require_steps > 0)  optimize_charges -= 1;
            else break;
        }

        for (int n = 0; n < 4; ++n)
            if (nitro_packs[n] > 0)
                nitro_packs[n] -= 1;

        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            if (robots[r].touch)
            {
                robots[r].enemy_hit_ball_and_I_am_flying = false;
            }
        }
    }

    void show()
    {
        cout << "BALL: " << ball.position.x << " " <<
            ball.position.y << " " <<
            ball.position.z << "   " <<
            ball.velocity.x << " " <<
            ball.velocity.y << " " <<
            ball.velocity.z << endl;

        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            cout << "ROBOT " << r << ": " << robots[r].position.x << " " <<
                robots[r].position.y << " " <<
                robots[r].position.z << "   " << 
                robots[r].velocity.x << " " <<
                robots[r].velocity.y << " " <<
                robots[r].velocity.z << "      " << robots[r].nitro_amount << endl;
        }
    }
};