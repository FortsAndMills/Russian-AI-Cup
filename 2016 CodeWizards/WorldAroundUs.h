#include "Headers.h"

Point spawns[3] = { Point(3000, 200), Point(3200, 800), Point(3800, 1000) };

enum AREA {MY_HOME,
		   TOP_LANE,
		   MIDDLE_LANE,
		   BOTTOM_LANE,
		   ENEMY_HOME};

double getDistToLane(LaneType lane, double X, double Y)
{
	if (lane == LANE_MIDDLE)
		return abs(X + Y - 4000) / sqrt(2);
	if (lane == LANE_TOP)
		return Min(abs(X - 200), abs(Y - 200));
	if (lane == LANE_BOTTOM)
		return Min(abs(X - 3800), abs(Y - 3800));
}
LaneType getLane(double X, double Y)
{
	double dist_to_pos = abs(X + Y - 4000) / sqrt(2);
	double dist_to_top = Min(abs(X - 200), abs(Y - 200));
	double dist_to_down = Min(abs(X - 3800), abs(Y - 3800));

	if (dist_to_pos < dist_to_top && dist_to_pos < dist_to_down)
		return LANE_MIDDLE;
	if (dist_to_top < dist_to_down)
		return LANE_TOP;
	return LANE_BOTTOM;
}
AREA getArea(LaneType lane)
{
	if (lane == LANE_TOP)
		return TOP_LANE;
	if (lane == LANE_BOTTOM)
		return BOTTOM_LANE;
	return MIDDLE_LANE;
}
AREA getArea(double X, double Y)
{
	if (X < 800 && Y > 3200)
		return MY_HOME;
	if (Y < 800 && X > 3200)
		return ENEMY_HOME;

	return getArea(getLane(X, Y));
}
LaneType getLane(AREA area)
{
	if (area == TOP_LANE)
		return LANE_TOP;
	if (area == BOTTOM_LANE)
		return LANE_BOTTOM;
	if (area == MIDDLE_LANE)
		return LANE_MIDDLE;

	//////////////cout << tick << ") " << "ERROR; area error: " << endl;
	return LANE_MIDDLE;
}

class WhereAtLine
{
public:
	double x, y;
	double far;
	double L;
	LaneType lane;
	Point point() { return Point(x, y); }

	WhereAtLine(LaneType lane, double far)
	{
		this->lane = lane;
		this->far = far;
		if (lane == LANE_TOP)
		{
			L = (5200 + 400 * sqrt(2));
			if (far <= 2600 / L)
			{
				x = 200;
				y = 3200 - L * far;
			}
			else if (far > 1 - 2600 / L)
			{
				x = far * L - (L - 2600) + 600;
				y = 200;
			}
			else
			{
				x = (far * L - 2600) * sqrt(2) + 200;
				y = 800 - x;
			}
		}
		else if (lane == LANE_BOTTOM)
		{
			L = (5200 + 400 * sqrt(2));
			if (far <= 2600 / L)
			{
				x = far * L + 800;
				y = 3800;
			}
			else if (far > 1 - 2600 / L)
			{
				x = 3800;
				y = 3400 - (far * L - (L - 2600));
			}
			else
			{
				x = (far * L - 2600) * sqrt(2) + 3400;
				y = 7200 - x;
			}
		}
		else
		{
			L = 2400 * sqrt(2);
			x = far * 2400 + 800;
			y = 4000 - x;
		}
	}
	WhereAtLine(double X, double Y)
	{
		lane = getLane(X, Y);
		if (lane == LANE_TOP)
		{
			L = (5200 + 400 * sqrt(2));
			if (Y > 600 && Y > X + 400)
			{
				x = 200;
				y = Y;
				far = (3200 - Y) / L;
			}
			else if (X > 600 && Y < X - 400)
			{
				x = X;
				y = 200;
				far = (X - 600) / L + (L - 2600) / L;
			}
			else
			{
				x = (800 + X - Y) / 2;
				y = (800 - X + Y) / 2;
				far = (x - 200) / sqrt(2) / L + 2600 / L;
			}
		}
		else if (lane == LANE_BOTTOM)
		{
			L = (5200 + 400 * sqrt(2));
			if (X < 3200 && Y > X + 400)
			{
				x = X;
				y = 3800;
				far = (X - 800) / L;
			}
			else if (Y < 3200 && Y < X - 400)
			{
				x = 3800;
				y = Y;
				far = (3400 - Y) / L + (L - 2600) / L;
			}
			else
			{
				x = (7200 + X - Y) / 2;
				y = (7200 - X + Y) / 2;
				far = (x - 3400) / sqrt(2) / L + 2600 / L;
			}
		}
		else
		{
			L = 2400 * sqrt(2);
			x = (4000 + X - Y) / 2;
			y = (4000 - X + Y) / 2;
			far = (x - 800) / 2400.0;
		}
	}

	Point getVector()
	{
		if (lane == LANE_TOP)
		{
			if (far <= 2600 / L)
			{
				return Point(0, -1);
			}
			else if (far > 1 - 2600 / L)
			{
				return Point(1, 0);
			}
			else
			{
				return Point(1 / sqrt(2), -1 / sqrt(2));
			}
		}
		else if (lane == LANE_BOTTOM)
		{
			if (far <= 2600 / L)
			{
				return Point(1, 0);
			}
			else if (far > 1 - 2600 / L)
			{
				return Point(0, -1);
			}
			else
			{
				return Point(1 / sqrt(2), -1 / sqrt(2));
			}
		}
		else
		{
			return Point(1 / sqrt(2), -1 / sqrt(2));
		}
	}
};
bool operator < (const WhereAtLine & w1, const WhereAtLine & w2)
{
	return w1.far < w2.far;
}

// эта функци€ вы€сн€ет, пропушена ли та или ина€ лини€ в зависимости от того, какие башни ещЄ живы
// так мы будем вы€сн€ть, стоит ли обращать внимание на область вокруг домов.
int countTowers(LaneType lane, bool friends);
double TowerLife(LaneType lane, bool friends);
bool anyLanePushed(bool friends)
{
	return countTowers(LANE_TOP, !friends) == 0 || 
		countTowers(LANE_MIDDLE, !friends) == 0 || 
		countTowers(LANE_BOTTOM, !friends) == 0;
}
double nearestTower(LaneType lane);