#pragma once
#include "Geometry.h"

class TurnAction  // Поле ответа для каждого моего хоккеиста
{
public:
	double speed_up;
	double turn;
	ActionType action;
	
	int teammateindex;

	double PassAngle;
	double PassPower;

	TurnAction()
    {
        speed_up = 0;
	    turn = 0;
	    action = CANCEL_STRIKE;

	    PassAngle = 0;
	    PassPower = 1;

		teammateindex = -1;
    }
};

class PuckSearch
{
public:
	int PuckTime;  // Время для поимки шайбы
	double x;  // Куда идти для поимки шайбы
	double y;
	MathVector V;

    PuckSearch()
    {
        PuckTime = -1;
        x = -1;
        y = -1;
		V = MathVector();
    }
};

int int_IsLeft = -1;  // -1 если я справа и 1 если слева
bool bool_IsLeft = false;  // 0 если я справа и 1 если слева
class Goals
{
public:
	bool was_left_goal;
	bool was_right_goal;
	int goal_time;
	int Increment;
	int Decrement;

	void Clear()
	{
		was_left_goal = false;
		was_right_goal = false;
		goal_time = INF;
		Increment = -1;
		Decrement = -1;
	}

	Goals()
	{
		Clear();
	}

	void GoalsCount(int time)
	{
		goal_time = time;
		if (Increment == -1 && bool_IsLeft && was_right_goal)  // Забитые голы
			Increment = time;
		if (Increment == -1 && !bool_IsLeft && was_left_goal)
			Increment = time;
		if (Decrement == -1 && !bool_IsLeft && was_right_goal)
			Decrement = time;
		if (Decrement == -1 && bool_IsLeft && was_left_goal)
			Decrement = time;
	}
};

class MyHockeyist : public Hockeyist  // Хранилище данных хоккеиста.
{
public:
	PuckSearch PassTarget, Target;

	TurnAction answer;

	string state;  // состояние
	string maintask_state;
	int time_till_task;

	double mark;

	MathVector speed, position;

	MyHockeyist() : Hockeyist() {}
	MyHockeyist(Hockeyist& h, MyHockeyist & prev) : Hockeyist(h) 
	{
		state = prev.state;
		time_till_task = prev.time_till_task;

		Update(h);
	}
	void Update(Hockeyist& h)
	{
		speed = MathVector(h.getSpeedX(), h.getSpeedY());
		position = MathVector(h.getX(), h.getY());
		Target = PuckSearch();
		PassTarget = PuckSearch();
		answer = TurnAction();

		mark = strength() + endurance() * 0.1 + dexterity() + agility();
	}

	double DistanceTo(MathVector m)
	{
		return getDistanceTo(m.x, m.y);
	}
	double angleTo(MathVector m)
	{
		return getAngleTo(m.x, m.y);
	}

	double strength()
	{
		return (G.getZeroStaminaHockeyistEffectivenessFactor() * getStrength() + 
			(1 - G.getZeroStaminaHockeyistEffectivenessFactor()) * getStrength()
			* getStamina() / G.getHockeyistMaxStamina()) / 100.0;
	}
	double endurance()
	{
		return (G.getZeroStaminaHockeyistEffectivenessFactor() * getEndurance() +
			(1 - G.getZeroStaminaHockeyistEffectivenessFactor()) * getEndurance()
			* getStamina() / G.getHockeyistMaxStamina()) / 100.0;
	}
	double dexterity()
	{
		return (G.getZeroStaminaHockeyistEffectivenessFactor() * getDexterity() +
			(1 - G.getZeroStaminaHockeyistEffectivenessFactor()) * getDexterity()
			* getStamina() / G.getHockeyistMaxStamina()) / 100.0;
	}
	double agility()
	{
		return (G.getZeroStaminaHockeyistEffectivenessFactor() * getAgility() +
			(1 - G.getZeroStaminaHockeyistEffectivenessFactor()) * getAgility()
			* getStamina() / G.getHockeyistMaxStamina()) / 100.0;
	}
	double TimeFromKnokdown()
	{
		return G.getKnockdownTicksFactor() / agility();
	}
	double MaxTurnAngle()
	{
		return G.getHockeyistTurnAngleFactor() * agility();
	}
	double StrikeStrength(int SwingTicks = G.getMaxEffectiveSwingTicks())
	{
		return (G.getStrikePowerBaseFactor() +
			G.getStrikePowerGrowthFactor() * min(G.getMaxEffectiveSwingTicks(), SwingTicks)) * strength()
			* G.getStruckPuckInitialSpeedFactor();
	}
};

class PassInformation
{
public:
	MathVector V;
	double benefit, angle, power;
	Goals goals;

	PassInformation()
	{
		benefit = -INF;
		angle = 0;
		power = 0;
		V = MathVector();
	}
	PassInformation(MyHockeyist &h, double ang, double pow)
	{
		benefit = -INF;
		angle = ang;
		power = pow;

		double StrikeAngle = h.getAngle() + ang;  // Угол удара
		double Module = G.getPassPowerFactor() * G.getStruckPuckInitialSpeedFactor() * pow * h.strength()
			+ h.speed.module() * cos(StrikeAngle - h.speed.angle);  // Формула!!!

		if (Module < 0)
		{
			V = MathVector(0, 0);
		}

		V = MathVector(Module * cos(StrikeAngle), Module * sin(StrikeAngle));
	}
};