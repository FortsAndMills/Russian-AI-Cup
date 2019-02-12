#pragma once
#include "Geometry.h"

const double ROBOT_MIN_RADIUS = 1;
const double ROBOT_MAX_RADIUS = 1.05;
const double ROBOT_MAX_JUMP_SPEED = 15;
const double ROBOT_ACCELERATION = 100;
const double ROBOT_NITRO_ACCELERATION = 30;
const double ROBOT_MAX_GROUND_SPEED = 30;
const double ROBOT_ARENA_E = 0;
const double ROBOT_RADIUS = 1;
const double ROBOT_MASS = 2;
const double TICKS_PER_SECOND = 60;
const double MICROTICKS_PER_TICK = 100;
const double RESET_TICKS = 2 * TICKS_PER_SECOND;
const double BALL_ARENA_E = 0.7;
const double BALL_RADIUS = 2;
const double BALL_MASS = 1;
const double MIN_HIT_E = 0.4;
const double MAX_HIT_E = 0.5;
const double MAX_ENTITY_SPEED = 100;
const double MAX_NITRO_AMOUNT = 100;
const double START_NITRO_AMOUNT = 50;
const double NITRO_POINT_VELOCITY_CHANGE = 0.6;
const double NITRO_PACK_X = 20;
const double NITRO_PACK_Y = 1;
const double NITRO_PACK_Z = 30;
const double NITRO_PACK_RADIUS = 0.5;
const double NITRO_PACK_AMOUNT = 100;
const double NITRO_PACK_RESPAWN_TICKS = 10 * TICKS_PER_SECOND;
const double GRAVITY = 30;
const double delta_time = 1 / (MICROTICKS_PER_TICK * TICKS_PER_SECOND);

int NUMBER_OF_ROBOTS = 3;
bool NITRO_IN_GAME = true;
double HIT_E = (MIN_HIT_E + MAX_HIT_E) / 2;  // no random in simulation...


Vec3D NITRO_PACK_POSITIONS[] = {
                        Vec3D(NITRO_PACK_X, NITRO_PACK_Y, NITRO_PACK_Z),
                        Vec3D(-NITRO_PACK_X, NITRO_PACK_Y, NITRO_PACK_Z),
                        Vec3D(NITRO_PACK_X, NITRO_PACK_Y, -NITRO_PACK_Z),
                        Vec3D(-NITRO_PACK_X, NITRO_PACK_Y, -NITRO_PACK_Z)
};
Vec3D BASIC_FORWARD(0, 0, 1);
Vec3D FORWARD_WHEN_WALL(0, -1, 0);


// class for storing action in reparametrized way.
// x-y-z coordinate system is not convinient due to limitation of vector length.
class ActionPlan
{
public:
    // auxiliary
    double force = 0;              // from -ROBOT_MAX_GROUND_SPEED to ROBOT_MAX_GROUND_SPEED
    double angle = 0;              // from -PI to PI
    bool jump_activated = false;   // if false, no jump.
    double move_along_normal = 0;  // from -1 to 1: part of velocity corresponding to normal axis. 0 if use_nitro = false

    // as in Action class
    Vec3D target_velocity;         
    bool use_nitro = false;        // bool
    double jump_speed = 0;         // from 0 to ROBOT_MAX_JUMP_SPEED

    // by given touch_normal, force and angle computes target_velocity
    // this reparametrization simplifies sampling procedures.
    // yet it requires to know touch_normal.
    void reparametrize(const Vec3D & touch_normal)
    {
        // projecting BASIC_FORWARD to current plane
        Vec3D forward = BASIC_FORWARD - touch_normal * touch_normal.z;
        if (forward.length == 0)
            forward = FORWARD_WHEN_WALL;
        else
            forward.normalize();

        // rotating on angle around touch_normal
        double COS = cos(angle);
        double SIN = sin(angle);
        target_velocity = Vec3D(
            Vec3D(
                COS + (1 - COS) * touch_normal.x*touch_normal.x,
                (1 - COS) * touch_normal.x * touch_normal.y - SIN * touch_normal.z,
                (1 - COS) * touch_normal.x * touch_normal.z + SIN * touch_normal.y
            ) ^ forward,
            Vec3D(
                (1 - COS) * touch_normal.x * touch_normal.y + SIN * touch_normal.z,
                COS + (1 - COS) * touch_normal.y*touch_normal.y,
                (1 - COS) * touch_normal.y * touch_normal.z - SIN * touch_normal.x
            ) ^ forward,
            Vec3D(
                (1 - COS) * touch_normal.x * touch_normal.z - SIN * touch_normal.y,
                (1 - COS) * touch_normal.y * touch_normal.z + SIN * touch_normal.x,
                COS + (1 - COS) * touch_normal.z*touch_normal.z
            ) ^ forward
        );

        // adding part along touch_normal
        target_velocity *= sqrt(1 - move_along_normal * move_along_normal);
        target_velocity += touch_normal * move_along_normal;

        // setting absolute value
        target_velocity *= force;
    }

    void set_action(model::Action & action) const
    {
        action.target_velocity_x = target_velocity.x;
        action.target_velocity_y = target_velocity.y;
        action.target_velocity_z = target_velocity.z;
        action.jump_speed = jump_speed;
        action.use_nitro = use_nitro;
    }

    
private:
    // we will sample in terms of trajectories, which are standard normal stochastic processes.
    // each process is initiaited uniformly from [-1, 1], then for each tick N(0, s^2) is added
    double force_trajectory;
    double angle_trajectory;
    double jump_trajectory;
    double nitro_trajectory;
    double man_trajectory;  // man = move along normal

public:
    void action_from_trajectory()
    {
        // move with maximum speed with probability near 1/2
        force = force_trajectory * ROBOT_MAX_GROUND_SPEED / REP_MAXSPEED_PROBA;
        // angle is same as in trajectory
        angle = angle_trajectory * PI;
        // nitro is used if a threshold is passed; can't be used if speed is low
        use_nitro = nitro_trajectory > NITRO_THRESHOLD && abs(force_trajectory) >= SPEED_TO_ALLOW_NITRO;
        // if use_nitro = true, we may move along touch_normal axis! The fraction is same as in trajectory.
        move_along_normal = use_nitro * min(1.0, max(-1.0, man_trajectory));
        // jump with maximum power with probability near 1/2
        // by some reason, jump_speed is not clamped in given simulation code
        // hope it is clamped in real simulator...
        jump_speed = jump_activated * min(1.0, max(0.0, jump_trajectory + 2*REP_JUMP_PROBA)) * ROBOT_MAX_JUMP_SPEED;
    }

    void empty()  // clear action
    {
        force = 0;
        angle = 0;
        use_nitro = false;
        jump_speed = 0;
        move_along_normal = 0;
    }
    void sample()  // initiate processes
    {
        force_trajectory = uniform();
        angle_trajectory = uniform();
        nitro_trajectory = uniform();
        jump_trajectory = uniform();
        man_trajectory = uniform();
        jump_activated = coin(JUMP_PROBABILITY);
        action_from_trajectory();
    }

    void continue_trajectory(const ActionPlan & previous)
    {
        // continue random processes
        force_trajectory = previous.force_trajectory + TRAJECTORY_NOISE * normal();
        angle_trajectory = previous.angle_trajectory + TRAJECTORY_NOISE * normal();
        nitro_trajectory = previous.nitro_trajectory + TRAJECTORY_NOISE * normal();
        jump_trajectory = previous.jump_trajectory + TRAJECTORY_NOISE * normal();
        man_trajectory = previous.man_trajectory + TRAJECTORY_NOISE * normal();
        
        jump_activated = coin(JUMP_PROBABILITY) || 
            (previous.jump_activated && coin(JUMP_CONTINUE_PROBABILITY));
        action_from_trajectory();
    }

private:
    // when mutating, we need samples from mutation random process.
    double mutation_force_trajectory;
    double mutation_angle_trajectory;
    double mutation_jump_trajectory;
    double mutation_nitro_trajectory;
    double mutation_man_trajectory;
    bool mutation_jump_activated;

public:
    void copy(const ActionPlan & source)
    {
        mutation_force_trajectory = MUTATION_NOISE * normal();
        mutation_angle_trajectory = MUTATION_NOISE * normal();
        mutation_jump_trajectory = MUTATION_NOISE * normal();
        mutation_nitro_trajectory = MUTATION_NOISE * normal();
        mutation_man_trajectory = MUTATION_NOISE * normal();
        mutation_jump_activated = coin(MUTATION_JUMP_PROBABILITY);

        force_trajectory = source.force_trajectory + mutation_force_trajectory;
        angle_trajectory = source.angle_trajectory + mutation_angle_trajectory;
        nitro_trajectory = source.nitro_trajectory + mutation_jump_trajectory;
        jump_trajectory = source.jump_trajectory + mutation_nitro_trajectory;
        man_trajectory = source.man_trajectory + mutation_man_trajectory;
        jump_activated = source.jump_activated || mutation_jump_activated;
    }
    void continue_copy(const ActionPlan & source, const ActionPlan & previous)
    {
        mutation_force_trajectory = previous.mutation_force_trajectory + MUTATION_NOISE * normal();
        mutation_angle_trajectory = previous.mutation_angle_trajectory + MUTATION_NOISE * normal();
        mutation_jump_trajectory = previous.mutation_jump_trajectory + MUTATION_NOISE * normal();
        mutation_nitro_trajectory = previous.mutation_nitro_trajectory + MUTATION_NOISE * normal();
        mutation_man_trajectory = previous.mutation_man_trajectory + MUTATION_NOISE * normal();
        mutation_jump_activated = coin(MUTATION_JUMP_PROBABILITY) ||
            (previous.mutation_jump_activated && coin(JUMP_CONTINUE_PROBABILITY));

        force_trajectory = source.force_trajectory + mutation_force_trajectory;
        angle_trajectory = source.angle_trajectory + mutation_angle_trajectory;
        nitro_trajectory = source.nitro_trajectory + mutation_jump_trajectory;
        jump_trajectory = source.jump_trajectory + mutation_nitro_trajectory;
        man_trajectory = source.man_trajectory + mutation_man_trajectory;
        jump_activated = source.jump_activated || mutation_jump_activated;
    }
};