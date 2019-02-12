#include "WarPieces.h"

class WarWizard;
map<int, WarWizard *> wizards;
WarWizard * me;

class WarMinion;
map<int, WarMinion *> minions;
vector<WarMinion *> shadow_minions;

class WarTower;
map<int, WarTower *> towers;

class WarTree;
map <int, WarTree *> trees;

// Ищем, куда дальше всего продвинулась по той или иной линии та или иная команда
WhereAtLine getWarSegment(LaneType lane, bool friends)
{
	WhereAtLine Segment_pos(lane, friends ? 0.0 : 1.0);
	for(auto P: pieces)
	{
		if (P->isFriend == friends && P->isValid() && P->isOnLane(lane))
		{
			// мы движемся к 1.0, враг к 0.0
			WhereAtLine push_Segment(P->x, P->y);

			if ((Segment_pos < push_Segment && friends) || (push_Segment < Segment_pos && !friends))
				Segment_pos = push_Segment;
		}
	}
	
	return Segment_pos;
}

// Ищем ближайшую к 0.0 точку, которую нам НЕ видно, бинарным поиском
double PRECISION = 0.001;
Point getNearestUnvisible(LaneType lane, double limit1, double limit2)
{
	if (limit2 - limit1 < PRECISION)
		return WhereAtLine(lane, limit2).point();

	double mid_limit = (3 * limit1 + limit2) / 4;  // кажется обычно, что ближе к limit1 ответ
	Point test_point = WhereAtLine(lane, mid_limit).point();
	for (auto P : pieces)
	{
		if (P->isFriend && P->isValid() && P->lane() == lane && P->isWatching(test_point))
		{
			return getNearestUnvisible(lane, mid_limit, limit2);
		}
	}
	return getNearestUnvisible(lane, limit1, mid_limit);
}
Point getEnemyFarestUnvisible(LaneType lane, double limit1, double limit2)
{
	if (limit2 - limit1 < PRECISION)
		return WhereAtLine(lane, limit1).point();

	double mid_limit = (limit1 + 3 * limit2) / 4;
	Point test_point = WhereAtLine(lane, mid_limit).point();
	for (auto P : pieces)
	{
		if (!P->isFriend && P->status != DEAD && P->lane() == lane && P->isWatching(test_point))
		{
			return getEnemyFarestUnvisible(lane, limit1, mid_limit);
		}
	}
	return getEnemyFarestUnvisible(lane, mid_limit, limit2);
}

// какое время товарищ будет добираться до намеченной цели
int WarPiece::timeToWarSegment()
{
	WhereAtLine WTL(x, y);

	// предполагается добраться до точки, которая отстоит от фронта противника на расстояние видимости волшебника
	double frontSegment = getWarSegment(WTL.lane, !isFriend).far;
	int sign = (isFriend ? -1 : 1);
	frontSegment += sign * game->getWizardVisionRange() / WTL.L;

	double front_distance = sign * (WTL.far - frontSegment) * WTL.L;  // итого расстояние до фронта по линии
	double line_distance = sqrt(getDist2To(WTL.point()));                    // расстояние до самой линии
	
	// складываем расстояния, а не применяем теорему Пифагора
	return Max(0, (int)((front_distance + line_distance) / getAverageSpeed()));
}

// функция оценки происходящего в бою
// считаем время, за которое враг побьёт нас, и время, за которое мы его, исходя из средней огневой мощи и суммарного здоровья
int estimateWarFront(LaneType lane, const vector<WarPiece*> & war)
{
	double enemy_attack, enemy_attack_td, enemy_defence;
	double my_attack, my_attack_td, my_defence;
	int time_of_win, time_of_lose, prev_time_of_battle = 0, time_of_battle = 0;
	double est_my_attack = 0, est_enemy_attack = 0;
	double me, enemy;

	//////////////cout << tick << ", ESTIMATE WARLINE----------------------" << endl;
	do
	{
		//////////////cout << time_of_battle << " iteration:" << endl;
		enemy_attack = 0, enemy_attack_td = 0, enemy_defence = 0;
		my_attack = 0, my_attack_td = 0, my_defence = 0;
		me = 0, enemy = 0;
		
		for (auto P : war)
		{
			double push_Segment = WhereAtLine(P->x, P->y).far;

			// если он успеет до того, как битва "закончится"
			int time_to_go = 0;
			if (P->status == GO_INTO_BATTLE)
				time_to_go = P->timeToWarSegment();

			if (time_to_go <= time_of_battle)
			{
				//int time_to_die = time_of_battle == 0 ? 0 : Min(time_of_battle, (int)(ceil)(3 * P->life / (P->isFriend ? est_enemy_attack : est_my_attack)));

				double C = time_of_battle == 0 ? 1 : (time_of_battle - time_to_go) / (double)time_of_battle;

				//////////////cout << "    " << P->id << " is in battle: a = " << P->attack() << " * " << C << ", d = " << P->defence() << endl;
				if (P->isFriend)
				{
					my_attack += P->attack() * C;
					my_defence += P->defence();
					me += P->attack() * C * P->defence();
				}
				else
				{
					enemy_attack += P->attack() * C;
					enemy_defence += P->defence();
					enemy += P->attack() * C * P->defence();
				}
			}
		}

		// высчитываем новое ожидаемое время сражения.
		time_of_win = my_attack == 0 ? INFINITY : ceil(enemy_defence / my_attack);
		time_of_lose = enemy_attack == 0 ? INFINITY : ceil(my_defence / enemy_attack);

		prev_time_of_battle = time_of_battle;
		est_my_attack = my_attack;
		est_enemy_attack = enemy_attack;

		time_of_battle = Min(time_of_lose, time_of_win);
	} 
	while (time_of_battle > prev_time_of_battle);

	return me - enemy;

	// иначе просто время сражения со знаком победы или поражения.
	//return time_of_lose > time_of_win ? time_of_win : -time_of_lose;
}

// функция оценки происходящего на линии
double estimateWarline(LaneType lane)
{
	// собираем тех, кто на заданной линии
	vector<WarPiece*> war;
	for (WarPiece * piece : pieces)
		if (piece->isInBattle(getArea(lane), true) && piece->type != TOWER && piece->type != BASE)
			war.push_back(piece);

	// считаем время окончания боя со знаком победителя
	double battle_to_go = estimateWarFront(lane, war);

	// прибавляем время, за которое миньон в среднем дойдёт от фронта до базы противника
	if (battle_to_go > 0)
	{
		WhereAtLine front = getWarSegment(lane, true);
		double time = Max(0.0, ((1 - front.far) * front.L / game->getMinionSpeed()));

		time += TowerLife(lane, false) / battle_to_go;				

		battle_to_go /= time;
	}
	else if (battle_to_go < 0)
	{
		WhereAtLine front = getWarSegment(lane, false);
		double time = Max(0.0, ((front.far - 0) * front.L / game->getMinionSpeed()));

		time += TowerLife(lane, true) / abs(battle_to_go);

		battle_to_go /= time;
	}

	// получили время победы той или иной команды, но со знаком победившей команды.
	return battle_to_go;
}
LaneType chooseWorstLane()
{
	LaneType lanes[3] = { LANE_TOP, LANE_MIDDLE, LANE_BOTTOM };
	double worst = 100000000;
	LaneType ans;

	for (int i = 0; i < 3; ++i)
	{
		double battle_to_go = estimateWarline(lanes[i]);		
		if (battle_to_go < worst)
		{
			worst = battle_to_go;
			ans = lanes[i];
		}
	}

	//cout << "worst line is " << ans << endl;
	return ans;
}
LaneType chooseTowerLane()
{
	LaneType lanes[3] = { LANE_TOP, LANE_MIDDLE, LANE_BOTTOM };
	double near = 100000000;
	LaneType ans;

	for (int i = 0; i < 3; ++i)
	{
		double battle_to_go = getWarSegment(lanes[i], true).far - nearestTower(lanes[i]);
		if (battle_to_go < near)
		{
			near = battle_to_go;
			ans = lanes[i];
		}
	}

	return ans;
}