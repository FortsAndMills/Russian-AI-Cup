#pragma once
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <set>
#include <iostream>
#include <map>
#include <vector>

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES
const int INF = 1000000;

using namespace model;
using namespace std;

World W;
Game G;

double min(double a, double b)
{
	if (a < b)
		return a;
	else
		return b;
}
double max(double a, double b)
{
	if (a > b)
		return a;
	else
		return b;
}
double Fabs(double a)
{
	if (a < 0)
		return -a;
	return a;
}

class MathVector
{
public:
	double x, y;
	double angle;

	double count_angle()  // Считаем угол вектора
	{
		if (x >= 0 && y >= 0)
		{
			if (x == 0)
			{
				if (y == 0)
					return 0;
				else
					return PI / 2;
			}
			return atan(y / x);
		}
		if (x >= 0 && y < 0)
		{
			if (x == 0)
				return -PI / 2;
			return atan(y / x);
		}
		if (x <= 0 && y >= 0)
		{
			if (x == 0)
				return PI / 2;
			return atan(y / x) + PI;
		}
		if (x <= 0 && y < 0)
		{
			if (x == 0)
				return -PI / 2;
			return atan(y / x) - PI;
		}
	}

	MathVector() { x = 0; y = 0; angle = 0; }
	MathVector(double _x, double _y)
	{
		x = _x;
		y = _y;
		angle = count_angle();
	}
	double module()
	{
		return sqrt(x * x + y * y);
	}
};

MathVector operator * (MathVector A, double b)
{
	return MathVector(A.x * b, A.y * b);
}
MathVector operator * (double b, MathVector A)
{
	return MathVector(A.x * b, A.y * b);
}
MathVector& operator *= (MathVector & A, double b)
{
	A.x *= b;
	A.y *= b;
	A.angle = A.count_angle();
	return A;
}
MathVector operator - (MathVector A, MathVector B)
{
	return MathVector(A.x - B.x, A.y - B.y);
}
MathVector operator + (MathVector A, MathVector B)
{
	return MathVector(A.x + B.x, A.y + B.y);
}
MathVector& operator += (MathVector & A, MathVector B)
{
	A.x += B.x;
	A.y += B.y;
	A.angle = A.count_angle();
	return A;
}
MathVector& operator -= (MathVector & A, MathVector B)
{
	A.x -= B.x;
	A.y -= B.y;
	A.angle = A.count_angle();
	return A;
}
MathVector& operator /= (MathVector & A, double b)
{
	A.x /= b;
	A.y /= b;
	A.angle = A.count_angle();
	return A;
}
MathVector operator / (MathVector A, double b)
{
	return MathVector(A.x / b, A.y / b);
}

class Line  // Класс геометрической прямой.
{
public:
	double a, b, c;
	Line() {}
	Line(double x1, double y1, double x2, double y2)
	{
		if (x1 == x2)  // прямая x = c;
		{
			b = 0;
			a = 1;
			c = -x1;
			return;
		}
		b = 1;  // иначе принимаем b за единицу
		a = (y2 - y1) / (x1 - x2);
		c = -a * x1 - y1;
	}

	double getXbyY(double y)
	{
		if (a != 0)
			return (b * y + c) / (-a);
		return -c / b;
	}
	double getYbyX(double x)
	{
		if (b != 0)
			return (a * x + c) / (-b);
		return -c / a;
	}
	double DistanceTo(double x, double y)
	{
		return Fabs((a * x + b * y + c) / sqrt(a * a + b * b));
	}
	double angle()
	{
		if (b == 0)
			return PI / 2;
		return atan(-a / b);
	}

	MathVector project(double X, double Y)
	{
		if (a == 0)
			return MathVector(X, getYbyX(X));
		
		double move = (X - getXbyY(Y)) * cos(angle());
		return MathVector(X, getXbyY(Y)) + move * MathVector(cos(angle()), sin(angle()));
	}
};
class PartLine  // Класс геометрической прямой.
{
public:
	Line A;
	double min_x, min_y, max_x, max_y;

	PartLine() {}
	PartLine(double x1, double y1, double x2, double y2)
	{
		A = Line(x1, y1, x2, y2);
		min_x = min(x1, x2);
		max_x = max(x1, x2);
		min_y = min(y1, y2);
		max_y = max(y1, y2);
	}

	MathVector Projection(double X, double Y)
	{
		MathVector proj = A.project(X, Y);
		if (proj.x < min_x)
			proj = MathVector(min_x, A.getYbyX(min_x));
		if (proj.x > max_x)
			proj = MathVector(max_x, A.getYbyX(max_x));

		return proj;
	}
	double Height(double X, double Y)
	{
		MathVector proj = Projection(X, Y);
		return (MathVector(X, Y) - proj).module();
	}
};

class Circle
{
public:
	double x, y, R;
	Circle()
	{
		x = 0;
		y = 0;
		R = 0;
	}
	Circle(double X, double Y, double _R)
	{
		x = X;
		y = Y;
		R = _R;
	}

	void CrossLine(Line A, double & ansX, double & ansY, bool left)
	{
		if (A.b == 0)
		{
			ansX = -A.c / A.a;
			if (left)
				ansY = y + sqrt(R * R - pow((ansX - x), 2));
			else
				ansY = y - sqrt(R * R - pow((ansX - x), 2));
			return;
		}
		else if (left)
			ansX = x - R / sqrt(1 + pow(A.a / A.b, 2));
		else
			ansX = x + R / sqrt(1 + pow(A.a / A.b, 2));
		ansY = (-A.c - A.a * ansX) / A.b;

		/*cerr << A.a << "x + " << A.b << "y + " << A.c << endl;
		cerr << "(x - " << x << ")^2 + (y - " << y << ")^2 = " << R * R << endl;
		cerr << left << endl;
		cerr << ansX << " " << ansY << endl;
		cerr << endl;*/
	}
	double getXbyY(double Y, bool left)
	{
		if (left)
			return x - sqrt(R * R - pow(Y - y, 2));
		else
			return sqrt(R * R - pow(Y - y, 2)) + x;
	}
	bool IsIn(double X, double Y)
	{
		return (X - x) * (X - x) + (Y - y) * (Y - y) <= R * R;
	}
	bool IsIn(MathVector M)
	{
		return IsIn(M.x, M.y);
	}
};

const double ILLUMINATION = 40;
class Trapetion
{
public:
	Line Limit;
	Line SecondLimit;
	double y;
	bool down;
	bool right;

	Trapetion() {}
	Trapetion(Line A, Line _SecondLimit, double Y, bool _down, bool _right)
	{
		Limit = A;
		SecondLimit = _SecondLimit;
		down = _down;
		y = Y;
		right = _right;
	}
	bool IsIn(double X, double Y)
	{
		return (((Limit.getYbyX(X) > Y) ^ down) || Limit.getYbyX(X) == Y) &&
			(((Y < y) ^ down) || Y == y) &&
			(((SecondLimit.getYbyX(X) < Y) ^ down) || SecondLimit.getYbyX(X) == Y);
	}
	bool IsIn(MathVector m)
	{
		return IsIn(m.x, m.y);
	}

	MathVector center()
	{
		/*if (down)
			return MathVector(Limit.getXbyY(y), (G.getRinkBottom() + y) / 2);
		return MathVector(Limit.getXbyY(y), (G.getRinkTop() + y) / 2);*/

		double Y, X = Limit.getXbyY(y);
		if (down)
			Y = G.getRinkBottom();
		else
			Y = G.getRinkTop();
		return MathVector(X, (Y * 2 + y) / 3);
	}
};