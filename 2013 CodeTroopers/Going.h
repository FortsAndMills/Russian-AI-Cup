#pragma once
#include "Helpers.h"

class Way
{
public:
	map< pair<int, int>, set< pair<int, int> > > Map;
	vector<int> target_x, target_y;
	int start_x, start_y;
	int AllDist;
	set<int> id;
	int cur_dist;
	double Price;

	Way()
	{
		id.insert(self.getId());
	}
	void CreateWay(vector<int> x, vector<int> y, double price, const World& world)
	{
		target_x = x;
		target_y = y;
		start_x = self.getX();
		start_y = self.getY();
		AllDist = wt_dis[x[0]][y[0]];
		id.insert(self.getId());
		Price = price;

		set< pair<int, int> > was_here;
		for (int i = 0; i < (int)x.size(); ++i)
			Build(target_x[i], target_y[i], was_here, world);

		TroopersCoord.insert(make_pair(self.getX(), self.getY()));
		cur_dist = HowFarCanGo(TroopersCoord);
		TroopersCoord.erase(make_pair(self.getX(), self.getY()));
	}
	Way(int x, int y, double price, const World& world)
	{
		vector<int> x_targets, y_targets;
		x_targets.push_back(x);
		y_targets.push_back(y);
		CreateWay(x_targets, y_targets, price, world);
	}
	Way(vector<int> x, vector<int> y, double price, const World& world)
	{
		CreateWay(x, y, price, world);
	}

	void Build(int x, int y, set< pair<int, int> >& was_here, const World& world)
	{
		was_here.insert(make_pair(x, y));

		if (DoExistWT(x + 1, y, world) && wt_dis[x + 1][y] < wt_dis[x][y])
		{
			Map[make_pair(x + 1, y)].insert(make_pair(x, y));
			if (!was_here.count(make_pair(x + 1, y)))
				Build(x + 1, y, was_here, world);
		}
		if (DoExistWT(x - 1, y, world) && wt_dis[x - 1][y] < wt_dis[x][y])
		{
			Map[make_pair(x - 1, y)].insert(make_pair(x, y));
			if (!was_here.count(make_pair(x - 1, y)))
				Build(x - 1, y, was_here, world);
		}
		if (DoExistWT(x, y + 1, world) && wt_dis[x][y + 1] < wt_dis[x][y])
		{
			Map[make_pair(x, y + 1)].insert(make_pair(x, y));
			if (!was_here.count(make_pair(x, y + 1)))
				Build(x, y + 1, was_here, world);
		}
		if (DoExistWT(x, y - 1, world) && wt_dis[x][y - 1] < wt_dis[x][y])
		{
			Map[make_pair(x, y - 1)].insert(make_pair(x, y));
			if (!was_here.count(make_pair(x, y - 1)))
				Build(x, y - 1, was_here, world);
		}
	}

	int HowFarCanGo(int x, int y, map< pair<int, int>, int >& was_here, set< pair<int, int> >& TroopersCoord)
	{
		if (TroopersCoord.count(make_pair(x, y)))
			return 0;

		if (was_here.count(make_pair(x, y)))
		{
			return was_here[make_pair(x, y)];
		}

		int IsNotStart = 1;
		if (start_x == x && start_y == y)
		{
			IsNotStart = 0;
		}
		int best = IsNotStart;
		for (set< pair<int, int> >::iterator it = Map[make_pair(x, y)].begin(); it != Map[make_pair(x, y)].end(); ++it)
		{
			int k = IsNotStart + HowFarCanGo(it->first, it->second, was_here, TroopersCoord);
			if (k > best)
				best = k;
		}

		was_here[make_pair(x, y)] = best;
		return best;
	}
	int HowFarCanGo(set< pair<int, int> > TroopersCoord)
	{
		map< pair<int, int>, int > was_here;
		for (int i = 0; i < (int)target_x.size(); ++i)
		{
			if (!TroopersCoord.count(make_pair(target_x[i], target_y[i])))
				was_here[make_pair(target_x[i], target_y[i])] = 1;
		}
		
		TroopersCoord.erase(make_pair(start_x, start_y));
		return HowFarCanGo(start_x, start_y, was_here, TroopersCoord);
	}

	bool CheckId(set<int>& ids)
	{
		bool f = true;
		for (set<int>::iterator it = id.begin(); it != id.end(); ++it)
		{
			if ((*it) == self.getId())
				return true;
			else if (ids.count((*it)))
				f = false;
		}
		return f;
	}
};

vector<Way> ways;

// Проблемы с doexisting-ом. 
// Хотя если будем запускать только при рассмотре вариантов хода...
class target  // цель, выйти или войти в радиус.
{
public:
	string type;  // тип - выйти или войти
	int x, y;  // координаты цели
	TrooperStance stance;  // стойка врага
	double shooting_range;  // его радиус - если нужен
	int limit;  // текущий предел
	double price;
	vector <int> var_x;  // координаты ответов
	vector <int> var_y;
	TrooperStance my_stance;
	double ShootingRange;
	int target_index;
	BonusType bonus_type;

	target()
	{

	}
	target(int x1, int y1, TrooperStance target_stance, 
		TrooperStance my__stance, double range, int target_i, int lim)
	{
		type = "GO TO RADIUS";
		x = x1;
		y = y1;
		stance = target_stance;
		limit = lim;
		ShootingRange = range;
		my_stance = my__stance;
		target_index = target_i;
	}
    target(int x1, int y1, TrooperStance target_stance, double target_range,
		TrooperStance my__stance, int target_i, int lim)
	{
		type = "GO OUT OF RADIUS";
		x = x1;
		y = y1;
		stance = target_stance;
		limit = lim;
		shooting_range = target_range;
		my_stance = my__stance;
		target_index = target_i;
	}
	target(int x1, int y1, int target_i, int lim, bool Grenade)
	{
		if (Grenade)
			type = "GRENADE";
		else
			type = "CURE";

		x = x1;
		y = y1;
		target_index = target_i;
		limit = lim;
	}
	target(Trooper& target, int lim, bool ToGoOut)
	{
		if (ToGoOut)
			type = "GO OUT OF RADIUS";
		else
			type = "GO TO RADIUS";
		x = target.getX();
		y = target.getY();
		stance = target.getStance();
		shooting_range = target.getShootingRange();
		limit = lim;
		ShootingRange = self.getShootingRange();
		my_stance = self.getStance();
	}
	target(Trooper& target, int lim)
	{
		if (target.isTeammate())
			type = "CURE";
		else
			type = "GRENADE";

		x = target.getX();
		y = target.getY();
		stance = target.getStance();
		shooting_range = target.getShootingRange();
		limit = lim;
	}
	target(int X, int Y, int lim, double Price)
	{
		type = "GET TO DISTANCE";
		x = X;
		y = Y;
		stance = STANDING;
		shooting_range = 0;
		
		limit = lim;
		price = Price;
	}
	target(int lim)
	{
		type = "GET CAP BONUS";
		limit = lim;
		price = UNION_PEACE_PRICE;
	}
	target(int lim, bool Run)
	{
		if (Run)
		{
			type = "RUN";
			limit = lim;
		}
		else
		{
			type = "GET CAP BONUS";
			limit = lim;
			price = UNION_PEACE_PRICE;
		}
	}
	target(int lim, BonusType bt)
	{
		bonus_type = bt;
		limit = lim;
		type = "WANT BONUS";
	}
	
	void new_answer(int x, int y, int n)
	{
		if (n < limit)
		{
			var_x.clear();
			var_y.clear();
		}
		
		var_x.push_back(x);
		var_y.push_back(y);
		limit = n;
	}
	void CheckVariant(int X, int Y, int n, const World& world)
	{
		if (type == "GO OUT OF RADIUS" &&
			!world.isVisible(shooting_range, 
			x, y, stance,
			X, Y, my_stance))
		{
			new_answer(X, Y, n);
		}
		else if (type == "GO TO RADIUS" &&
			world.isVisible(ShootingRange, 
			X, Y, my_stance, 
			x, y, stance))
		{
			new_answer(X, Y, n);
		}
		else if (type == "GET CAP BONUS" &&
			CommanderBonus(X, Y, world))
		{
			new_answer(X, Y, n);
		}
		else if (type == "GET TO DISTANCE" &&
			(x - X) * (x - X) + 
			(Y - y) * (Y - y) <= 
			NEAR_EACH_OTHER * NEAR_EACH_OTHER)
		{
			new_answer(X, Y, n);
		}
		else if (type == "GRENADE" &&
			(X - x) * (X - x) +
			(Y - y) * (Y - y) <=
			game.getGrenadeThrowRange() * game.getGrenadeThrowRange())
		{
			new_answer(X, Y, n);
		}
		else if (type == "CURE" &&
			abs(X - x) + abs(Y - y) <= 1)
		{
			new_answer(X, Y, n);
		}
		else if (type == "RUN" && n == limit)
		{
			int k = 0;
			while (k < (int)enemies.size() && !world.isVisible(enemies[k].getVisionRange(),
				enemies[k].getX(), enemies[k].getY(), enemies[k].getStance(),
				X, Y, my_stance))
			{
				++k;
			}

			if (k == (int)enemies.size())
			{
				new_answer(X, Y, n);
			}
		}
		else if (type == "WANT BONUS")
		{
			if (BonusesCoord[bonus_type].count(make_pair(X, Y)))
				new_answer(X, Y, n);
		}
	}
};
class WayTarget : public target
{
public:
	Way way;
	//int best_new_dist;

	WayTarget(target& t) :
	target(t)
	{

	}
	WayTarget(Way& _way, int lim) :
		target()
	{
		way = _way;
		type = "GET FROM THE WAY";
		limit = lim;
		price = way.Price;
		//best_new_dist = way.cur_dist;
	}
};
void TargetDistance(vector<target> &targets, int x, int y, int n, const World& world)  // Ищет результаты таргетов.
{
	bool finish = true;
	for (int i = 0; i < (int)targets.size(); ++i)
	{
		if (n <= targets[i].limit)
		{
			finish = false;
			targets[i].CheckVariant(x, y, n, world);
		}
	}

	cur_dis[x][y] = n;
	if ((DoExist(x + 1, y, world) && cur_dis[x + 1][y] < n - 1) ||
		(DoExist(x - 1, y, world) && cur_dis[x - 1][y] < n - 1) ||
		(DoExist(x, y + 1, world) && cur_dis[x][y + 1] < n - 1) ||
		(DoExist(x, y - 1, world) && cur_dis[x][y - 1] < n - 1))
		return;

	if (finish)
		return;
	
	if (DoExist(x + 1, y, world))
	{
		if (cur_dis[x + 1][y] > n + 1)
			TargetDistance(targets, x + 1, y, n + 1, world);
	}
	if (DoExist(x - 1, y, world))
	{
		if (cur_dis[x - 1][y] > n + 1)
			TargetDistance(targets, x - 1, y, n + 1, world);
	}
	if (DoExist(x, y + 1, world))
	{
		if (cur_dis[x][y + 1] > n + 1)
			TargetDistance(targets, x, y + 1, n + 1, world);
	}
	if (DoExist(x, y - 1, world))
	{
		if (cur_dis[x][y - 1] > n + 1)
			TargetDistance(targets, x, y - 1, n + 1, world);
	}
}
void WayTargetDistance(vector<WayTarget> &targets, int x, int y, TrooperStance my_stance, 
	double ShootingRange, int n, const World& world)  // Ищет результаты таргетов.
{
	bool finish = true;
	for (int i = 0; i < (int)targets.size(); ++i)
	{
		if (n <= targets[i].limit)
		{
			finish = false;
			if (targets[i].type == "GET FROM THE WAY" &&
                !TroopersCoord.count(make_pair(x, y)))
			{
				TroopersCoord.insert(make_pair(x, y));
				int G = targets[i].way.HowFarCanGo(TroopersCoord);
				TroopersCoord.erase(make_pair(x, y));

				if (G > targets[i].way.cur_dist)
				{
					targets[i].new_answer(x, y, n);
				}
			}
		}
	}

	cur_dis[x][y] = n;

	if (finish)
		return;
	if ((DoExist(x + 1, y, world) && cur_dis[x + 1][y] < n - 1) ||
		(DoExist(x - 1, y, world) && cur_dis[x - 1][y] < n - 1) ||
		(DoExist(x, y + 1, world) && cur_dis[x][y + 1] < n - 1) ||
		(DoExist(x, y - 1, world) && cur_dis[x][y - 1] < n - 1))
		return;
	
	if (DoExistWT(x + 1, y, world))
	{
		if (cur_dis[x + 1][y] > n + 1)
			WayTargetDistance(targets, x + 1, y, my_stance, ShootingRange, n + 1, world);
	}
	if (DoExistWT(x - 1, y, world))
	{
		if (cur_dis[x - 1][y] > n + 1)
			WayTargetDistance(targets, x - 1, y, my_stance, ShootingRange, n + 1, world);
	}
	if (DoExistWT(x, y + 1, world))
	{
		if (cur_dis[x][y + 1] > n + 1)
			WayTargetDistance(targets, x, y + 1, my_stance, ShootingRange, n + 1, world);
	}
	if (DoExistWT(x, y - 1, world))
	{
		if (cur_dis[x][y - 1] > n + 1)
			WayTargetDistance(targets, x, y - 1, my_stance, ShootingRange, n + 1, world);
	}
}

double GetMarkDistance(double x, double y, double x1, double y1)
{
	return abs(x - x1) + abs(y - y1);
}
void HowToGoHere(set < pair<int, int> >& good, set < pair<int, int> >& was_here, int X, int Y, const World& world)
{  // length - на какое расстояние до точки надо добраться.
	was_here.insert(make_pair(X, Y));
	if (dis[X][Y] == 1)  // Если дошли до 1 - это хорошая клетка, сюда надо идти.
	{
		good.insert(make_pair(X, Y));
		return;
	}

	if ((DoExist(X + 1, Y, world)) && 
		dis[X + 1][Y] < dis[X][Y] &&
		was_here.count(make_pair(X + 1, Y)) == 0)  // Иначе ищем, где уменьшаются числа
	{
		HowToGoHere(good, was_here, X + 1, Y, world);
	}
	if (DoExist(X - 1, Y, world) && dis[X - 1][Y] < dis[X][Y]
	&& was_here.count(make_pair(X - 1, Y)) == 0)
	{
		HowToGoHere(good, was_here, X - 1, Y, world);
	}
	if (DoExist(X, Y + 1, world) && dis[X][Y + 1] < dis[X][Y]
	&& was_here.count(make_pair(X, Y + 1)) == 0)
	{
		HowToGoHere(good, was_here, X, Y + 1, world);
	}
	if (DoExist(X, Y - 1, world) && dis[X][Y - 1] < dis[X][Y]
	&& was_here.count(make_pair(X, Y - 1)) == 0)
	{
		HowToGoHere(good, was_here, X, Y - 1, world);
	}
}
void NearHere(set < pair<int, int> >& good, set < pair<int, int> >& was_here, int start_x, int start_y, int X, int Y, const World& world)
{
	was_here.insert(make_pair(X, Y));
	if (dis[start_x][start_y] - dis[X][Y] <= 2 &&
		dis[start_x][start_y] - dis[X][Y] >= 0)
	{
		good.insert(make_pair(X, Y));
		return;
	}

	if ((DoExist(X + 1, Y, world)) && 
		dis[X + 1][Y] < dis[X][Y] &&
		was_here.count(make_pair(X + 1, Y)) == 0)  // Иначе ищем, где уменьшаются числа
	{
		NearHere(good, was_here, start_x, start_y, X + 1, Y, world);
	}
	if (DoExist(X - 1, Y, world) && dis[X - 1][Y] < dis[X][Y]
	&& was_here.count(make_pair(X - 1, Y)) == 0)
	{
		NearHere(good, was_here, start_x, start_y, X - 1, Y, world);
	}
	if (DoExist(X, Y + 1, world) && dis[X][Y + 1] < dis[X][Y]
	&& was_here.count(make_pair(X, Y + 1)) == 0)
	{
		NearHere(good, was_here, start_x, start_y, X, Y + 1, world);
	}
	if (DoExist(X, Y - 1, world) && dis[X][Y - 1] < dis[X][Y]
	&& was_here.count(make_pair(X, Y - 1)) == 0)
	{
		NearHere(good, was_here, start_x, start_y, X, Y - 1, world);
	}
}
void HowToGoHereWT(set < pair<int, int> >& good, set < pair<int, int> >& was_here, int X, int Y, const World& world)
{  // length - на какое расстояние до точки надо добраться.
	was_here.insert(make_pair(X, Y));
	if (wt_dis[X][Y] == 1)  // Если дошли до 1 - это хорошая клетка, сюда надо идти.
	{
		good.insert(make_pair(X, Y));
		return;
	}

	if ((DoExistWT(X + 1, Y, world)) && 
		wt_dis[X + 1][Y] < wt_dis[X][Y] &&
		was_here.count(make_pair(X + 1, Y)) == 0)  // Иначе ищем, где уменьшаются числа
	{
		HowToGoHereWT(good, was_here, X + 1, Y, world);
	}
	if (DoExistWT(X - 1, Y, world) && wt_dis[X - 1][Y] < wt_dis[X][Y]
	&& was_here.count(make_pair(X - 1, Y)) == 0)
	{
		HowToGoHereWT(good, was_here, X - 1, Y, world);
	}
	if (DoExistWT(X, Y + 1, world) && wt_dis[X][Y + 1] < wt_dis[X][Y]
	&& was_here.count(make_pair(X, Y + 1)) == 0)
	{
		HowToGoHereWT(good, was_here, X, Y + 1, world);
	}
	if (DoExistWT(X, Y - 1, world) && wt_dis[X][Y - 1] < wt_dis[X][Y]
	&& was_here.count(make_pair(X, Y - 1)) == 0)
	{
		HowToGoHereWT(good, was_here, X, Y - 1, world);
	}
}
void WantToGoHere(vector<int> X, vector<int> Y, double Price, int length, const World& world)  // Говорим, что хорошо бы пойти в эту клетку
{ // length = 0, если в эту же точку, length = 1, если в соседнюю
	if (X.size() == 0)
		return;

	set < pair<int, int> > good;  // Выяcняем, какие клетки хорошие.
	set < pair<int, int> > was_here;
	set < pair<int, int> > was_here_wt;
	
	bool get_variants = false;
	for (int k = 0; k < (int)X.size(); ++k)
	{
		if (dis[X[k]][Y[k]] - length == 0)  // Мы уже в точке, ха-ха.
		{
			//cerr //<< "do-not-move +" //<< Price //<< endl;
			do_not_move += Price;
			continue;
		}
		else if (dis[X[k]][Y[k]] - length < 0)
		{
			//cerr //<< "move and go on all nearest fields +" //<< Price //<< endl;
			do_not_move -= Price;
			for (int i = 0; i < (int)price.size(); ++i)
				price[i] += Price;
			continue;
		}
		else if (dis[X[k]][Y[k]] == UNDEFINED && wt_dis[X[k]][Y[k]] == UNDEFINED)
		{
			//cerr //<< "(want to go to (" //<< X[k] //<< ", " //<< Y[k] //<< "), but it is too far for me, so - backup_var!" //<< endl;
			for (int i = 0; i < (int)price.size(); ++i)
			{
				if ((X[k] - new_x[i] < X[k] - self.getX() && X[k] > self.getX()) ||
					(X[k] - new_x[i] > X[k] - self.getX() && X[k] < self.getX()) ||
					(Y[k] - new_y[i] < Y[k] - self.getY() && Y[k] > self.getY()) ||
					(Y[k] - new_y[i] > Y[k] - self.getY() && Y[k] < self.getY()))
				{
					//cerr //<< "3Move to (" //<< new_x[i] //<< ", " //<< new_y[i] //<< "): +("
						//<< Price //<< ")" //<< endl;
					price[i] += Price;
				}
				else
				{
					//cerr //<< "4Move to (" //<< new_x[i] //<< ", " //<< new_y[i] //<< "): -("
						//<< Price //<< ")" //<< endl;
					price[i] -= Price;
				}
			}
			continue;
		}

		get_variants = true;
		if (dis[X[k]][Y[k]] - wt_dis[X[k]][Y[k]] < game.getCommanderAuraRange())
			HowToGoHere(good, was_here, X[k], Y[k], world);
		else
		{
			//cerr //<< "Going to (" //<< X[k] //<< ", " //<< Y[k] //<< ") on WT!!!" //<< endl;
			HowToGoHereWT(good, was_here_wt, X[k], Y[k], world);
		}
	}
	
	if (!get_variants)
		return;
	if (good.size() <= 0)
		int a = 0;
	for (int i = 0; i < (int)price.size(); ++i)
	{
		if (good.find(make_pair(new_x[i], new_y[i])) != good.end())  // Если хорошая
		{
			//cerr //<< "7Move to (" //<< new_x[i] //<< ", " //<< new_y[i] //<< "): +("
					//<< Price //<< ")" //<< endl;
			price[i] += Price;  // то плюс вот такое выражение
		}
		else
		{
			//cerr //<< "8Move to (" //<< new_x[i] //<< ", " //<< new_y[i] //<< "): -("
					//<< Price //<< ")" //<< endl;
			price[i] -= Price;  // иначе минус.
		}
	}
}
void WantToGoHere(int X, int Y, double Price, int length, const World& world)
{
	vector<int> x, y;
	x.push_back(X);
	y.push_back(Y);
	WantToGoHere(x, y, Price, length, world);
}

const int GET_AWAY_FROM_MY_WAY_LIMIT = 10;
void UpdateLocks(const World& world)
{
	int moves = HowManyMovesCanDo();
	vector <WayTarget> targets;

	set<int> ids;
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		ids.insert(troopers[i].getId());
	}

	for (int i = 0; i < (int)ways.size(); ++i)
	{
		if (ways[i].CheckId(ids) ||
			ways[i].AllDist == 0 || 
			ways[i].AllDist == UNDEFINED)
		{
			ways.erase(ways.begin() + i, ways.begin() + i + 1);
			--i;
		}
		else
		{
			TroopersCoord.insert(make_pair(self.getX(), self.getY()));
			ways[i].cur_dist = ways[i].HowFarCanGo(TroopersCoord);
			TroopersCoord.erase(make_pair(self.getX(), self.getY()));
			for (int j = 0; j < (int)price.size(); ++j)
			{
				TroopersCoord.insert(make_pair(new_x[j], new_y[j]));
				int NewResult = ways[i].HowFarCanGo(TroopersCoord);
				TroopersCoord.erase(make_pair(new_x[j], new_y[j]));

				if (NewResult < ways[i].cur_dist)
				{
					//cerr //<< "Variant to go to (" //<< new_x[j] //<< ", " //<< new_y[j] 
					//<< ") gets -" //<< ways[i].Price * 2 //<< " because of LOCK!" //<< endl;
					price[j] -= ways[i].Price * 2;
				}
			}

			if (ways[i].cur_dist != ways[i].AllDist)
			{
				//cerr //<< "Added target to get out of marshrut of someone to (" 
					//<< ways[i].target_x[0] //<< ", " //<< ways[i].target_y[0] //<< ")" //<< endl;
				targets.push_back(WayTarget(ways[i], GET_AWAY_FROM_MY_WAY_LIMIT));
			}
		}
	}

	ClearCurDis();
	WayTargetDistance(targets, self.getX(), self.getY(), self.getStance(),
		game.getCommanderAuraRange(), 0, world);
	for (int i = 0; i < (int)targets.size(); ++i)
	{
		if (targets[i].var_x.size() > 0 &&
			(targets[i].var_x.size() != 1 || 
			targets[i].var_x[0] != self.getX() ||
			targets[i].var_y[0] != self.getY()))
		{
			WantToGoHere(targets[i].var_x, targets[i].var_y, targets[i].price * 2, 0, world);
		
			//cerr //<< "Because of lock targets want way to:";
			for (int j = 0; j < (int)targets[i].var_x.size(); ++j)
			{
				//cerr //<< " (" //<< targets[i].var_x[j] //<< ", " //<< targets[i].var_y[j] //<< ")";
			}
			//cerr //<< " with price " //<< targets[i].price * 2 //<< endl;

			ways.push_back(Way(targets[i].var_x, targets[i].var_y, targets[i].price * 2, world));
		}
		else if (targets[i].var_x.size() == 0)
		{
			//cerr //<< "WayTarget didn't find answers: may be I'm aside and cann't help" //<< endl;
		}
	}
}