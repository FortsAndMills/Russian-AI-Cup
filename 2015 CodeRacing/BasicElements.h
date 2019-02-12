#pragma once

#define PI 3.14159265358979323846
#define E 0.0000000001
#define _USE_MATH_DEFINES

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <set>

using namespace model;
using namespace std;

const Game * game;
const World * world;

int ID;
map <int, Car *> self;

class Answer
{
public:
	double Engine;
private:
	double Wheels;
public:
	void setWheels(double W)
	{
		if (W > 1)
			Wheels = 1;
		else if (W < -1)
			Wheels = -1;
		else
			Wheels = W;
	}
	double wheels()
	{
		return Wheels;
	}
public:
	bool brake;

	Answer()
	{
		Engine = 0;
		Wheels = 0;
		brake = false;
	}
	Answer(double _Engine, double _Wheels, bool _brake)
	{
		Engine = _Engine;
		setWheels(_Wheels);
		brake = _brake;
	}
};

class Vector
{
public:
	double x, y;

	Vector() {}
	Vector(double _x, double _y)
	{
		x = _x;
		y = _y;
	}
	Vector(double angle)
	{
		x = cos(angle);
		y = sin(angle);
	}

	double module()
	{
		return sqrt(x * x + y * y);
	}
	double module2()
	{
		return x * x + y * y;
	}
	double angle()
	{
		if (module() == 0)
			return 0;
		if (y >= 0)
			return acos(x / module());
		else
			return -acos(x / module());
	}
};
double operator ^ (const Vector one, const Vector second)
{
	return one.x * second.x + one.y * second.y;
}
Vector operator + (const Vector one, const Vector second)
{
	return Vector(one.x + second.x, one.y + second.y);
}
Vector operator - (const Vector one, const Vector second)
{
	return Vector(one.x - second.x, one.y - second.y);
}
Vector operator * (const Vector one, const double second)
{
	return Vector(one.x * second, one.y * second);
}
Vector operator / (const Vector one, const double second)
{
	return Vector(one.x / second, one.y / second);
}
Vector operator += (Vector & one, const Vector second)
{
	one.x += second.x;
	one.y += second.y;
	return one;
}
Vector operator -= (Vector & one, const Vector second)
{
	one.x -= second.x;
	one.y -= second.y;
	return one;
}
Vector operator *= (Vector & one, const double second)
{
	one.x *= second;
	one.y *= second;
	return one;
}
Vector operator /= (Vector & one, const double second)
{
	one.x /= second;
	one.y /= second;
	return one;
}

class Rectangle
{
public:
	double Ax, Ay, Bx, By;

	Rectangle() {}
	Rectangle(double ax, double ay, double bx, double by)
	{
		if (ax < bx)
		{
			Ax = ax;
			Bx = bx;
		}
		else
		{
			Ax = bx;
			Bx = ax;
		}

		if (ay < by)
		{
			Ay = ay;
			By = by;
		}
		else
		{
			Ay = by;
			By = ay;
		}
	}
	
	bool isInside(double X, double Y)
	{
		return Ax < X && X < Bx && Ay < Y && Y < By;
	}
};
class Circle
{
public:
	double x, y, R;

	Circle(double X, double Y, double r)
	{
		x = X;
		y = Y;
		R = r;
	}

	bool isInside(double X, double Y)
	{
		return Vector(X - x, Y - y).module() <= R;
	}
};

double sign(double a)
{
	if (a > 0)
		return 1;
	if (a < 0)
		return -1;
	return 0;
}
int min(int a, int b)
{
	if (a < b)
		return a;
	return b;
}
int max(int a, int b)
{
	if (a > b)
		return a;
	return b;
}
double min(double a, double b)
{
	if (a < b)
		return a;
	return b;
}
double max(double a, double b)
{
	if (a > b)
		return a;
	return b;
}
double norm(double ang)
{
	while (ang <= -PI)
		ang += 2 * PI;
	while (ang > PI)
		ang -= 2 * PI;
	return ang;
}
void normalize(double &ang)
{
	ang = norm(ang);
}