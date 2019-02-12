#pragma once

#include "Brain.h"


const int MIN_LENGTH = 8;  // не менее 5
map <int, vector <Path *>> path_variants;
map <int, int> chosen_variant;
vector <Path *> creating_variants;

int last_key;
vector < vector <vector <double> > > best_ways;
vector < vector <bool> > isOpened;

void InitBestWays(int k, int x, int y, double est, WAY from, ROAD_TYPE prev_type)
{
	best_ways[k][x][y] = est;
	vector < WAY > ways = adjacentWays(x, y);

	for (int i = 0; i < ways.size(); ++i)
	{
		ROAD_TYPE type = GetRoadType(ways[i], from);
		double addition = estimateTurn(type, prev_type);
		int new_x = x + (ways[i] == RIGHT) - (ways[i] == LEFT);
		int new_y = y + (ways[i] == DOWN) - (ways[i] == UP);

		if (est + addition < best_ways[k][new_x][new_y])
			InitBestWays(k, new_x, new_y, est + addition, opposite(ways[i]), type);
	}
}
void StartFieldEstimations()
{
	for (int k = 0; k < keys.size(); ++k)
	{
		best_ways.push_back(vector <vector <double> >());
		for (int x = 0; x < world->getWidth(); ++x)
		{
			best_ways[k].push_back(vector <double>());
			isOpened.push_back(vector <bool>());
			for (int y = 0; y < world->getHeight(); ++y)
			{
				best_ways[k][x].push_back(INFINITY);
				isOpened[x].push_back(field[x][y].type != UNKNOWN);
			}
		}
	}

	for (int k = 0; k < keys.size(); ++k)
	{
		//InitBestWays(k, keys[k][0], keys[k][1], 0, 0, START);
	}
}
double BW(int k, int x, int y)
{
	InitBestWays(k, keys[k][0], keys[k][1], 0, 0, START);
	return best_ways[k][x][y];
}
void CheckForOpeningTiles()
{
	vector <vector <TileType> > f = world->getTilesXY();

	bool found = false;
	for (int x = 0; x < world->getWidth(); ++x)
	{
		for (int y = 0; y < world->getHeight(); ++y)
		{
			if (!isOpened[x][y] && f[x][y] != UNKNOWN)
			{
				found = true;
				field[x][y] = Tile(x, y);
				isOpened[x][y] = true;
			}
		}
	}

	if (found)
	{
		for (int k = 0; k < keys.size(); ++k)
		{
			for (int x = 0; x < world->getWidth(); ++x)
			{
				for (int y = 0; y < world->getHeight(); ++y)
				{
					best_ways[k][x][y] = INFINITY;
				}
			}
		}
	}
}

map <int, map <int, double> > path_ests;  // 25.1
double EstimatePathFlying(Path * path)
{
	if (path_ests[path->path[1]->x].count(path->path[1]->y))
		return path_ests[path->path[1]->x][path->path[1]->y];

	PhysMe P(Answer(1, 0, 0));
	bool f = Fly(path, P, WayToAngle(path->from[0]), -1, 1);
	if (f)
	{
		path_ests[path->path[1]->x][path->path[1]->y] = 1.0 / (P.tick - world->getTick() + 1) * RETURN_ESTIMATION;
	}
	else
	{
		path_ests[path->path[1]->x][path->path[1]->y] = P.tick - world->getTick();
	}

	return path_ests[path->path[1]->x][path->path[1]->y];
}
void EstimatePath(Path * path)
{
	pair<Vector, Vector> points = path->path[0]->LeavePoints(path->from[0]);
	double ang1 = self[ID]->getAngleTo(points.first.x, points.first.y);
	double ang2 = self[ID]->getAngleTo(points.second.x, points.second.y);

	double ans = min(fabs(ang1), fabs(ang2));
	if (ang1 * ang2 < 0 && max(fabs(ang1), fabs(ang2)) <= PI / 2)
		ans = 0;

	Vector MySpeed = Vector(self[ID]->getSpeedX(), self[ID]->getSpeedY());
	path->self_estimation = (1 + MySpeed.module() / 20) * ans / PI * RETURN_ESTIMATION;

	//path->self_estimation = (MySpeed.module() * MySpeed.module() / 400) * ans / PI * RETURN_ESTIMATION;  // 1 + V / 20

	//path->self_estimation += EstimatePathFlying(path);  // 25.1
}

Answer SearchWays(int oldx, int oldy)
{
	initBonuses();
	path_ests.clear();
	int X = self[ID]->getX() / game->getTrackTileSize();
	int Y = self[ID]->getY() / game->getTrackTileSize();
	int myX = X;
	int myY = Y;

	if (oldx != X || oldy != Y)
	{
		CheckForOpeningTiles();

		chosen_variant[ID] = -1;

		for (int i = 0; i < path_variants[ID].size(); ++i)
			delete path_variants[ID][i];

		path_variants[ID].clear();
		creating_variants.clear();
		creating_variants.push_back(new Path(X, Y));

		last_key = creating_variants[0]->key_id;
		
		vector <Path *> new_creating_variants;

		for (int n = 1; n <= MIN_LENGTH; ++n)
		{
			new_creating_variants.clear();

			int S = creating_variants.size();
			for (int i = 0; i < S; ++i)
			{
				Path * creating = creating_variants[i];
				int x = creating->last_x();
				int y = creating->last_y();

				if (field[x][y].type == UNKNOWN)
				{
					new_creating_variants.push_back(creating);
				}
				else if (true)//creating->path_estimation + BW(last_key % keys.size(), x, y) <= acceptable_estimation)
				{
					vector < pair<int, int> > possible = creating->possibleContinuation();
					for (int i = 0; i < possible.size(); ++i)
					{
						Path * NewPath = new Path(creating);
						NewPath->add(possible[i].first, possible[i].second);
						new_creating_variants.push_back(NewPath);

						if (possible[i].first == NewPath->tar_x() && possible[i].second == NewPath->tar_y())
						{
							NewPath->key_id++;

							if (NewPath->key_id > last_key)
							{
								//acceptable_estimation += BW(NewPath->key_id % keys.size(), keys[last_key % keys.size()][0], keys[last_key % keys.size()][1]);

								last_key = NewPath->key_id;
							}
						}
					}

					delete creating;
				}
				else
				{
					delete creating;
				}
			}

			creating_variants = new_creating_variants;
		}

		double best_tail = INFINITY;
		for (int i = 0; i < creating_variants.size(); ++i)
		{
			Path * creating = creating_variants[i];
			int x = creating->last_x();
			int y = creating->last_y();
			
			if (creating->key_id == last_key)
			{
				creating->path_estimation += BW(creating->key_id % keys.size(), x, y);

				if (creating->path_estimation < best_tail)
					best_tail = creating->path_estimation;
			}
		}

		for (int i = 0; i < creating_variants.size(); ++i)
		{
			Path * creating = creating_variants[i];
			int x = creating->last_x();
			int y = creating->last_y();

			if (creating->key_id != last_key)
			{
				creating->path_estimation += BW(creating->key_id % keys.size(), x, y);
				creating->path_estimation += best_tail - BW(creating->key_id % keys.size(), myX, myY);
			}

			if (creating->path_estimation <= best_tail + RETURN_ESTIMATION)
				path_variants[ID].push_back(creating);
			else
				delete creating;
		}
	
		//cout << "Path variants: " << path_variants[ID].size() << "!" << endl;
	}

	double best_est = INFINITY;
	Answer ans(1, 0, 0);
	//cout << endl << endl << "-------------" << endl;

	int newchv = -1;
	for (int i = 0; i < path_variants[ID].size(); ++i)
	{
		path_variants[ID][i]->EstimateBonuses();
		EstimatePath(path_variants[ID][i]);
		//cout << "estimation of " << path_variants[ID][i]->path[path_variants[ID][i]->path.size() - 1]->x << ", " << path_variants[ID][i]->path[path_variants[ID][i]->path.size() - 1]->y << " = " << path_variants[ID][i]->self_estimation << " + " << path_variants[ID][i]->path_estimation << " - " << path_variants[ID][i]->bonus_estimation << endl;

		double Est = path_variants[ID][i]->est();
		if (chosen_variant[ID] != -1 && i == chosen_variant[ID])
		{
			Est -= 100;
		}

		if (Est < best_est)
		{
			newchv = i;
			best_est = Est;
		}
	}

	if (chosen_variant[ID] != -1 && chosen_variant[ID] != newchv)
	{
		//cout << "!" << endl;
	}
	chosen_variant[ID] = newchv;

	Think(path_variants[ID][chosen_variant[ID]]);
	ans = path_variants[ID][chosen_variant[ID]]->ans;

	return ans;
}








/*const int MIN_LENGTH = 4;
vector <Path *> path_variants[ID];
int chosen_variant[ID];
vector <Path *> creating_variants;

vector < vector <vector <double> > > field_estimations;
vector < vector <vector <double> > > worst_estimations;

double best_estimation;
int last_key;

int TEST = 0;
void Travel()
{
	vector <Path *> new_creating_variants;

	int S = creating_variants.size();
	for (int i = 0; i < S; ++i)
	{
		++TEST;
		Path * creating = creating_variants[i];

		int x = creating->last_x();
		int y = creating->last_y();
		if (x == creating->tar_x() && y == creating->tar_y())
		{
			if (creating->path.size() <= MIN_LENGTH || creating->key_id < last_key)
			{
				creating->key_id++;
				if (creating->key_id > last_key)
				{
					last_key = creating->key_id;

					int X = self[ID]->getX() / game->getTrackTileSize();
					int Y = self[ID]->getY() / game->getTrackTileSize();
					best_estimation = worst_estimations[last_key % keys.size()][X][Y];
				}
			}
			else if (creating->path_estimation < best_estimation)
			{
				best_estimation = creating->path_estimation;
				path_variants[ID].push_back(creating);
			}
		}

		if (x != creating->tar_x() || y != creating->tar_y() || creating->path.size() <= MIN_LENGTH)
		{
			vector < pair<int, int> > possible = creating->possibleContinuation();

			for (int i = 0; i < possible.size(); ++i)
			{
				int new_x = possible[i].first;
				int new_y = possible[i].second;

				Path * NewPath = new Path(creating);
				NewPath->add(new_x, new_y);

				if (NewPath->path_estimation + (fabs(new_x - NewPath->tar_x()) + fabs(new_y - NewPath->tar_y())) * 100 < RETURN_ESTIMATION + best_estimation &&
					NewPath->path_estimation <= RETURN_ESTIMATION + field_estimations[NewPath->key_id % keys.size()][new_x][new_y])
				{
					if (NewPath->path_estimation < field_estimations[NewPath->key_id % keys.size()][new_x][new_y])
						field_estimations[NewPath->key_id % keys.size()][new_x][new_y] = NewPath->path_estimation;

					new_creating_variants.push_back(NewPath);
				}
				else
				{
					delete NewPath;
				}
			}

			delete creating;
		}
	}
	creating_variants = new_creating_variants;

	if (creating_variants.size())
		Travel();
}

Answer SearchWays(int oldx, int oldy)
{
	int X = self[ID]->getX() / game->getTrackTileSize();
	int Y = self[ID]->getY() / game->getTrackTileSize();

	if (oldx != X || oldy != Y)
	{
		for (int i = 0; i < path_variants[ID].size(); ++i)
			delete path_variants[ID][i];

		path_variants[ID].clear();
		creating_variants.push_back(new Path(X, Y));
		for (int k = 0; k < keys.size(); ++k)
			for (int x = 0; x < world->getWidth(); ++x)
				for (int y = 0; y < world->getHeight(); ++y)
					field_estimations[k][x][y] = INFINITY;

		last_key = creating_variants[0]->key_id;
		best_estimation = worst_estimations[last_key][X][Y];
		TEST = 0;

		Travel();

		//cout << "NUMBER OF ITERATIONS: " << TEST << endl;
	}

	double best_est = INFINITY;
	Answer ans(1, 0, 0);
	//cout << endl << endl << "-------------" << endl;
	for (int i = 0; i < path_variants[ID].size(); ++i)
	{
		Think(path_variants[ID][i]);
		//cout << "estimation = " << path_variants[ID][i]->self_estimation << " + " << path_variants[ID][i]->path_estimation << endl;

		if (path_variants[ID][i]->est() < best_est)
		{
			chosen_variant[ID] = i;
			best_est = path_variants[ID][i]->est();
			ans = path_variants[ID][i]->ans;
		}
	}

	return ans;
}

void InitTravel(int k, int x, int y, double est)
{
	if (worst_estimations[k][x][y] <= est)
		return;

	worst_estimations[k][x][y] = est;
	vector < pair<int, int> > adj = adjacent(x, y);

	for (int i = 0; i < adj.size(); ++i)
	{
		InitTravel(k, adj[i].first, adj[i].second, est + (field[adj[i].first][adj[i].second].type == STRAIGHT ? 100 : 500));
	}
}
void StartFieldEstimations()
{
	for (int k = 0; k < keys.size(); ++k)
	{
		worst_estimations.push_back(vector <vector <double> >());
		for (int x = 0; x < world->getWidth(); ++x)
		{
			worst_estimations[k].push_back(vector <double>());
			for (int y = 0; y < world->getHeight(); ++y)
			{
				worst_estimations[k][x].push_back(INFINITY);
			}
		}
	}

	for (int k = 0; k < keys.size(); ++k)
	{
		InitTravel(k, keys[k][0], keys[k][1], 0);
	}
}*/