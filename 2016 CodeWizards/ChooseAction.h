#include "News.h"

bool ProcessBattle(WarWizard::MissileTargetStrategy strat)
{
	me->strategy = strat;
	Battle battle(getArea(me->x, me->y));

	for (int i = 0; i < 30; ++i)
		battle.NextTick();
	for (int i = 0; i < 30; ++i)
		battle.NextTick(3);
	for (int i = 0; i < 15; ++i)
		battle.NextTick(5);
	for (int i = 0; i < 10; ++i)
		battle.NextTick(10);

	for (WarWizard w : battle._wizards)
	{
		drawArrow(wizards[w.id]->x, wizards[w.id]->y, w.x, w.y, 0x00FF00);
		if (w.life <= 0)
		{
			drawCross(wizards[w.id]->x, wizards[w.id]->y, game->getWizardRadius(), 0xFF0000);
		}
		else
		{
			//debug.text(w.x, w.y, to_string(w.life).c_str());
		}
	}
	for (WarMinion m : battle._minions)
	{
		drawArrow(minions[m.id]->x, minions[m.id]->y, m.x, m.y, 0x00FF00);
		if (m.life <= 0)
		{
			drawCross(minions[m.id]->x, minions[m.id]->y, game->getMinionRadius(), 0xFF0000);
		}
		else
		{
			//debug.text(m.x, m.y, to_string(m.life).c_str());
		}
	}
	for (WarTower t : battle._towers)
	{
		if (t.life <= 0)
		{
			drawCross(towers[t.id]->x, towers [t.id]->y, game->getGuardianTowerRadius(), 0xFF0000);
		}
		else
		{
			//debug.text(t.x, t.y, to_string(t.life).c_str());
		}
	}

	for (WarPiece * piece : pieces)
	{
		if (battle.when_die.count(piece->id))
			piece->estimated_death_time = battle.when_die[piece->id] + 1;
		else
			piece->estimated_death_time = NEVER;
	}

	return battle.isDead();
}

WarWizard::MissileTargetStrategy last_strategy = WarWizard::NEAREST;
int last_strategy_update_tick = NEVER;
double last_strategy_bonus(WarWizard::MissileTargetStrategy strategy)
{
	return (strategy == last_strategy && last_strategy_update_tick == tick - 1 && last_strategy != WarWizard::RUN) ? 50 : 0;
}
double health_bonus(double health)
{
	return sqrt(Max(0.0, health) * me->maxLife);
}
double death_penalty(int death_tick, int max_time)
{
	int time = death_tick - tick;
	return death_tick != 0 ? max_time - sqrt(time * max_time) + 100 + sqrt(max_time) : 0;
	//return death_tick != 0 ? 1000.0 / pow(time, 0.4) : 0;
}
double estimate(Battle & battle, int max_time)
{
	bool dead = battle.isDead();
	int result = battle.winner();
	if (result == 0)
	{
		double est =
			3 * estimateWarFront(me->lane(), battle.war) +
			battle.xp + 0.1 * battle.points +
			1.3 * health_bonus(battle.myHealth()) -
			2 * death_penalty(battle.when_die[me->id], max_time);

		if (game->isRawMessagesEnabled())
		{
			est =
				3 * estimateWarFront(me->lane(), battle.war) +
				(self->getXp() < 750 ? battle.xp : 0) + battle.points +
				0.1 * health_bonus(battle.myHealth()) -
				2 * death_penalty(battle.when_die[me->id], max_time) -
				0.3 * battle.enemy_xp;
		}

		//string text = me->StrategyName[me->strategy] + ") " + to_string(est);// to_string(estimateWarFront(me->lane(), true, battle.war) + " + " + battle.xp + " + " + health_bonus(battle.myHealth()) + " - " + death_penalty(battle.when_die[me->id], 330) + " = " + est;

		return est;
	}
	else if (result == -1)
	{
		return -1000000.0 + (battle.exceeded == NEVER ? max_time : battle.exceeded - tick);
	}
	else if (!dead)
	{
		return 1000000.0 - (battle.exceeded == NEVER ? max_time : battle.exceeded - tick);
	}
	return 1000000.0 - 10000;
}
void Fight()
{
	double best = -INFINITY;
	WarWizard::MissileTargetStrategy ans = WarWizard::NEAREST;
	me->missile_target = NULL;
	me->staff_target = NULL;
	AREA A = getArea(me->x, me->y);

	Point retreat = WhereToRun(A).make_module(ESCAPE_DISTANCE) + me->pos();
	vector <Obstacle> obstacles = clear(getObstacles(self->getRadius()), me->pos(), retreat, 0, 0);
	retreat_path = BuildPath(Segment(me->pos(), retreat), obstacles, 15);

	for (int i = 0; i < NUM_OF_STRATEGIES; ++i)
	{
		me->strategy = me->strategies[i];

		//debug.text(me->x, me->y + 40 + i * 20, me->StrategyName[me->strategy].c_str());

		Battle battle(A);

		for (int i = 0; i < 30; ++i)
			battle.NextTick();
		for (int i = 0; i < 30; ++i)
			battle.NextTick(3);

		double est120 = estimate(battle, 120);
		//debug.text(me->x + 150, me->y + 40 + i * 20, to_string((int)est120).c_str());

		for (int i = 0; i < 15; ++i)
			battle.NextTick(5);

		double est195 = estimate(battle, 195);
		//debug.text(me->x + 200, me->y + 40 + i * 20, to_string((int)est195).c_str());

		for (int i = 0; i < 10; ++i)
			battle.NextTick(10);

		double est295 = estimate(battle, 300);
		//debug.text(me->x + 250, me->y + 40 + i * 20, to_string((int)est295).c_str());

		double est = 0.5 * est120 + 0.3 * est195 + 0.2 * est295 + last_strategy_bonus(me->strategies[i]);
		//est = est195 + last_strategy_bonus(me->strategies[i]);
		//debug.text(me->x + 420, me->y + 40 + i * 20, to_string((int)est).c_str());

		if (est > best)
		{
			best = est;
			ans = me->strategies[i];
		}
	}

	me->strategy = ans;
	bool run = ans == WarWizard::RUN;

	last_strategy = ans;
	last_strategy_update_tick = tick;
	ProcessBattle(ans);

	Beat();
	Shoot();
	if (!run && me->missile_target == NULL && me->staff_target == NULL)
		MoveFurther();

	if (run)
		Run(getArea(me->x, me->y));
}

LaneType changeLine;
int wizardDomination(AREA A)
{
	int ans = 0;
	for (auto it : wizards)
		if (it.second->status != UNKNOWN && it.second->status != DEAD && it.second->status != SIT_HOME &&
			it.second->isInArea(A))
			ans += it.second->isFriend ? 1 : -1;
	return ans;
}
int myWizards(AREA A)
{
	int ans = 0;
	for (auto it : wizards)
		if (it.second->status != UNKNOWN && it.second->status != DEAD && it.second->status != SIT_HOME &&
			it.second->isInArea(A))
			ans += it.second->isFriend;
	return ans;
}
bool isEverythingBad(AREA A)
{
	return myWizards(A) == 0 ||
		wizardDomination(A) < -1 ||
		(wizardDomination(A) == -1 && myWizards(A) == 1);
}
bool canMakeWorser(AREA A)
{
	if (A == MY_HOME || A == ENEMY_HOME)
		return false;

	return myWizards(A) > 1 &&
		wizardDomination(A) >= 0 &&
		(wizardDomination(A) != 0 || myWizards(A) > 2);
}
bool checkIfINeedToChangeLine()
{
	if (!game->isRawMessagesEnabled()) return false;

	AREA areas[3] = { TOP_LANE, BOTTOM_LANE, MIDDLE_LANE };
	for (int i = 0; i < 3; ++i)
	{
		if (isEverythingBad(areas[i]) && canMakeWorser(getArea(me->x, me->y)))
		{
			Point P = getWarSegment(getLane(areas[i]), false).point();

			double my_dist = me->getDist2To(P);
			int ans = 0;
			for (auto it : wizards)
				if (it.second->isFriend && it.second->status != DEAD &&	canMakeWorser(getArea(it.second->x, it.second->y)) &&
					it.second->getDist2To(P) < my_dist)
					my_dist = 0;

			if (my_dist != 0)
			{
				changeLine = getLane(areas[i]);
				return true;
			}
		}
	}

	return false;
}

LaneType whereToGo()
{
	if (tick == 0)
	{
		if (self->isMaster() && !game->isRawMessagesEnabled())  // пытаемся что-то слать
		{
			vector<Message> mes = { Message(LANE_TOP, SKILL_ADVANCED_MAGIC_MISSILE, vector<signed char>()),
				Message(LANE_MIDDLE, SKILL_ADVANCED_MAGIC_MISSILE, vector<signed char>()),
				Message(LANE_MIDDLE, SKILL_ADVANCED_MAGIC_MISSILE, vector<signed char>()),
				Message(LANE_BOTTOM, SKILL_ADVANCED_MAGIC_MISSILE, vector<signed char>()) };

			me->move.setMessages(mes);
		}
	}

	vector<Message> mes = self->getMessages();
	if (tick > 400)
	{
		if (game->isRawMessagesEnabled())
		{
			if (isEverythingBad(MIDDLE_LANE))
				return LANE_MIDDLE;
			if (isEverythingBad(BOTTOM_LANE))
				return LANE_BOTTOM;
			if (isEverythingBad(TOP_LANE))
				return LANE_TOP;
			return chooseWorstLane();
		}
		return chooseTowerLane();
	}
	else
	{
		if (mes.size() > 0)
		{
			//cout << tick << ") Got order! Go to " << mes[0].getLane() << endl;
			return mes[0].getLane();
		}

		switch (self->getId())
		{
		case 1: return LANE_TOP;
		case 2: return LANE_TOP;
		case 3: return LANE_MIDDLE;
		case 4: return LANE_MIDDLE;
		case 5: return LANE_MIDDLE;
		case 10: return LANE_TOP;
		case 9: return LANE_TOP;
		case 8: return LANE_MIDDLE;
		case 7: return LANE_MIDDLE;
		case 6: return LANE_MIDDLE;
		}
	}

	return LANE_MIDDLE;
}
void chooseAction()
{
	if (me->status == SIT_HOME)
	{
		LaneType where_to_go = whereToGo();

		GoIntoBattle(where_to_go);
	}
	else if (me->status == GO_INTO_BATTLE)
	{
		GoIntoBattle(me->lane());
	}
	else if (me->status == BATTLE)
	{
		/*if (checkIfINeedToChangeLine())
		{
			Point P = getWarSegment(changeLine, true).point();
			if (!GetToPoint(P, 30, true))
				Fight();
		}
		else*/
			Fight();
	}

	// уворачиваемся от ракет
	vector<WarWizard *> enemies;
	for (auto it : wizards)
		if (!it.second->isFriend && it.second->isInBattle(getArea(me->x, me->y)))
			enemies.push_back(it.second);
	HateRockets(enemies);
}
