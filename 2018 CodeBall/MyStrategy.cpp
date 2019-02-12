#include "MyStrategy.h"
#include "RandomSearch.h"

using namespace model;

int tick = -1;
int reset_ticks = 0;
int my_score = 0;
int enemy_score = 0;
RandomSearch * randomSearch;

void MyStrategy::act(const Robot& me, const Rules& rules, const Game& game, Action& action)
{
    // Initialization
    if (tick == -1)
    {
        //read_config();

        cout << "Initialization starts!" << endl;
        NUMBER_OF_ROBOTS = rules.team_size;
        NITRO_IN_GAME = game.nitro_packs.size() > 0;

        cout << NITRO_IN_GAME << ", " << NUMBER_OF_ROBOTS << endl;

        randomSearch = new RandomSearch(game);
        randomSearch->require_init = true;
    }

    // new tick started
    if (game.current_tick > tick)
    {
        tick = game.current_tick;

        // Goal check
        for (int p = 0; p < 2; ++p)
        {
            if (game.players[p].me)
            {
                if (my_score != game.players[p].score)
                {
                    cout << "I SCORED GOAL" << endl;
                    my_score = game.players[p].score;                    
                    reset_ticks = RESET_TICKS;
                    randomSearch->require_init = true;
                }
            }
            else
            {
                if (enemy_score != game.players[p].score)
                {
                    cout << "ENEMY SCORED GOAL" << endl;
                    enemy_score = game.players[p].score;
                    reset_ticks = RESET_TICKS;
                    randomSearch->require_init = true;
                }
            }
        }

        // do nothing when reset
        if (reset_ticks > 0)
        {
            reset_ticks -= 1;
        }

        if (reset_ticks == 0)
        {
            // replan!
            unsigned int start_time = clock();
            randomSearch->replan(game, tick);

            int time = clock() - start_time;

            if (NUMBER_OF_ROBOTS > 2)
            {
                if (time > 1000)
                {
                    if (time < 40000)
                    {
                        OPTIMIZE_CHARGES = min(70, OPTIMIZE_CHARGES + 1);
                    }
                    else
                    {
                        OPTIMIZE_CHARGES = max(20, OPTIMIZE_CHARGES - 1);
                    }
                }
            }

            cout << tick << " time: " << time << endl;
        }
    }

    if (reset_ticks == 0)
        randomSearch->execute(me, action, tick);
}

// CUSTOM RENDERING...
string JSON_TEXT(string text)
{
    return "{\"Text\": \"" + text + "\"}";
}
string JSON_SPHERE(double x, double y, double z, double radius, double r, double g, double b, double a)
{
    return "{ \"Sphere\": {\"x\": " + 
        std::to_string(x) + ", \"y\": " + 
        std::to_string(y) + ", \"z\": " + 
        std::to_string(z) + ", \"radius\": " + 
        std::to_string(radius) + ", \"r\": " + 
        std::to_string(r) + ", \"g\": " + 
        std::to_string(g) + ", \"b\": " + 
        std::to_string(b) + ", \"a\": " + std::to_string(a) + "}}, ";
}

std::string MyStrategy::custom_rendering()
{
    return "";

    string json = "[";
    for (int b = 0; b < BATCH_SIZE*NUMBER_OF_ROBOTS; ++b)
    {
        json += JSON_SPHERE(randomSearch->world[b].ball.position.x, 
                            randomSearch->world[b].ball.position.y, 
                            randomSearch->world[b].ball.position.z, 
                            randomSearch->world[b].ball.radius, 
                            0, 1, 1, 0.25);

        int r = b / BATCH_SIZE;
        json += JSON_SPHERE(randomSearch->world[b].robots[r].position.x,
            randomSearch->world[b].robots[r].position.y,
            randomSearch->world[b].robots[r].position.z,
            randomSearch->world[b].robots[r].radius,
            0, 1, 0, 0.25);
    }

    for (int b = BATCH_SIZE*NUMBER_OF_ROBOTS; b < 2*BATCH_SIZE*NUMBER_OF_ROBOTS; ++b)
    {
        json += JSON_SPHERE(randomSearch->world[b].ball.position.x,
            randomSearch->world[b].ball.position.y,
            randomSearch->world[b].ball.position.z,
            randomSearch->world[b].ball.radius,
            1, 0, 1, 0.25);

        int r = b / BATCH_SIZE;
        json += JSON_SPHERE(randomSearch->world[b].robots[r].position.x,
            randomSearch->world[b].robots[r].position.y,
            randomSearch->world[b].robots[r].position.z,
            randomSearch->world[b].robots[r].radius,
            1, 0, 0, 0.25);
    }

    string text = "";
    for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
    {
        text += "Robot " + std::to_string(r) + " reward is " +
            std::to_string(randomSearch->quality[randomSearch->chosen_path[r]]);
    }

    json += JSON_TEXT(text);
    json += "]";
    return json;
}



Simulation world, true_world;
ActionPlan * actions;
void check_worlds()
{
    bool error = false;
    if (world.ball.position != true_world.ball.position)
    {
        cout << "BALL POSITION WRONG" << endl;
        error = true;
    }
    if (world.ball.velocity != true_world.ball.velocity)
    {
        cout << "BALL VELOCITY WRONG" << endl;
        error = true;
    }
    for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
    {
        if (world.robots[r].position != true_world.robots[r].position)
        {
            cout << "ROBOT POSITION WRONG" << endl;
            error = true;
        }
        if (world.robots[r].velocity != true_world.robots[r].velocity)
        {
            cout << "ROBOT VELOCITY WRONG" << endl;
            error = true;
        }
    }

    if (error)
    {
        cout << "Expected:" << endl;
        world.show();
        cout << "Real:" << endl;
        true_world.show();
    }
}

void MyStrategy::DEBUG(const model::Robot& me, const model::Rules& rules, const model::Game& game, model::Action& action)
{
    if (game.current_tick > tick)
    {
        if (game.current_tick == 0)
        {
            NUMBER_OF_ROBOTS = rules.team_size;
            NITRO_IN_GAME = game.nitro_packs.size() > 0;
            actions = new ActionPlan[2 * NUMBER_OF_ROBOTS];
        }

        tick = game.current_tick;
        true_world.set(game);

        if (game.current_tick > 0)
        {
            check_worlds();
        }

        world.set(game);

        for (int r = 0; r < 2 * NUMBER_OF_ROBOTS; ++r)
        {
            if (world.robots[r].is_teammate)
            {
                actions[r].force = ROBOT_MAX_GROUND_SPEED;
                actions[r].angle = PI / 2;
            }
        }

        //world.tick(actions, 1);
    }

    actions[me.id - 1].set_action(action);
}