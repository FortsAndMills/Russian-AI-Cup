#pragma once

#include "Bonuses.h"

vector < vector <Tile> > field;
vector < vector <int> > keys;

const double RETURN_ESTIMATION = 1250;

vector <Tile> adjacent(Tile T)
{
	vector <Tile> ans;
	if ((T.ways & UP)) ans.push_back(field[T.x][T.y - 1]);
	if ((T.ways & DOWN)) ans.push_back(field[T.x][T.y + 1]);
	if ((T.ways & LEFT)) ans.push_back(field[T.x - 1][T.y]);
	if ((T.ways & RIGHT)) ans.push_back(field[T.x + 1][T.y]);
	return ans;
}
vector < pair<int, int> > adjacent(int x, int y)
{
	Tile T = field[x][y];
	vector < pair<int, int> > ans;
	if ((T.ways & UP)) ans.push_back(pair<int, int>(x, y - 1));
	if ((T.ways & DOWN)) ans.push_back(pair<int, int>(x, y + 1));
	if ((T.ways & LEFT)) ans.push_back(pair<int, int>(x - 1, y));
	if ((T.ways & RIGHT)) ans.push_back(pair<int, int>(x + 1, y));
	return ans;
}
vector < WAY > adjacentWays(int x, int y)
{
	Tile T = field[x][y];
	vector < WAY > ans;
	if ((T.ways & UP)) ans.push_back(UP);
	if ((T.ways & DOWN)) ans.push_back(DOWN);
	if ((T.ways & LEFT)) ans.push_back(LEFT);
	if ((T.ways & RIGHT)) ans.push_back(RIGHT);
	return ans;
}
bool isAdjacent(double x, double y, Tile * T2)
{
	if (x < 0 || x >= world->getWidth() ||
		y < 0 || y >= world->getHeight())
		return false;

	vector <Tile> ans = adjacent(field[x][y]);
	int i = 0;
	while (i < ans.size() && (ans[i].x != T2->x || ans[i].y != T2->y))
		++i;
	return i < ans.size();
}

enum ROAD_TYPE { STRAIGHT, TURN_LEFT, TURN_RIGHT, RETURN, START };
double estimateTurn(ROAD_TYPE New, ROAD_TYPE Last)
{
	if (New == STRAIGHT)
		return 100;
	else if (New == RETURN)
		return RETURN_ESTIMATION;
	else if ((New == TURN_LEFT && Last == TURN_RIGHT) || (New == TURN_RIGHT && Last == TURN_LEFT))
		return 75;
	else if ((New == TURN_LEFT && Last == TURN_LEFT) || (New == TURN_RIGHT && Last == TURN_RIGHT))
		return 600;
	else if (New == TURN_LEFT || New == TURN_RIGHT)
		return 150;
	return 0;
}
ROAD_TYPE GetRoadType(WAY from, WAY to)
{
	int a = WayToNum(to);
	int b = WayToNum(from);
	if (a != -1 && b != -1)
	{
		int test = (a + 4 - b) % 4;
		if (test == 1)
			return TURN_RIGHT;
		else if (test == 3)
			return TURN_LEFT;
		else if (test == 0)
			return RETURN;
		else
			return STRAIGHT;
	}

	return START;
}

class Path
{
public:
	vector <Tile *> path;
	vector <ROAD_TYPE> types;

	vector <WAY> from;
	vector <WAY> to;

	int key_id;

	double self_estimation = 0;
	double path_estimation = 0;
	double bonus_estimation = 0;
	Answer ans;

	Path(int x, int y)
	{
		key_id = self[ID]->getNextWaypointIndex();
		add(x, y);
	}
	Path(Path * another)
	{
		key_id = another->key_id;
		ans = another->ans;

		for (int i = 0; i < another->path.size(); ++i)
			add(another->path[i]->x, another->path[i]->y);
	}
	Path(Path * another, int k)
	{
		key_id = another->key_id;
		ans = another->ans;

		for (int i = 0; i <= k; ++i)
			add(another->path[i]->x, another->path[i]->y);
	}
	~Path()
	{
		for (int i = 0; i < path.size(); ++i)
			delete path[i];
	}

	double est()
	{
		return self_estimation + path_estimation - bonus_estimation;
	}
	int last_x()
	{
		return path[path.size() - 1]->x;
	}
	int last_y()
	{
		return path[path.size() - 1]->y;
	}
	int tar_x()
	{
		return world->getWaypoints()[key_id % keys.size()][0];
	}
	int tar_y()
	{
		return world->getWaypoints()[key_id % keys.size()][1];
	}
	bool hasField(int x, int y)
	{
		return false;

		int i = 0;
		while (i < path.size() && (path[i]->x != x || path[i]->y != y))
		{
			++i;
		}
		return i < path.size();
	}
	vector < pair<int, int> > possibleContinuation()
	{
		WAY all = path[path.size() - 1]->ways & (~to[path.size() - 1]);

		vector < pair<int, int> > ans;
		int x = last_x();
		int y = last_y();
		if ((all & UP) && !hasField(x, y - 1)) ans.push_back(pair<int, int>(x, y - 1));
		if ((all & DOWN) && !hasField(x, y + 1)) ans.push_back(pair<int, int>(x, y + 1));
		if ((all & LEFT) && !hasField(x - 1, y)) ans.push_back(pair<int, int>(x - 1, y));
		if ((all & RIGHT) && !hasField(x + 1, y)) ans.push_back(pair<int, int>(x + 1, y));
		return ans;
	}

	void add(WAY where)
	{
		int x = last_x();
		int y = last_y();

		if (where & UP) add(x, y - 1);
		if (where & DOWN) add(x, y + 1);
		if (where & LEFT) add(x - 1, y);
		if (where & RIGHT) add(x + 1, y);
	}
	void add(int x, int y)
	{
		path.push_back(new Tile(x, y));
		from.push_back(NONE);
		to.push_back(NONE);
		types.push_back(START);

		int P = path.size() - 1;
		if (P > 0)
		{
			int x_inc = path[P]->x - path[P - 1]->x;
			int y_inc = path[P]->y - path[P - 1]->y;

			if (x_inc == 1)
			{
				from[P - 1] = RIGHT;
				to[P] = LEFT;
			}
			else if (x_inc == -1)
			{
				from[P - 1] = LEFT;
				to[P] = RIGHT;
			}
			else if (y_inc == 1)
			{
				from[P - 1] = DOWN;
				to[P] = UP;
			}
			else
			{
				from[P - 1] = UP;
				to[P] = DOWN;
			}

			types[P - 1] = GetRoadType(from[P - 1], to[P - 1]);

			WAY cross = (from[P - 1] | to[P - 1]);
			if (cross == (UP | DOWN))
				path[P - 1]->setType(VERTICAL);
			else if (cross == (RIGHT | LEFT))
				path[P - 1]->setType(HORIZONTAL);
			else if (cross == (UP | RIGHT))
				path[P - 1]->setType(LEFT_BOTTOM_CORNER);
			else if (cross == (RIGHT | DOWN))
				path[P - 1]->setType(LEFT_TOP_CORNER);
			else if (cross == (LEFT | DOWN))
				path[P - 1]->setType(RIGHT_TOP_CORNER);
			else if (cross == (UP | LEFT))
				path[P - 1]->setType(RIGHT_BOTTOM_CORNER);

			if (P > 1)
			{
				path_estimation += estimateTurn(types[P - 1], types[P - 2]);				
			}
		}
	}

	void EstimateBonuses()
	{
		bonus_estimation = 0;
		for (int i = 1; i < path.size() - 1; ++i)
		{
			bonus_estimation += estimateBonuses(path[i]);
		}
	}
};

bool isInTrack(Vector pos)
{
	int X = pos.x / game->getTrackTileSize();
	int Y = pos.y / game->getTrackTileSize();

	return X >= 0 && X <= world->getWidth() && Y >= 0 && Y <= world->getHeight() &&
		field[X][Y].whereIs(pos).inside;
}
