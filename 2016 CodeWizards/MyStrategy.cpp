#include "ChooseAction.h"
#include "time.h"

void Init()
{
	bool first = true;
	vector <Wizard> w = world->getWizards();
	for (int i = 0; i < (int)w.size(); ++i)
	{
		int ID = w[i].getId();
		wizards[ID] = new WarWizard(&(w[i]));
		pieces.push_back(wizards[ID]);
		if (ID > 5) first = false;
	}
	me = wizards[self->getId()];

	for (int i = 0; i < 5; ++i)  // инициализируем вражеских волшебников раз и навсегда
	{
		int ID = i + 1 + (first ? 5 : 0);
		wizards[ID] = new WarWizard(ID);
		pieces.push_back(wizards[ID]);
	}

	map<int, int> conformity = { {11, 12}, {12, 11},  // базы
								 {13, 14}, {14, 13},  // первая мид
								 {15, 16}, {16, 15},  // вторая мид
								 {17, 23}, {23, 17},  // первая топ
								 {18, 24}, {24, 18},  // вторая топ
								 {19, 21}, {21, 19},  // первая низ
								 {20, 22}, {22, 20} };// вторая низ
	set<int> second_towers = { 15, 16, 18, 24, 20, 22 };

	vector <Building> b = world->getBuildings();
	for (int i = 0; i < (int)b.size(); ++i)
	{
		int ID = b[i].getId();
		towers[ID] = new WarTower(&b[i], second_towers.count(ID));  // сразу две башни
		////cout << ID << ", " << towers[ID]->x << ", " << towers[ID]->y << endl;
		towers[conformity[ID]] = new WarTower(conformity[ID], towers[ID]);
		pieces.push_back(towers[ID]);
		pieces.push_back(towers[conformity[ID]]);
	}

	towers[15]->protect = me->id <= 5 ? TOP_LANE : BOTTOM_LANE;
	towers[16]->protect = me->id > 5 ? TOP_LANE : BOTTOM_LANE;
	towers[22]->protect = MIDDLE_LANE;
	towers[20]->protect = MIDDLE_LANE;
}

void draw()
{	
	for (auto it = wizards.begin(); it != wizards.end(); ++it)
	{
		if (it->second->status != DEAD)
		{
			WhereAtLine WL(it->second->x, it->second->y);
			Point P = WL.point();
			//debug.fillCircle(P.x, P.y, 10);
			//debug.text(P.x, P.y, to_string(WL.far).c_str());
			//debug.text(it->second->x, it->second->y - 40, me->StrategyName[it->second->strategy].c_str());
		}
	}
	for (auto it = minions.begin(); it != minions.end(); ++it)
	{
		Point P = WhereAtLine(it->second->x, it->second->y).point();
		//debug.fillCircle(P.x, P.y, 10, 0x00FF00);
	}

	//debug.text(200, 3200, to_string(estimateWarline(LANE_TOP)).c_str());
	//debug.text(800, 3200, to_string(estimateWarline(LANE_MIDDLE)).c_str());
	//debug.text(800, 3800, to_string(estimateWarline(LANE_BOTTOM)).c_str());
	
	//debug.endPost();
}

bool bonus1 = false;
bool bonus2 = false;
void bonusCostyle()
{
	for (int t = tick - DEAD; t <= tick; ++t)
	{
		if (tick > 0 && tick % game->getBonusAppearanceIntervalTicks() == 0)
		{
			bonus1 = true;
			bonus2 = true;
		}
	}

	if (isPointVisible(Point(1200, 1200)))
	{
		bonus1 = false;
	}
	if (isPointVisible(Point(2800, 2800)))
	{
		bonus2 = false;
	}

	for (int i = 0; i < (int)world->getBonuses().size(); ++i)
	{
		Bonus B = world->getBonuses()[i];
		if (B.getX() == 1200)
			bonus1 = true;
		else
			bonus2 = true;
	}

	if (bonus1 && me->getDist2To(Point(1200, 1200)) < 1000 * 1000)
	{
		double best_time = INFINITY;
		WarWizard * ans = NULL;
		for (auto ww : wizards)
		{
			WarWizard * wiz = ww.second;
			if (wiz->isValid())
			{
				int time = sqrt(wiz->getDist2To(Point(1200, 1200))) / wiz->getAverageSpeed();
				if (time < best_time)
				{
					best_time = time;
					ans = wiz;
				}
			}
		}

		if (ans == me)
			GetToPoint(Point(1200, 1200), (me->sum_xp > 750 && game->isRawMessagesEnabled()) ? 1 : 5, true);
	}
	if (bonus2 && me->getDist2To(Point(2800, 2800)) < 1000 * 1000)
	{
		double best_time = INFINITY;
		WarWizard * ans = NULL;
		for (auto ww : wizards)
		{
			WarWizard * wiz = ww.second;
			if (wiz->isValid())
			{
				int time = sqrt(wiz->getDist2To(Point(2800, 2800))) / wiz->getAverageSpeed();
				if (time < best_time)
				{
					best_time = time;
					ans = wiz;
				}
			}
		}

		if (ans == me)
			GetToPoint(Point(2800, 2800), (me->sum_xp > 750 && game->isRawMessagesEnabled()) ? 1 : 5, true);
	}
}

bool I_AM_FIRE = false;
void learn(Move & move)
{
	I_AM_FIRE = I_AM_FIRE || (self->getSkills().size() == 0 && UsesOfStaff >= 3);

	if (I_AM_FIRE)
	{
		switch (self->getSkills().size())
		{
		case 0: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_PASSIVE_1); break;
		case 1: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_AURA_1); break;
		case 2: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_PASSIVE_2); break;
		case 3: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_AURA_2); break;
		case 4: move.setSkillToLearn(SKILL_FIREBALL); break;
		case 5: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_1); break;
		case 6: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_AURA_1); break;
		case 7: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_2); break;
		case 8: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_AURA_2); break;
		case 9: move.setSkillToLearn(SKILL_FROST_BOLT); break;
		case 10: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_1); break;
		case 11: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_1); break;
		case 12: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_1); break;
		case 13: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_AURA_1); break;
		case 14: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_2); break;
		case 15: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_2); break;
		case 16: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_2); break;
		case 17: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_AURA_2); break;
		}
	}
	else
	{
		switch (self->getSkills().size())
		{
		case 0: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_1); break;
		case 1: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_AURA_1); break;
		case 2: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_2); break;
		case 3: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_AURA_2); break;
		case 4: move.setSkillToLearn(SKILL_FROST_BOLT); break;
		case 5: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_PASSIVE_1); break;
		case 6: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_AURA_1); break;
		case 7: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_PASSIVE_2); break;
		case 8: move.setSkillToLearn(SKILL_STAFF_DAMAGE_BONUS_AURA_2); break;
		case 9: move.setSkillToLearn(SKILL_FIREBALL); break;
		case 10: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_1); break;
		case 11: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_1); break;
		case 12: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_1); break;
		case 13: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_AURA_1); break;
		case 14: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_2); break;
		case 15: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_2); break;
		case 16: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_2); break;
		case 17: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_AURA_2); break;
		}
	}




	/*switch (self->getSkills().size())
	{
	case 0: move.setSkillToLearn(SKILL_RANGE_BONUS_PASSIVE_1); break;
	case 1: move.setSkillToLearn(SKILL_RANGE_BONUS_AURA_1); break;
	case 2: move.setSkillToLearn(SKILL_RANGE_BONUS_PASSIVE_2); break;
	case 3: move.setSkillToLearn(SKILL_RANGE_BONUS_AURA_2); break;
	case 4: move.setSkillToLearn(SKILL_ADVANCED_MAGIC_MISSILE); break;
	case 5: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_1); break;
	case 6: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_AURA_1); break;
	case 7: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_2); break;
	case 8: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_BONUS_AURA_2); break;
	case 9: move.setSkillToLearn(SKILL_FROST_BOLT); break;
	case 10: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_1); break;
	case 11: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_1); break;
	case 12: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_1); break;
	case 13: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_AURA_1); break;
	case 14: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_2); break;
	case 15: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_2); break;
	case 16: move.setSkillToLearn(SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_2); break;
	case 17: move.setSkillToLearn(SKILL_MOVEMENT_BONUS_FACTOR_AURA_2); break;
	}*/
}

void MyStrategy::move(const Wizard& _self, const World& _world, const Game& _game, Move& move)
{
	//clock_t start = clock();

	//debug.beginPost();

	if (tick != 0)
	{
		delete self;
		delete world;
		delete game;
	}
	self = new Wizard(_self);
	world = new World(_world);
	game = new Game(_game);
	WAS_DEAD = world->getTickIndex() - 1 - tick;
	tick = world->getTickIndex();
	if (WAS_DEAD >= game->getWizardMinResurrectionDelayTicks())
		me->status = DEAD;

	//////cout << tick << endl;

	if (tick == 0)
		Init();
	
	getNews();  // обновляем данные о волшебниках и крипах
	chooseAction();  // выбираем действие
	bonusCostyle();

	draw();

	move = me->move;  // записываем результат

	learn(move);

	//clock_t finish = clock();
	//float diff = ((float)finish - start) / CLOCKS_PER_SEC;
	////////////////cout << diff << endl;
}

MyStrategy::MyStrategy() { }
