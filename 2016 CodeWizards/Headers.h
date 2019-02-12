#pragma once

#include "MyStrategy.h"

#define PI 3.14159265358979323846
#define _USE_MATH_DEFINES
#define E 0.000001

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <ctime>

//#include "Debug.h"
//Debug //debug = Debug();

using namespace model;
using namespace std;

Wizard * self = NULL;
World * world = NULL;
Game * game = NULL;
int tick = 0;
int WAS_DEAD = 0;

template <class T>
inline T Min(T a, T b)
{
	return a < b ? a : b;
}
template <class T>
inline T Max(T a, T b)
{
	return a < b ? b : a;
}
int Sign(double a)
{
	if (a > 0)
		return 1;
	if (a < 0)
		return -1;
	return 0;
}
double normalize(double angle)
{
	while (angle > PI)
		angle -= 2 * PI;
	while (angle <= -PI)
		angle += 2 * PI;
	return angle;
}
inline double limit(double a, double Limit)
{
	return abs(a) > Limit ?  Limit * Sign(a) : a;
}

class Point
{
public:
	double x, y;

	Point() : Point(-1, -1) {}
	Point(double x, double y)
	{
		this->x = x;
		this->y = y;

		if (x != x)
		{
			//////////////cout << "PANIC!";
		}
	}
	Point(double angle)
	{
		x = cos(angle);
		y = sin(angle);
	}

	Point & make_module(double new_module)
	{
		if (x == 0 && y == 0)
		{
			//////////////cout << "ERROR: module error!" << endl;
			return *this;
		}

		double module = sqrt(norm2());
		x *= new_module / module;
		y *= new_module / module;
		return *this;
	}
	inline double norm2() const
	{
		return x * x + y * y;
	}
	Point turn(double alpha)
	{
		double new_x = x * cos(alpha) - y * sin(alpha);
		y = x * sin(alpha) + y * cos(alpha);
		x = new_x;
		return *this;
	}
	inline double angle() const
	{
		return atan2(y, x);
	}
};
Point operator + (const Point & A, const Point & B)
{
	return Point(A.x + B.x, A.y + B.y);
}
double operator * (const Point & A, const Point & B)
{
	return A.x * B.x + A.y * B.y;
}
Point operator - (const Point & A, const Point & B)
{
	return Point(A.x - B.x, A.y - B.y);
}
Point operator * (const double a, const Point & B)
{
	return Point(a * B.x, a * B.y);
}
Point operator - (const Point & B)
{
	return Point(-B.x, -B.y);
}
bool operator < (const Point & A, const Point & B)
{
	return A.norm2() < B.norm2();
}
bool operator == (const Point & A, const Point & B)
{
	return A.x == B.x && A.y == B.y;
}
bool operator != (const Point & A, const Point & B)
{
	return A.x != B.x || A.y != B.y;
}
Point & operator += (Point & A, const Point & B)
{
	A.x += B.x;
	A.y += B.y;
	return A;
}
Point & operator -= (Point & A, const Point & B)
{
	A.x -= B.x;
	A.y -= B.y;
	return A;
}

class Circle
{
public:
	double x, y;
	inline Point pos() const { return Point(x, y); };
	double R;

	Circle(Point C, double R)
	{
		this->x = C.x;
		this->y = C.y;
		this->R = R;
	}

	inline bool isIn(Point A) const
	{
		return (A - pos()).norm2() < R * R - E;
	}
	bool isInside(const Circle & small) const
	{
		return (small.pos() - pos()).norm2() < (R - small.R) * (R - small.R) - E;
	}
	Point touch(Point A, double sign = 1)
	{
		double D = sqrt((pos() - A).norm2());
		double mod = sqrt(D * D - R * R);

		if (R >= D || mod < E)
			return A;
		
		return A + mod / D * (pos() - A).turn(sign * asin(R / D));
	}

	set<Point> key_points() const
	{
		set<Point> ans;
		ans.insert(Point(x + R, y));
		ans.insert(Point(x - R, y));
		ans.insert(Point(x, y + R));
		ans.insert(Point(x, y - R));
		ans.insert(Point(x + R / sqrt(2), y + R / sqrt(2)));
		ans.insert(Point(x + R / sqrt(2), y - R / sqrt(2)));
		ans.insert(Point(x - R / sqrt(2), y + R / sqrt(2)));
		ans.insert(Point(x - R / sqrt(2), y - R / sqrt(2)));
		return ans;
	}
};

class Segment
{
public:
	Point A, B;
	double a, b, c;
	Segment() {}
	Segment(Point A, Point B)
	{
		this->A = A;
		this->B = B;

		if (A.x == -1 || B.x == -1)
		{
			//////////////cout << "ERROR: segmentation... he-he, fault!" << endl;
		}

		if (A.x == B.x)
			a = 1, b = 0, c = -A.x;
		else
			a = (B.y - A.y) / (B.x - A.x), b = -1, c = -a * A.x - b * A.y;
	}
	
	bool intersects(const Circle & circle)
	{
		double dist = abs(circle.pos().x * a + circle.pos().y * b + c) / sqrt(a * a + b * b);
		if (dist < circle.R)
		{
			if ((circle.pos() - A) * (B - A) > 0 && (circle.pos() - B) * (A - B) > 0)
				return true;
			else if (circle.isIn(A) || circle.isIn(B))
				return true;
		}
		return false;
	}

	double how_far(const Circle & circle)
	{
		Point w1 = (circle.pos() - A);
		Point w2 = (B - A);
		double cosinus = w1 * w2 / sqrt(w1.norm2() * w2.norm2());

		if (abs(cosinus) > 1.0)
		{
			cosinus = limit(cosinus, 1);
		}
		
		double dist = sqrt(w1.norm2());
		double h = dist * sqrt(1 - cosinus * cosinus);
		double L1 = dist * cosinus;

		if (h > circle.R)
		{
			return INFINITY;
		}

		double L2 = sqrt(circle.R * circle.R - h * h);
		return L1 - L2;
	}
	double how_far(Point P)
	{
		return abs(P.x * a + P.y * b + c) / sqrt(a * a + b * b);
	}

	int side(Point P)
	{
		return Sign(a * P.x + b * P.y + c);
	}
	double len() const
	{
		return sqrt((B - A).norm2());
	}
};
double len(const vector<Segment> & segments)
{
	double ans = 0;
	int segments_size = segments.size();
	for (int i = 0; i < segments_size; ++i)
		ans += segments[i].len();
	return ans;
}

class Line
{
public:
	Point A, B;
	double a, b, c;

	void fillCoeff()
	{
		if (A.x == B.x)
			a = 1, b = 0, c = -A.x;
		else
			a = (B.y - A.y) / (B.x - A.x), b = -1, c = -a * A.x - b * A.y;
	}
	Line(Point A, Point B)
	{
		this->A = A;
		this->B = B;
		fillCoeff();
	}
	Line(Point A, Circle circle, double sign = 1)
	{
		this->A = A;
		this->B = circle.touch(A, sign);

		if (A == B)
			this->B = A + (circle.pos() - A).turn(sign * PI / 2);
		
		fillCoeff();
	}

	bool intersects(const Circle & circle)
	{
		double dist = (circle.pos().x * a + circle.pos().y * b + c) / sqrt(a * a + b * b);
		return dist < circle.R - E;
	}

	Point intersects(Line L)
	{
		if (b != 0)
		{
			double x = (-L.c - L.b * c) / (L.a + L.b * a);
			double y = (-c - a * x) / b;
			return Point(x, y);
		}
		double x = -c / a;
		double y = (-L.c - L.a * x) / L.b;
		return Point(x, y);
	}

	bool parallel(Line anoth)
	{
		if (b == 0)
		{
			if (anoth.b == 0)
				return true;
			return false;
		}
		return abs(a / b - anoth.a / anoth.b) < E;
	}
};

enum OBSTACLE_TYPE { WIZARD, MINION, TOWER, BASE, TREE };
class Obstacle : public Circle
{
public:
	int allowed_sign = 0;
	OBSTACLE_TYPE type;
	
	Obstacle() : Circle(Point(), 0) {}
	Obstacle(Point C, double R, OBSTACLE_TYPE type) : Circle(C, R) { this->type = type; }
	virtual ~Obstacle() {}
};

ostream & operator << (ostream & out, const Point & P)
{
	return out << "(" << P.x << ", " << P.y << ")";
}
ostream & operator << (ostream & out, const Segment & S)
{
	return out << S.A << ", " << S.B;
}
ostream & operator << (ostream & out, const Circle & C)
{
	return out << C.pos() << "--" << C.R;
}

void drawArrow(double x1, double y1, double x2, double y2, int32_t color = 0)
{
	//debug.line(x1, y1, x2, y2, color);
	Point par(x1 - x2, y1 - y2);
	par = (1 / 10.0) * par;

	Point ar1 = par.turn(PI / 20) + Point(x2, y2);
	//debug.line(ar1.x, ar1.y, x2, y2, color);
	Point ar2 = par.turn(-PI / 10) + Point(x2, y2);
	//debug.line(ar2.x, ar2.y, x2, y2, color);
}
void drawCross(double x, double y, double R, int32_t color)
{
	//debug.line(x + R, y + R, x - R, y - R, color);
	//debug.line(x + R, y - R, x - R, y + R, color);
}
