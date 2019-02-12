#pragma once

#include "BasicElements.h"

typedef int WAY;
#define UP 1
#define DOWN 2
#define RIGHT 4
#define LEFT 8
#define NONE 0
WAY opposite(WAY w)
{
	WAY ans = 0;
	if (w & UP) ans |= DOWN;
	if (w & DOWN) ans |= UP;
	if (w & RIGHT) ans |= LEFT;
	if (w & LEFT) ans |= RIGHT;
	return ans;
}
int WayToNum(WAY w)
{
	if (w == UP)
		return 0;
	if (w == RIGHT)
		return 1;
	if (w == DOWN)
		return 2;
	if (w == LEFT)
		return 3;
	return -1;
}
double WayToAngle(WAY w)
{
	if (w == UP)
		return -PI / 2;
	if (w == RIGHT)
		return 0;
	if (w == DOWN)
		return PI / 2;
	if (w == LEFT)
		return PI;

	//cout << "FATAL ERROR" << endl;
	int a = 0;
	return 0 / a;
}
double MiddleAngle(WAY w)
{
	if (w == (UP | RIGHT))
		return -PI / 4;
	if (w == (RIGHT | DOWN))
		return PI / 4;
	if (w == (DOWN | LEFT))
		return 3 * PI / 4;
	if (w == (LEFT | UP))
		return -3 * PI / 4;

	//cout << "FATAL ERROR" << endl;
	int a = 0;
	return 0 / a;
}

class BorderInfo
{
public:
	bool inside;
	bool outside;

	int path_index = -1;

	bool isRect;
	int i;

	double X, Y;

	BorderInfo() {}
	BorderInfo(bool _isRect, int _i, double _X, double _Y)
	{
		inside = false;
		outside = false;

		isRect = _isRect;
		i = _i;

		X = _X;
		Y = _Y;
	}
	BorderInfo(bool isInside, double _X, double _Y)
	{
		inside = isInside;
		outside = !isInside;

		X = _X;
		Y = _Y;
	}
};

class Tile
{
public:
	int x, y;
	TileType type;
	TileType original_type;

	WAY ways;

	vector <Rectangle> borts;
	vector <WAY> borts_side;
	vector <Circle> edges;
	vector <WAY> edges_turns;
	Tile(int _x, int _y)
	{
		x = _x;
		y = _y;

		original_type = EMPTY;
		if (x >= 0 && x < world->getWidth() && y >= 0 && y < world->getHeight())
			original_type = world->getTilesXY()[x][y];
		setType(original_type);
	}
	void setType(TileType _type)
	{
		type = _type;

		ways = TileWays(type);

		double T = game->getTrackTileSize();
		double M = game->getTrackTileMargin();
		double marg = 0;

		edges.clear();
		edges_turns.clear();
		if ((ways & UP) && (ways & RIGHT))
		{
			edges.push_back(Circle(x * T + T, y * T, M + marg));
			edges_turns.push_back(UP | RIGHT);
		}
		if ((ways & DOWN) && (ways & RIGHT))
		{
			edges.push_back(Circle(x * T + T, y * T + T, M + marg));
			edges_turns.push_back(DOWN | RIGHT);
		}
		if ((ways & DOWN) && (ways & LEFT))
		{
			edges.push_back(Circle(x * T, y * T + T, M + marg));
			edges_turns.push_back(DOWN | LEFT);
		}
		if ((ways & UP) && (ways & LEFT))
		{
			edges.push_back(Circle(x * T, y * T, M + marg));
			edges_turns.push_back(UP | LEFT);
		}

		borts.clear();
		borts_side.clear();
		if (!(ways & UP))
		{
			borts.push_back(Rectangle(x * T, y * T, x * T + T, y * T + M + marg));
			borts_side.push_back(UP);
		}
		if (!(ways & DOWN))
		{
			borts.push_back(Rectangle(x * T, y * T + T - M - marg, x * T + T, y * T + T));
			borts_side.push_back(DOWN);
		}
		if (!(ways & RIGHT))
		{
			borts.push_back(Rectangle(x * T + T - M - marg, y * T, x * T + T, y * T + T));
			borts_side.push_back(RIGHT);
		}
		if (!(ways & LEFT))
		{
			borts.push_back(Rectangle(x * T, y * T, x * T + M + marg, y * T + T));
			borts_side.push_back(LEFT);
		}
	}

	bool contains(double X, double Y)
	{
		double Ax = x * game->getTrackTileSize();
		double Ay = y * game->getTrackTileSize();
		double Bx = (x + 1) * game->getTrackTileSize();
		double By = (y + 1) * game->getTrackTileSize();

		return Ax <= X && X <= Bx && Ay <= Y && Y <= By;
	}
	BorderInfo whereIs(double X, double Y)
	{
		if (!contains(X, Y))
			return BorderInfo(false, X, Y);

		int i = 0;
		while (i < borts.size() && !borts[i].isInside(X, Y))
			++i;
		if (i < borts.size())
			return BorderInfo(true, i, X, Y);

		i = 0;
		while (i < edges.size() && !edges[i].isInside(X, Y))
			++i;
		if (i < edges.size())
			return BorderInfo(false, i, X, Y);

		return BorderInfo(true, X, Y);
	}
	BorderInfo whereIs(Vector V)
	{
		return whereIs(V.x, V.y);
	}

	pair <Vector, Vector> LeavePoints(WAY w)
	{
		double T = game->getTrackTileSize();
		double M = game->getTrackTileMargin() + self[ID]->getHeight() / 2;
		
		if (w == UP)
			return pair<Vector, Vector>(Vector(x * T + M, y * T), Vector(x * T + T - M, y * T));
		if (w == DOWN)
			return pair<Vector, Vector>(Vector(x * T + T - M, y * T + T), Vector(x * T + M, y * T + T));
		if (w == LEFT)
			return pair<Vector, Vector>(Vector(x * T, y * T + M), Vector(x * T, y * T + T - M));
		if (w == RIGHT)
			return pair<Vector, Vector>(Vector(x * T + T, y * T + T - M), Vector(x * T + T, y * T + M));
		
		return pair<Vector, Vector>();
	}
	Vector LeavePoint(WAY w, WAY to)
	{
		pair <Vector, Vector> ans = LeavePoints(w);
		if ((WayToNum(to) - WayToNum(w) + 4) % 4 == 1)
			return ans.first;
		return ans.second;
	}
	Vector MainLeavePoint(WAY w)
	{
		double T = game->getTrackTileSize();
		double M = game->getTrackTileMargin() + self[ID]->getHeight() / 2;

		if (w == UP)
			return Vector(x * T + T / 2, y * T);
		if (w == DOWN)
			return Vector(x * T + T / 2, y * T + T);
		if (w == LEFT)
			return Vector(x * T, y * T + T / 2);
		if (w == RIGHT)
			return Vector(x * T + T, y * T + T / 2);

		return Vector();
	}

	double borderNormal(BorderInfo wb)
	{
		if (wb.isRect)
		{
			if (borts_side[wb.i] == UP)
				return PI / 2;
			else if (borts_side[wb.i] == DOWN)
				return -PI / 2;
			else if (borts_side[wb.i] == RIGHT)
				return PI;
			else if (borts_side[wb.i] == LEFT)
				return 0;
		}
		return Vector(wb.X - edges[wb.i].x, wb.Y - edges[wb.i].y).angle();
	}

	WAY TileWays(TileType T)
	{
		if (T == HORIZONTAL)
			return RIGHT | LEFT;
		if (T == VERTICAL)
			return UP | DOWN;
		if (T == LEFT_TOP_CORNER)
			return DOWN | RIGHT;
		if (T == RIGHT_TOP_CORNER)
			return DOWN | LEFT;
		if (T == LEFT_BOTTOM_CORNER)
			return UP | RIGHT;
		if (T == RIGHT_BOTTOM_CORNER)
			return UP | LEFT;
		if (T == LEFT_HEADED_T)
			return DOWN | LEFT | UP;
		if (T == RIGHT_HEADED_T)
			return DOWN | RIGHT | UP;
		if (T == BOTTOM_HEADED_T)
			return DOWN | RIGHT | LEFT;
		if (T == TOP_HEADED_T)
			return UP | LEFT | RIGHT;
		if (T == CROSSROADS)
			return DOWN | UP | RIGHT | LEFT;

		if (T == UNKNOWN)
		{
			WAY ans = 0;
			if (x + 1 < world->getWidth())
				ans |= RIGHT;
			if (x - 1 >= 0)
				ans |= LEFT;
			if (y + 1 < world->getHeight())
				ans |= DOWN;
			if (y - 1 >= 0)
				ans |= UP;
			return ans;
		}

		return 0;
	}
};



