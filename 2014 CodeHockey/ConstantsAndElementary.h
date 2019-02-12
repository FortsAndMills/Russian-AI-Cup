#pragma once
#include "Classes.h"

int tick = -1;  // Номер текущего тика
string state = "RUSH";  // Состояние

double MID_VEL = 3;  // Средняя скорость
double MID_BACK_VEL_COEFF = 5.0 / 12.0;

int TEAM_SIZE;

double GoalieX[2];  // Координаты вратарей
Hockeyist MyGoalie, EnemyGoalie;  // И ссылки на них
bool DoesGoalieExist = true;

Puck puck;

map <long long int, MyHockeyist> hc;  // перечень всех хоккеистов
vector <long long int> best_hock;

double GetGoalieY(double Y)
{
	if (Y < G.getGoalNetTop() + MyGoalie.getRadius())  // Определяем позицию вратарей по позиции параметра
	{
		return G.getGoalNetTop() + MyGoalie.getRadius();
	}
	else if (Y > G.getGoalNetTop() + G.getGoalNetHeight() - MyGoalie.getRadius())
	{
		return G.getGoalNetTop() + G.getGoalNetHeight() - MyGoalie.getRadius();
	}
	return Y;
}
bool IsMyPart(double x)  // находится ли x на верной половине
{
	return ((x > (G.getRinkLeft() + G.getRinkRight()) / 2) ^ (bool_IsLeft));
}

double Speed(double sx, double sy)
{
	return sqrt(sx * sx + sy * sy);
}
double Distance(double x1, double y1, double x2, double y2)  // три функции отличаются только названием, да
{
	return sqrt(pow(x1 - x2, 2) + pow(y1 - y2, 2));
}

const double PUCK_RAD_COEFF = 1;
MathVector OppositeNetRink(bool left)
{
	if (puck.getY() <= (G.getGoalNetTop() * 2 + G.getGoalNetHeight()) / 2.0)
	{
		return MathVector(G.getRinkLeft() * left +
			G.getRinkRight() * (!left),
			G.getGoalNetTop() + G.getGoalNetHeight());
	}
	else
	{
		return MathVector(G.getRinkLeft() * left +
			G.getRinkRight() * (!left),
			G.getGoalNetTop());
	}
}
MathVector EnemyOppositeNetRink()
{
	return OppositeNetRink(!bool_IsLeft);
}
MathVector MyOppositeNetRink()
{
	return OppositeNetRink(bool_IsLeft);
}
MathVector EnemyGoalNetTarget(bool can_be_overtime = true)
{
	if (!DoesGoalieExist && can_be_overtime)
	{
		return MathVector(G.getRinkRight() * bool_IsLeft + G.getRinkLeft() * (!bool_IsLeft),
			(G.getGoalNetTop() * 2 + G.getGoalNetHeight()) / 2);
	}

	return EnemyOppositeNetRink() + MathVector(0,
		(puck.getRadius()) * (puck.getY() > (G.getGoalNetTop() * 2 + G.getGoalNetHeight()) / 2)
		- (puck.getRadius()) * (puck.getY() <= (G.getGoalNetTop() * 2 + G.getGoalNetHeight()) / 2))
		* PUCK_RAD_COEFF;
}
MathVector MyGoalNetTarget()
{
	return MyOppositeNetRink() + MathVector(0,
		(puck.getRadius()) * (puck.getY() > (G.getGoalNetTop() * 2 + G.getGoalNetHeight()) / 2)
		- (puck.getRadius()) * (puck.getY() <= (G.getGoalNetTop() * 2 + G.getGoalNetHeight()) / 2))
		* PUCK_RAD_COEFF;
}

double GET_MID_VEL(MyHockeyist &h)
{
	return h.agility() * MID_VEL;
}
double GET_MID_BACK_VEL(MyHockeyist &h)
{
	return GET_MID_VEL(h) * MID_BACK_VEL_COEFF;
}

bool CanPickPuck(MyHockeyist& h)  // определение возможности захватить шайбу
{
	return h.getDistanceTo(W.getPuck()) <= G.getStickLength() &&
		Fabs(h.getAngleTo(W.getPuck())) <= G.getStickSector() / 2;
}
bool CanStrikePuck(MyHockeyist& h)  // возможность ударить шайбу
{
	return h.getDistanceTo(W.getPuck()) <= G.getStickLength() &&
		Fabs(h.getAngleTo(W.getPuck())) <= G.getStickSector() / 2;
}
bool CanStrikeHockeyist(MyHockeyist& h, MyHockeyist& h2)  // возможность ударить шайбу
{
	return h.getDistanceTo(h2) <= G.getStickLength() &&
		Fabs(h.getAngleTo(h2)) <= G.getStickSector() / 2 &&
		h.getId() != h2.getId();
}

double Norm(double ang)
{
	while (ang < -PI)
		ang += PI * 2;
	while (ang > PI)
		ang -= PI * 2;
	return ang;
}

double max(double a, double b, double c)
{
	return max(a, max(b, c));
}
double sign(double n)
{
	if (n < 0)
		return -1;
	else if (n > 0)
		return 1;
	return 0;
}