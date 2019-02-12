#pragma once

#include "MyStrategy.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <set>
#include <iostream>
#include <map>

using namespace model;
using namespace std;

Trooper self = Trooper();
Game game = Game(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

string STATE = "peace";  // Текущее положение

vector< vector<int> > dis;  // Для рассчёта расстояний между клетками
vector< vector<int> > wt_dis;  // Аналогично, но без своих труперов.
vector< vector<int> > cur_dis;  // Аналогично, но для произвольных клеток.
vector< vector<int> > cap_dis;  // Специально для кэпбонуса
vector< vector<CellType> > IsFree;  // Массив свободных и занятых клеток: 0 - занята, 2 - занята своим трупером, 1 - не занята.

int my_time = 0;
map<int, int> times;

int ds_update_time = -1000;

bool HasCrusade;  // Поход крестовый одна штука
int CrusadeX, CrusadeY;  // Куда идти
int CrusadeGetTime = -1000;
int REQUEST_MAX_TIME = 7;

vector<int> new_x;  // Варианты похода в соседние клетки
vector<int> new_y;
vector<double> price;
double do_not_move;

map<BonusType, set< pair<int, int> > > BonusesCoord;

map<TrooperType, bool> CapDeadlock;  // Тупик!
map<TrooperType, int> CapDeadlockTargetId;
map<int, map <int, int> > BetweenTroopersDistance;

set< pair<int, int> > TroopersCoord;
TrooperType last_type;
double mid_x, mid_y;
bool NewTurnStart;
int NumOfTypes;

vector<Trooper> troopers;  // Наши
int commander_index, self_index;
vector<Trooper> enemies;  // Не наши

const int UNDEFINED = 10000000;  // Величина неизвестна
const int DANGER_LIMITS = 10;  // Насколько далеко смотреть в поисках опасности
const int SEARCH_SAFETY_LIMITS = 1;  // За сколько MoveCost до конца раунда смотреть за опасностью
const int WALKING_LIMIT = 315;  // Предел ходьбы
const int WANT_BONUS_LIMIT = 15;
const int WANT_TO_FIGHT_LIMIT = 25;
const int WANT_TO_CURE_LIMIT = 10;
const int UNION_LIMIT = 35;
const int CAN_GET_HERE_LIMIT = 9;
const int NEAR_EACH_OTHER = 3;
const int UNION_PEACE_PRICE = 500;

int IsWaitingForAttack;
bool HasToAttack;

bool DoExist(int x, int y, const World& world)  // Определяет, можно ли пойти на такую клетку
{
	return (x >= 0 && x < world.getWidth() && y >= 0 && y < world.getHeight() && IsFree[x][y] == FREE && !TroopersCoord.count(make_pair(x, y)));
}
bool DoExistWT(int x, int y, const World& world)  // Определяет, можно ли пойти на такую клетку
{
	return (x >= 0 && x < world.getWidth() && y >= 0 && y < world.getHeight() && IsFree[x][y] == FREE);
}
bool DoExistOnMap(int x, int y, const World& world, vector< vector<bool> >& map)
{
	return (x >= 0 && x < world.getWidth() && y >= 0 && y < world.getHeight() && map[x][y]);
}

void FillField(int x, int y, int n, set< pair<int, int> >& answers, const World& world)  // Рекурсивно заполняет массив dis чиселками
{
	dis[x][y] = n;
	
	bool a = (DoExist(x + 1, y, world) && dis[x + 1][y] > n + 1);
	bool b = (DoExist(x - 1, y, world) && dis[x - 1][y] > n + 1);
	bool c = (DoExist(x, y + 1, world) && dis[x][y + 1] > n + 1);
	bool d = (DoExist(x, y - 1, world) && dis[x][y - 1] > n + 1);
	
	if (a)
	{
		dis[x + 1][y] = n + 1;
		answers.insert(make_pair(x + 1, y));
	}
	if (b)
	{
		dis[x - 1][y] = n + 1;
		answers.insert(make_pair(x - 1, y));
	}
	if (c)
	{
		dis[x][y + 1] = n + 1;
		answers.insert(make_pair(x, y + 1));
	}
	if (d)
	{
		dis[x][y - 1] = n + 1;
		answers.insert(make_pair(x, y - 1));
	}
}
void FillDistances(int start_x, int start_y, const World& world)  // Рекурсивно заполняет массив dis чиселками
{
	int n = 0;
	set < pair<int, int> > vars, new_vars;
	vars.insert(make_pair(start_x, start_y));

	while (n < WALKING_LIMIT && vars.size() > 0)
	{
		for (set < pair<int, int> >::iterator it = vars.begin(); it != vars.end(); ++it)
		{
			FillField(it->first, it->second, n, new_vars, world);
		}

		vars = new_vars;
		new_vars.clear();
		++n;
	}
}
void FillFieldWT(int x, int y, int n, set< pair<int, int> >& answers, const World& world)  // Рекурсивно заполняет массив dis чиселками
{
	wt_dis[x][y] = n;
	
	bool a = (DoExistWT(x + 1, y, world) && wt_dis[x + 1][y] > n + 1);
	bool b = (DoExistWT(x - 1, y, world) && wt_dis[x - 1][y] > n + 1);
	bool c = (DoExistWT(x, y + 1, world) && wt_dis[x][y + 1] > n + 1);
	bool d = (DoExistWT(x, y - 1, world) && wt_dis[x][y - 1] > n + 1);
	
	if (a)
	{
		wt_dis[x + 1][y] = n + 1;
		answers.insert(make_pair(x + 1, y));
	}
	if (b)
	{
		wt_dis[x - 1][y] = n + 1;
		answers.insert(make_pair(x - 1, y));
	}
	if (c)
	{
		wt_dis[x][y + 1] = n + 1;
		answers.insert(make_pair(x, y + 1));
	}
	if (d)
	{
		wt_dis[x][y - 1] = n + 1;
		answers.insert(make_pair(x, y - 1));
	}
}
void FillDistancesWT(int start_x, int start_y, const World& world)  // Рекурсивно заполняет массив dis чиселками
{
	int n = 0;
	set < pair<int, int> > vars, new_vars;
	vars.insert(make_pair(start_x, start_y));

	while (n < WALKING_LIMIT && vars.size() > 0)
	{
		for (set < pair<int, int> >::iterator it = vars.begin(); it != vars.end(); ++it)
		{
			FillFieldWT(it->first, it->second, n, new_vars, world);
		}

		vars = new_vars;
		new_vars.clear();
		++n;
	}
}
void FillFieldCap(int x, int y, int n, set< pair<int, int> >& answers, const World& world)  // Рекурсивно заполняет массив dis чиселками
{
	cap_dis[x][y] = n;
	
	bool a = (DoExistWT(x + 1, y, world) && cap_dis[x + 1][y] > n + 1);
	bool b = (DoExistWT(x - 1, y, world) && cap_dis[x - 1][y] > n + 1);
	bool c = (DoExistWT(x, y + 1, world) && cap_dis[x][y + 1] > n + 1);
	bool d = (DoExistWT(x, y - 1, world) && cap_dis[x][y - 1] > n + 1);
	
	if (a)
	{
		cap_dis[x + 1][y] = n + 1;
		answers.insert(make_pair(x + 1, y));
	}
	if (b)
	{
		cap_dis[x - 1][y] = n + 1;
		answers.insert(make_pair(x - 1, y));
	}
	if (c)
	{
		cap_dis[x][y + 1] = n + 1;
		answers.insert(make_pair(x, y + 1));
	}
	if (d)
	{
		cap_dis[x][y - 1] = n + 1;
		answers.insert(make_pair(x, y - 1));
	}
}
void FillDistancesCap(int start_x, int start_y, const World& world)  // Рекурсивно заполняет массив dis чиселками
{
	int n = 0;
	set < pair<int, int> > vars, new_vars;
	vars.insert(make_pair(start_x, start_y));

	while (n <= game.getCommanderAuraRange() && vars.size() > 0)
	{
		for (set < pair<int, int> >::iterator it = vars.begin(); it != vars.end(); ++it)
		{
			FillFieldCap(it->first, it->second, n, new_vars, world);
		}

		vars = new_vars;
		new_vars.clear();
		++n;
	}
}

void SetDis(const World& world)  // Очистка и заполнение для self-а.
{
	for (int i = 0; i < world.getWidth(); ++i)
		for (int j = 0; j < world.getHeight(); ++j)
			dis[i][j] = UNDEFINED;
	wt_dis = dis;

	// Вызывается уже когда труперов прочекают, так что до этого не надо работать с dis!!!
	FillDistances(self.getX(), self.getY(), world);
	FillDistancesWT(self.getX(), self.getY(), world);

	for (set< pair<int, int> >::iterator it = TroopersCoord.begin(); it != TroopersCoord.end(); ++it)
	{
		int best = UNDEFINED - 1;
		if (DoExist(it->first + 1, it->second, world) && dis[it->first + 1][it->second] < best)
			best = dis[it->first + 1][it->second];
		if (DoExist(it->first - 1, it->second, world) && dis[it->first - 1][it->second] < best)
			best = dis[it->first - 1][it->second];
		if (DoExist(it->first, it->second + 1, world) && dis[it->first][it->second + 1] < best)
			best = dis[it->first][it->second + 1];
		if (DoExist(it->first, it->second - 1, world) && dis[it->first][it->second - 1] < best)
			best = dis[it->first][it->second - 1];

		dis[it->first][it->second] = best + 1;
	}
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		BetweenTroopersDistance[self.getId()][troopers[i].getId()] = wt_dis[troopers[i].getX()][troopers[i].getY()];
		BetweenTroopersDistance[troopers[i].getId()][self.getId()] = wt_dis[troopers[i].getX()][troopers[i].getY()];
	}
}
void ClearCurDis()
{
	for (int i = 0; i < (int)cur_dis.size(); ++i)
		for (int j = 0; j < (int)cur_dis[i].size(); ++j)
			cur_dis[i][j] = UNDEFINED;
}
void ClearCapDis()
{
	for (int i = 0; i < (int)cur_dis.size(); ++i)
		for (int j = 0; j < (int)cur_dis[i].size(); ++j)
			cap_dis[i][j] = UNDEFINED;
}

int MoveCost(TrooperStance ts)
{
	if (ts == STANDING)
		return game.getStandingMoveCost();
	else if (ts == KNEELING)
		return game.getKneelingMoveCost();
	else
		return game.getProneMoveCost();
}
int MoveCost(Trooper& t)
{
	return MoveCost(t.getStance());
}
int HowManyMovesCanDo()
{
	return self.getActionPoints() / MoveCost(self);
}
int HowManyMovesCanDo(int left)
{
	return (self.getActionPoints() - left) / MoveCost(self);
}

bool CommanderBonus(int x, int y, const World& world)
{
	ClearCapDis();
	FillDistancesCap(x, y, world);

	int i = 0;
    for (i = 0; i < (int)troopers.size(); ++i)
    {
		int count1 = 0;
		int count2 = 0;
	    
		bool Special = true;
		for (int j = 0; j < (int)troopers.size(); ++j)
		{
			if (j != i &&
				((i != self_index && j != self_index && BetweenTroopersDistance[troopers[i].getId()][troopers[j].getId()] <= NEAR_EACH_OTHER) ||
				(i == self_index && j != self_index && cap_dis[troopers[j].getX()][troopers[j].getY()] <= NEAR_EACH_OTHER) ||
				(i != self_index && j == self_index && cap_dis[troopers[i].getX()][troopers[i].getY()] <= NEAR_EACH_OTHER)))
				++count2;

			if (j != i &&
				((i != self_index && j != self_index && BetweenTroopersDistance[troopers[i].getId()][troopers[j].getId()] <= game.getCommanderAuraRange()) ||
				(i == self_index && j != self_index && cap_dis[troopers[j].getX()][troopers[j].getY()] <= game.getCommanderAuraRange()) ||
				(i != self_index && j == self_index && cap_dis[troopers[i].getX()][troopers[i].getY()] <= game.getCommanderAuraRange())))
				++count1;
			else if (Special && j != i &&
				((i != self_index && j != self_index && BetweenTroopersDistance[troopers[i].getId()][troopers[j].getId()] <= game.getCommanderAuraRange() + 1) ||
				(i == self_index && j != self_index && cap_dis[troopers[j].getX()][troopers[j].getY()] <= game.getCommanderAuraRange() + 1) ||
				(i != self_index && j == self_index && cap_dis[troopers[i].getX()][troopers[i].getY()] <= game.getCommanderAuraRange() + 1)))
			{
				++count1;
				Special = false;
			}
		}

		if ((count2 == 0 && troopers.size() > 1) || (count1 < troopers.size() - 1 && troopers[i].getId() != self.getId()))
			return false;
    }

	return true;
}

void CreateMoveVariants(const World& world)
{
	new_x.clear();  // и составляем список возможных перемещений.
	new_y.clear();
	price.clear();
	if (DoExist(self.getX() - 1, self.getY(), world))
	{
		new_x.push_back(self.getX() - 1);
		new_y.push_back(self.getY());
		price.push_back(0);
	}
	if (DoExist(self.getX(), self.getY() + 1, world))
	{
		new_x.push_back(self.getX());
		new_y.push_back(self.getY() + 1);
		price.push_back(0);
	}
	if (DoExist(self.getX() + 1, self.getY(), world))
	{
		new_x.push_back(self.getX() + 1);
		new_y.push_back(self.getY());
		price.push_back(0);
	}
	if (DoExist(self.getX(), self.getY() - 1, world))
	{
		new_x.push_back(self.getX());
		new_y.push_back(self.getY() - 1);
		price.push_back(0);
	}
}

void CreateOpponents(const World& world);
bool IsStart = true;
void GameBegins(const World& world)
{
	NewTurnStart = true;
    HasToAttack = false;
	IsWaitingForAttack = 0;
	IsStart = false;
	CrusadeX = -1;
	CrusadeY = -1;
	CapDeadlockTargetId[COMMANDER] = -1;
	CapDeadlockTargetId[SCOUT] = -1;
	CapDeadlockTargetId[SNIPER] = -1;
	CapDeadlockTargetId[SOLDIER] = -1;
	CapDeadlockTargetId[FIELD_MEDIC] = -1;

	for (int i = 0; i < world.getWidth(); ++i)
	{
		dis.push_back(vector<int>());
		
		for (int j = 0; j < world.getHeight(); ++j)
		{
			dis[i].push_back(UNDEFINED);
		}
	}
	cur_dis = dis;
	cap_dis = dis;
	wt_dis = dis;

	CreateOpponents(world);
}
void Starter(const World& world, const Trooper& _self, const Game& _game)
{
	srand(time(NULL));
	last_type = self.getType();

	self = _self;
	game = _game;

	NewTurnStart = false;
    if (last_type != self.getType() || self.getActionPoints() == self.getInitialActionPoints())
    {
        int count = 0;
        for (int i = 0; i < (int)world.getTroopers().size(); ++i)
        {
            if (world.getTroopers()[i].isTeammate())
                ++count;
        }

        if (last_type != self.getType() || count == 1)
        {
            HasToAttack = false;
            NewTurnStart = true;
        }
    }
	
	++my_time;
	times[my_time] = world.getMoveIndex();
	HasCrusade = true;

	mid_x = 0;
	mid_y = 0;
	
	if (IsStart)
	{
        GameBegins(world);
	}

	IsFree = world.getCells();
	do_not_move = 0;
	
	CreateMoveVariants(world);

	troopers.clear();
	enemies.clear();
	commander_index = -1;

	STATE = "peace";
}
