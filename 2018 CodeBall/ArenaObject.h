#pragma once
#include "Representation.h"
#include "Strategy.h"

double HEIGHT = 20.0;
double WIDTH = 60.0;
double DEPTH = 80.0;
double GOAL_DEPTH = 10.0;
double GOAL_WIDTH = 30.0;
double GOAL_HEIGHT = 10.0;
double GOAL_TOP_RADIUS = 3.0;
double GOAL_SIDE_RADIUS = 1.0;
double BOTTOM_RADIUS = 3.0;
double CORNER_RADIUS = 13.0;
double TOP_RADIUS = 7.0;

Vec3D A1 = Vec3D(0, 0, 0);
Vec3D A2 = Vec3D(0., 1, 0);
Vec3D A3 = Vec3D(0., HEIGHT, 0);
Vec3D A4 = Vec3D(0., -1, 0);
Vec3D A5 = Vec3D(WIDTH / 2, 0, 0.);
Vec3D A6 = Vec3D(-1, 0, 0.);
Vec3D A7 = Vec3D(0., 0, (DEPTH / 2) + GOAL_DEPTH);
Vec3D A8 = Vec3D(0., 0, -1);
Vec3D A10 = Vec3D(0., 0, DEPTH / 2);
Vec3D A11 = Vec3D(0., 0, -1);
Vec3D A12 = Vec3D(GOAL_WIDTH / 2, 0, 0.);
Vec3D A13 = Vec3D(0, GOAL_HEIGHT, 0.);

class ArenaObject
{
public:
    Vec3D position;
    Vec3D velocity;
    Vec3D acceleration;

    double distance;
    Vec3D touch_normal;
    bool touch;

    double radius, radius_change_speed = 0;
    double mass, arena_e;
    
    bool is_teammate;
    double nitro_amount = 0;
    Vec3D nitro_acceleration;
    
    // auxiliary
    bool enemy_hit_ball_and_I_am_flying;

    ArenaObject()
    {
        this->radius = ROBOT_RADIUS;
        this->mass = ROBOT_MASS;
        this->arena_e = ROBOT_ARENA_E;
    }
    void set_ball_parameters()
    {
        this->radius = BALL_RADIUS;
        this->mass = BALL_MASS;
        this->arena_e = BALL_ARENA_E;
    }
    void set(const model::Ball& ball)
    {
        position.x = ball.x;
        position.y = ball.y;
        position.z = ball.z;
        velocity.x = ball.velocity_x;
        velocity.y = ball.velocity_y;
        velocity.z = ball.velocity_z;
    }
    void set(const model::Robot& robot)
    {
        is_teammate = robot.is_teammate;
        position.x = robot.x;
        position.y = robot.y;
        position.z = robot.z;
        velocity.x = robot.velocity_x;
        velocity.y = robot.velocity_y;
        velocity.z = robot.velocity_z;
        nitro_amount = robot.nitro_amount;
        touch = robot.touch;
        touch_normal.x = robot.touch_normal_x;
        touch_normal.y = robot.touch_normal_y;
        touch_normal.z = robot.touch_normal_z;

        enemy_hit_ball_and_I_am_flying = false;

        dan_to_arena();  // we require touch_normal for our action reparametrization
    }

    int time_till_collision(const ArenaObject & b) const
    {
        // returns time in steps till collides with b
        Vec3D normal = b.position - position;
        double penetration = normal.length - (radius + b.radius);
        normal.normalize();

        double vel = ((b.velocity - velocity) ^ normal) * delta_time;
        double acc = ((b.acceleration - acceleration) ^ normal) * (delta_time*delta_time);

        return quadratic_sol(acc, vel, penetration);
    }
    bool collide_entities(ArenaObject & b)
    {
        Vec3D normal = b.position - position;
        double penetration = radius + b.radius - normal.length;

        if (penetration > 0)
        {
            double k_a = (1 / mass) / ((1 / mass) + (1 / b.mass));
            double k_b = (1 / b.mass) / ((1 / mass) + (1 / b.mass));
            
            normal.normalize();
            position -= normal * penetration * k_a;
            b.position += normal * penetration * k_b;

            double delta_velocity = ((b.velocity - velocity) ^ normal) - b.radius_change_speed - radius_change_speed;
            
            if (delta_velocity < 0)
            {
                // HIT_E is set to mean between MAX_E and MIN_E, which in real game is random.
                normal *= (1 + HIT_E) * delta_velocity;
                velocity += normal * k_a;
                b.velocity -= normal * k_b;
                return true;
            }
        }
        return false;
    }

    int time_to_arena_collision()
    {
        // returns time in steps till collides with arena
        // two cases omitted :
        // 1) robot on the ground without jump
        if (radius_change_speed == 0 && arena_e == 0 && touch) return 1000000;

        double penetration = distance - radius;

        double vel = (velocity ^ touch_normal) * delta_time;
        double acc = ((acceleration - Vec3D(0, GRAVITY, 0)) ^ touch_normal) * (delta_time*delta_time);

        // 2) ball bouncing stagnates and can be neglected.
        if (arena_e != 0 && abs(vel) < 1e-4) return 1000000;

        return quadratic_sol(acc, vel, penetration);
    }
    void collide_with_arena(int steps)
    {
        dan_to_arena();
        double penetration = radius - distance;

        touch = false;
        if (penetration > 0)
        {
            position += touch_normal * penetration;
            
            double vel = (velocity ^ touch_normal) - radius_change_speed;
            if (vel < 0)
            {
                touch = true;
                // dividing by steps seems to partially compensate hits simulation error.
                velocity -= touch_normal * ((1 + arena_e / steps) * vel);
            }
        }
    }

    void move(int steps)
    {
        // in original, these two lines were in update function
        velocity += acceleration * (steps * delta_time);
        nitro_amount -= steps * delta_time * nitro_acceleration.length / NITRO_POINT_VELOCITY_CHANGE;
        if (nitro_amount < 0)  // avoiding problems with 100 microticks per step.
            nitro_amount = 0;

        // basic position update
        velocity.clamp(MAX_ENTITY_SPEED);
        position += velocity * (delta_time * steps);

        // discrete time correction(if steps = 1, no correction)
        // if acceleration is constant across the tick, this correction is precise.
        double coeff = ((steps + 1) / (2.0 * steps) - 1);
        position += acceleration * ((steps * delta_time)*(steps * delta_time)) * coeff;

        // gravity
        position.y -= GRAVITY * delta_time * delta_time / 2 * steps * steps;
        velocity.y -= GRAVITY * delta_time * steps;
    }

    // simulation part where acceleration is computed based on chosen actions
    void make_action(ActionPlan & a)
    {
        a.reparametrize(touch_normal);

        if (touch)
        {
            Vec3D target_velocity(a.target_velocity);
            target_velocity.clamp(ROBOT_MAX_GROUND_SPEED);

            target_velocity -= touch_normal * (touch_normal ^ target_velocity);

            acceleration = target_velocity - velocity;
            double tvc_length = acceleration.length;

            if (tvc_length > 0)
            {
                double max_acceleration = max(0.0, ROBOT_ACCELERATION * touch_normal.y);
                acceleration *= max_acceleration / tvc_length;
                acceleration.clamp(tvc_length / delta_time);
            }
        }
        else
            acceleration = Vec3D();
        
        if (NITRO_IN_GAME && a.use_nitro)
        {
            nitro_acceleration = a.target_velocity - velocity - acceleration * delta_time;
            nitro_acceleration.clamp(nitro_amount * NITRO_POINT_VELOCITY_CHANGE);

            double tvc_length = nitro_acceleration.length;

            if (tvc_length > 0)
            {
                nitro_acceleration *= (ROBOT_NITRO_ACCELERATION / tvc_length);
                nitro_acceleration.clamp(tvc_length / delta_time);
                acceleration += nitro_acceleration;
            }
        }
        else
            nitro_acceleration = Vec3D();

        radius = ROBOT_MIN_RADIUS + (ROBOT_MAX_RADIUS - ROBOT_MIN_RADIUS) * a.jump_speed / ROBOT_MAX_JUMP_SPEED;
        radius_change_speed = a.jump_speed;            
    }

private:
    void dan_to_plane(const Vec3D & point_on_plane, const Vec3D & plane_normal)
    {
        double dist = (position - point_on_plane) ^ plane_normal;
        if (dist < distance)
        {
            distance = dist;
            touch_normal = plane_normal;
        }
    }

    void dan_to_sphere_inner(const Vec3D & sphere_center, double sphere_radius)
    {
        Vec3D joint = sphere_center - position;
        double dist = sphere_radius - joint.length;
        if (dist < distance)
        {
            distance = dist;
            touch_normal = joint;
            touch_normal.normalize();
        }
    }

    void dan_to_sphere_outer(const Vec3D & sphere_center, double sphere_radius)
    {
        Vec3D joint = position - sphere_center;
        double dist = joint.length - sphere_radius;
        if (dist < distance)
        {
            distance = dist;
            touch_normal = joint;
            touch_normal.normalize();
        }
    }

    void dan_to_arena_quarter()
    {
        dan_to_plane(A1, A2);  // Ground
        dan_to_plane(A3, A4);  // Ceiling
        dan_to_plane(A5, A6);  // Side x
        dan_to_plane(A7, A8);  // Side z (goal)

        // Side z
        double vx = position.x - (GOAL_WIDTH / 2 - GOAL_TOP_RADIUS);
        double vy = position.y - (GOAL_HEIGHT - GOAL_TOP_RADIUS);
        if ((position.x >= GOAL_WIDTH / 2 + GOAL_SIDE_RADIUS) ||
            (position.y >= GOAL_HEIGHT + GOAL_SIDE_RADIUS) ||
            ((vx > 0 && vy > 0 && sqrt(vx*vx + vy*vy) >= GOAL_TOP_RADIUS + GOAL_SIDE_RADIUS)))
        {
            dan_to_plane(A10, A11);
        }

        // Side x & ceiling (goal)
        if (position.z >= (DEPTH / 2) + GOAL_SIDE_RADIUS)
        {
            dan_to_plane(A12, A6);
            dan_to_plane(A13, A4);
        }

        // Goal back corners
        if (position.z > (DEPTH / 2) + GOAL_DEPTH - BOTTOM_RADIUS)
        {
            dan_to_sphere_inner(Vec3D(
                    min(max(position.x,
                        BOTTOM_RADIUS - (GOAL_WIDTH / 2)),
                        (GOAL_WIDTH / 2) - BOTTOM_RADIUS),
                    min(max(position.y,
                        BOTTOM_RADIUS),
                        GOAL_HEIGHT - GOAL_TOP_RADIUS),
                        (DEPTH / 2) + GOAL_DEPTH - BOTTOM_RADIUS),
                BOTTOM_RADIUS);
        }

        // Corner
        if (position.x > (WIDTH / 2) - CORNER_RADIUS &&
            position.z > (DEPTH / 2) - CORNER_RADIUS)
        {
            dan_to_sphere_inner(Vec3D(
                (WIDTH / 2) - CORNER_RADIUS,
                position.y,
                (DEPTH / 2) - CORNER_RADIUS),
                CORNER_RADIUS);
        }

        // Goal outer corner
        if (position.z < (DEPTH / 2) + GOAL_SIDE_RADIUS)
        {
            // Side x
            if (position.x < (GOAL_WIDTH / 2) + GOAL_SIDE_RADIUS)
            {
                dan_to_sphere_outer(Vec3D(
                    (GOAL_WIDTH / 2) + GOAL_SIDE_RADIUS,
                    position.y,
                    (DEPTH / 2) + GOAL_SIDE_RADIUS),
                    GOAL_SIDE_RADIUS);
            }
            // Ceiling
            if (position.y < GOAL_HEIGHT + GOAL_SIDE_RADIUS)
            {
                dan_to_sphere_outer(Vec3D(
                    position.x,
                    GOAL_HEIGHT + GOAL_SIDE_RADIUS,
                    (DEPTH / 2) + GOAL_SIDE_RADIUS),
                    GOAL_SIDE_RADIUS);
            }

            // Top corner
            double vx = position.x - ((GOAL_WIDTH / 2) - GOAL_TOP_RADIUS);
            double vy = position.y - (GOAL_HEIGHT - GOAL_TOP_RADIUS);
            if (vx > 0 && vy > 0)
            {
                double vlength = sqrt(vx*vx + vy*vy);
                vx /= vlength;
                vy /= vlength;

                dan_to_sphere_outer(Vec3D(
                    (GOAL_WIDTH / 2) - GOAL_TOP_RADIUS
                        + vx * (GOAL_TOP_RADIUS + GOAL_SIDE_RADIUS),
                    (GOAL_HEIGHT - GOAL_TOP_RADIUS)
                        + vy * (GOAL_TOP_RADIUS + GOAL_SIDE_RADIUS),
                    (DEPTH / 2) + GOAL_SIDE_RADIUS),
                    GOAL_SIDE_RADIUS);
            }
        }

        // Goal inside top corners
        if (position.z > (DEPTH / 2) + GOAL_SIDE_RADIUS &&
            position.y > GOAL_HEIGHT - GOAL_TOP_RADIUS)
        {
            // Side x
            if (position.x > (GOAL_WIDTH / 2) - GOAL_TOP_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    (GOAL_WIDTH / 2) - GOAL_TOP_RADIUS,
                    GOAL_HEIGHT - GOAL_TOP_RADIUS,
                    position.z),
                    GOAL_TOP_RADIUS);
            }

            // Side z
            if (position.z > (DEPTH / 2) + GOAL_DEPTH - GOAL_TOP_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    position.x,
                    GOAL_HEIGHT - GOAL_TOP_RADIUS,
                    (DEPTH / 2) + GOAL_DEPTH - GOAL_TOP_RADIUS),
                    GOAL_TOP_RADIUS);
            }
        }

        // Bottom corners
        if (position.y < BOTTOM_RADIUS)
        {
            // Side x
            if (position.x > (WIDTH / 2) - BOTTOM_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    (WIDTH / 2) - BOTTOM_RADIUS,
                    BOTTOM_RADIUS,
                    position.z),
                    BOTTOM_RADIUS);
            }

            // Side z
            if (position.z > (DEPTH / 2) - BOTTOM_RADIUS &&
                position.x >= (GOAL_WIDTH / 2) + GOAL_SIDE_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    position.x,
                    BOTTOM_RADIUS,
                    (DEPTH / 2) - BOTTOM_RADIUS),
                    BOTTOM_RADIUS);
            }
        
            // Side z (goal)
            if (position.z > (DEPTH / 2) + GOAL_DEPTH - BOTTOM_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    position.x,
                    BOTTOM_RADIUS,
                    (DEPTH / 2) + GOAL_DEPTH - BOTTOM_RADIUS),
                    BOTTOM_RADIUS);
            }
        
            // Goal outer corner
            double vx = position.x - ((GOAL_WIDTH / 2) + GOAL_SIDE_RADIUS);
            double vz = position.z - ((DEPTH / 2) + GOAL_SIDE_RADIUS);
            double vlength = sqrt(vx*vx + vz*vz);
            if (vx < 0 && vy < 0 && vlength < GOAL_SIDE_RADIUS + BOTTOM_RADIUS)
            {
                vx /= vlength;
                vz /= vlength;
                dan_to_sphere_inner(Vec3D(
                    (GOAL_WIDTH / 2) + GOAL_SIDE_RADIUS
                        + vx * (GOAL_SIDE_RADIUS + BOTTOM_RADIUS),
                        BOTTOM_RADIUS,
                    (DEPTH / 2) + GOAL_SIDE_RADIUS
                        + vz * (GOAL_SIDE_RADIUS + BOTTOM_RADIUS)),
                    BOTTOM_RADIUS);
            }

            // Side x (goal)
            if (position.z >= (DEPTH / 2) + GOAL_SIDE_RADIUS &&
                position.x > (GOAL_WIDTH / 2) - BOTTOM_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    (GOAL_WIDTH / 2) - BOTTOM_RADIUS,
                    BOTTOM_RADIUS,
                    position.z),
                    BOTTOM_RADIUS);
            }

            // Corner
            if (position.x > (WIDTH / 2) - CORNER_RADIUS &&
                position.z > (DEPTH / 2) - CORNER_RADIUS)
            {
                double nx = position.x - ((WIDTH / 2) - CORNER_RADIUS);
                double nz = position.z - ((DEPTH / 2) - CORNER_RADIUS);
                double nlength = sqrt(nx*nx + nz*nz);
            
                if (nlength > CORNER_RADIUS - BOTTOM_RADIUS)
                {
                    nx /= nlength;
                    nz /= nlength;
                    dan_to_sphere_inner(Vec3D(
                        (WIDTH / 2) - CORNER_RADIUS
                            + nx * (CORNER_RADIUS - BOTTOM_RADIUS),
                        BOTTOM_RADIUS,
                        (DEPTH / 2) - CORNER_RADIUS
                            + nz * (CORNER_RADIUS - BOTTOM_RADIUS)),
                        BOTTOM_RADIUS);
                }
            }
        }

        // Ceiling corners
        if (position.y > HEIGHT - TOP_RADIUS)
        {
            // Side x
            if (position.x > (WIDTH / 2) - TOP_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    (WIDTH / 2) - TOP_RADIUS,
                    HEIGHT - TOP_RADIUS,
                    position.z),
                    TOP_RADIUS);
            }

            // Side z
            if (position.z > (DEPTH / 2) - TOP_RADIUS)
            {
                dan_to_sphere_inner(Vec3D(
                    position.x,
                    HEIGHT - TOP_RADIUS,
                    (DEPTH / 2) - TOP_RADIUS),
                    TOP_RADIUS);
            }

            // Corner
            if (position.x > (WIDTH / 2) - CORNER_RADIUS &&
                position.z > (DEPTH / 2) - CORNER_RADIUS)
            {
                double vx = position.x - ((WIDTH / 2) - CORNER_RADIUS);
                double vz = position.z - ((DEPTH / 2) - CORNER_RADIUS);
                double vlength = sqrt(vx*vx + vz*vz);

                if (vlength > CORNER_RADIUS - TOP_RADIUS)
                {
                    vx /= vlength;
                    vz /= vlength;
                    dan_to_sphere_inner(Vec3D(
                        (WIDTH / 2) - CORNER_RADIUS 
                            + vx * (CORNER_RADIUS - TOP_RADIUS),
                        HEIGHT - TOP_RADIUS,
                        (DEPTH / 2) - CORNER_RADIUS
                            + vz * (CORNER_RADIUS - TOP_RADIUS)),
                        TOP_RADIUS);
                }
            }
        }
    }

public:
    void dan_to_arena()
    {
        bool negate_x = position.x < 0;
        bool negate_z = position.z < 0;
        
        if (negate_x)
            position.x = -position.x;
        if (negate_z)
            position.z = -position.z;

        distance = 1000000;
        dan_to_arena_quarter();
        
        if (negate_x)
        {
            touch_normal.x = -touch_normal.x;
            position.x = -position.x;
        }
        if (negate_z)
        {
            touch_normal.z = -touch_normal.z;
            position.z = -position.z;
        }
    }
};
