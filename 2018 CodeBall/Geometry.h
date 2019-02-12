#pragma once
#include <cmath>
#include <random>
#include <algorithm>
#include <ctime>
#include <iostream>
using namespace std;

// VERSION 4.3 CHANGES:
// 35:24 vs ver.4.2
// nitro is allowed when speed is over 0.5
// nitro_amount / 40 is added to reward

// VERSION 4.4 CHANGES:
// 63:47 vs ver.4.3
// before: + influence for enemy robots, + max my influence
// now: + influence / 10 for all robots, + max my influence + max enemy influence

// VERSION 4.5 CHANGES:
// 33:16 vs ver.4.4
// force = force.z + team_sign instead of force = abs(force.z) for greater smoothness.

// VERSION 4.6 CHANGES:
// 41:49 vs ver.4.5 :(
// allowing to move across normals!

// VERSION 5: TL PROBLEMS
// HORIZON, OPTIMIZE_CHARGES -> 70

// IDEAS:
// 1) Enemy hit the ball and I am flying (EHBAIAF) penalty:
// -10 for every tick between enemy collision with ball until touch = true.
// first experiments: does not work.
// 2) if I hit ball, MIN_HIT_E. if enemy hits ball, MAX_HIT_E.
// 3) projection on direction to goals in potentials!
// 4) if touch=false, coeff = 10 instead of 0, if touch=true, coeff = 100 as now
// 5) what if strike at tick where collision was simulated?
// 6) distance to line connecting ball and attacker in defense


// Computational costs
const int BATCH_SIZE = 40;
const int RECOMPUTE_TICK = 4;
const int HORIZON = 70;
int OPTIMIZE_CHARGES = 70;

// Reparametrization parameters
double REP_MAXSPEED_PROBA = 0.5;
double SPEED_TO_ALLOW_NITRO = 0.5;
double REP_JUMP_PROBA = 0.5;

// Generation parameters
double TRAJECTORY_NOISE = 0.01;
double JUMP_PROBABILITY = 0.01;
double JUMP_CONTINUE_PROBABILITY = 0.9;
double NITRO_THRESHOLD = 0.5;

// Selection parameters
double SELECTION_QUANTILE = 0.7;
double SELECTED = 0.5;

// Mutation parameters
double MUTATION_NOISE = 0.01;
double MUTATION_JUMP_PROBABILITY = 0.002;
double MUTATION_JUMP_SHIFT_PROBABILITY = 0.3;
double MUTATION_JUMP_SHIFT_POWER = 30;

// Reward parameters
double BALL_SPEED_R = 1 / 20.0;
double ATTACK_GOAL_MARGIN_COEFF = 0.0001;
double POTENTIAL_COEFF = 100;
double NOT_MAX_INFLUENCE_COEFF = 10;
double NITRO_COEFF = 1 / 40.0;
double EHBAIAF_PENALTY = 0;

// TODO: remove before sending
/*
#include <fstream>
void read_config()
{
    ifstream myfile;
    string s;

    myfile.open("config.txt");
    if (myfile)
    {
        myfile >> s >> TRAJECTORY_NOISE;
        myfile >> s >> JUMP_PROBABILITY;
        myfile >> s >> JUMP_CONTINUE_PROBABILITY;
        myfile >> s >> NITRO_THRESHOLD;

        myfile >> s >> SELECTION_QUANTILE;
        myfile >> s >> SELECTED;

        myfile >> s >> MUTATION_NOISE;
        myfile >> s >> MUTATION_JUMP_PROBABILITY;
        myfile >> s >> MUTATION_JUMP_SHIFT_PROBABILITY;
        myfile >> s >> MUTATION_JUMP_SHIFT_POWER;

        myfile >> s >> BALL_SPEED_R;
        myfile.close();
    }
    else
    {
        cout << "Config reading error!" << endl;
    }
}
*/

#define PI 3.14159265

// random generation utils
std::default_random_engine random_generator;
std::uniform_real_distribution<double> random_uniform(-1.0, 1.0);
std::uniform_real_distribution<double> random_standard_uniform(0, 1.0);
std::normal_distribution<double> random_normal(0.0, 1.0);
double uniform() { return random_uniform(random_generator); }
double normal() { return random_normal(random_generator); }
double coin(double p) { return random_standard_uniform(random_generator) < p; }
int Power(int P)
{
    int power = 0;
    for (int i = 0; i < P; ++i)
    {
        power += 2 * coin(0.5) - 1;  // +1 or -1 with equal probabilities.
    }
    return power;
}

class Vec3D
{
public:
    double x = 0, y = 0, z = 0;
    double length = 0;

    Vec3D() {}
    Vec3D(double x, double y, double z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->length = sqrt(x*x + y*y + z*z);
    }


    Vec3D operator + (const Vec3D & other) const
    {
        return Vec3D(x + other.x, y + other.y, z + other.z);
    }
    Vec3D operator - (const Vec3D & other) const
    {
        return Vec3D(x - other.x, y - other.y, z - other.z);
    }
    Vec3D operator * (double val) const
    {
        return Vec3D(x * val, y * val, z * val);
    }
    double operator ^ (const Vec3D & other) const
    {
        return x * other.x + y * other.y + z * other.z;
    }
    Vec3D & operator *= (double val)
    {
        this->x *= val;
        this->y *= val;
        this->z *= val;
        this->length *= abs(val);
        return *this;
    }
    Vec3D & operator += (const Vec3D & other)
    {
        this->x += other.x;
        this->y += other.y;
        this->z += other.z;
        this->length = sqrt(x*x + y*y + z*z);
        return *this;
    }
    Vec3D & operator -= (const Vec3D & other)
    {
        this->x -= other.x;
        this->y -= other.y;
        this->z -= other.z;
        this->length = sqrt(x*x + y*y + z*z);
        return *this;
    }
    bool operator != (const Vec3D & other)
    {
        return abs(x - other.x) > 1e-2 || 
               abs(y - other.y) > 1e-2 || 
               abs(z - other.z) > 1e-2;
    }

    void normalize()
    {
        x /= length;
        y /= length;
        z /= length;
        length = 1;
    }
    void clamp(double lim)
    {
        if (length > lim)
        {
            double coeff = lim / length;
            x *= coeff;
            y *= coeff;
            z *= coeff;
            length = lim;
        }
    }
};

// util: returns minimum time till two trajectories collide.
// distance between two trajectories is represented as S = at^2 / 2 + bt + c
// collision happens when S <= 0
int quadratic_sol(double a, double b, double c)
{
    if (c < 0) return 0;  // trajectories already intersecting
    
    if (a == 0)           // linear equation
    {
        if (b < 0) return int(floor(-c / b));
        return 1000000;
    }

    // quadratic equation
    double D = (b*b - 2 * c * a);
    if (D >= 0 && a != 0)
    {
        D = sqrt(D);
        int sol1 = (-b - D) / a;
        int sol2 = (-b + D) / a;
        if (sol1 < 0) sol1 = 1000000;  // root < 0 corresponds to collision in the past
        if (sol2 < 0) sol2 = 1000000;
        return int(floor(min(sol1, sol2)));
    }

    return 1000000;
}