#include "Deadlock.h"

void CheckVars(MexicanDeadlock& f, vector<int>& var_x, vector<int>& var_y, vector<int>& dist, const World& world)
{
	for (int i = 0; i < (int)price.size(); ++i)
	{
		var_x.push_back(new_x[i]);
		var_y.push_back(new_y[i]);
		dist.push_back(1);
	}

	// Сколько можем сделать шагов
	int moves = HowManyMovesCanDo();

	vector <target> targets;
	//targets.push_back(target(moves, true));
	for (int i = 0; i < (int)f.fighters.size(); ++i)
	{
		if (!f.fighters[i].isTeammate())
		{
			if (world.isVisible(f.fighters[i].getShootingRange(), 
				f.fighters[i].x, f.fighters[i].y, f.fighters[i].stance,
				self.getX(), self.getY(), self.getStance()))
			{  // Составляем список таргетов: выйти из круга противника
				targets.push_back(target(f.fighters[i], moves, true));
				//cerr //<< "Get target " //<< targets.size() //<< ": to go out of radius of enemy on " //<< f.fighters[i].x //<< " " //<< f.fighters[i].y //<< endl;
			}
			if (!world.isVisible(self.getShootingRange(), 
				self.getX(), self.getY(), self.getStance(),
				f.fighters[i].x, f.fighters[i].y, f.fighters[i].stance))
			{  // или затащить его в свой круг
				targets.push_back(target(f.fighters[i], WANT_TO_FIGHT_LIMIT, false));
				//cerr //<< "Get target " //<< targets.size() //<< ": to get to my radius an enemy on " //<< f.fighters[i].x //<< " " //<< f.fighters[i].y //<< endl;
			}
			if (self.isHoldingGrenade() &&
                f.fighters[i].DistanceTo(self.getX(), self.getY()) > game.getGrenadeThrowRange())
			{
				targets.push_back(target(f.fighters[i], moves));
			}
		}
	}
	if (self.isHoldingMedikit() || self.getType() == FIELD_MEDIC)
	{
		for (int i = 0; i < (int)troopers.size(); ++i)
		{
			if (self.getId() != troopers[i].getId() &&
				troopers[i].getHitpoints() < troopers[i].getMaximalHitpoints())
			{
				targets.push_back(target(troopers[i], WANT_TO_CURE_LIMIT));
			}
		}
	}
	if (!self.isHoldingMedikit())
	{
		targets.push_back(target(HowManyMovesCanDo(game.getMedikitUseCost()), MEDIKIT));
	}
	if (!self.isHoldingGrenade())
	{
		targets.push_back(target(HowManyMovesCanDo(game.getGrenadeThrowCost()), GRENADE));
	}
	if (!self.isHoldingFieldRation())
	{
		targets.push_back(target(HowManyMovesCanDo(game.getFieldRationEatCost()), FIELD_RATION));
	}
	
	ClearCurDis();
	TargetDistance(targets, self.getX(), self.getY(), 0, world);
	for (int i = 0; i < (int)targets.size(); ++i)
	{
		for (int h = 0; h < (int)targets[i].var_x.size(); ++h)
		{  // все ответы на таргеты - верные и нужные варианты
			var_x.push_back(targets[i].var_x[h]);
			var_y.push_back(targets[i].var_y[h]);
			dist.push_back(dis[targets[i].var_x[h]][targets[i].var_y[h]]);

			if (targets[i].type == "GO TO RADIUS")
				{
					set <pair<int, int> > answers, WasHere;
					NearHere(answers, WasHere, targets[i].var_x[h], targets[i].var_y[h], targets[i].var_x[h], targets[i].var_y[h], world);
					for (set <pair<int, int> >::iterator it = answers.begin(); it != answers.end(); ++it)
					{
						var_x.push_back(it->first);
						var_y.push_back(it->second);
						dist.push_back(dis[it->first][it->second]);
					}
				}
		}
	}
}
Variant Fight_starter(const World& world)
{
	vector <Fighter> fighters;  // Составляем список файтеров.
	int t = CreateFight(fighters, world);

	//cerr //<< endl //<< endl //<< "Started fight: " //<< fighters[t].name //<< "  -----------" //<< endl;
	MexicanDeadlock f(fighters, t, world);  // Создали битву.
	
	Variant answer(WV_DO_NOTHING, DoNothingVariant(f, world));
	//cerr //<< "WV_DO_NOTHING_variant is " //<< answer.price //<< endl;  // вариант - ничего не делать
	if (IsWaitingForAttack >= (int)troopers.size() || HasToAttack)
	{
		//cerr //<< "WV_DO_NOTHING_variant DELETED! No attack!" //<< endl;
		answer.price = -UNDEFINED;
        HasToAttack = true;
	}
	
	if (f.CanUseFieldRation())
	{
		double result = UseFieldRationVariant(f, world);
		//cerr //<< "Variant to use field ration is " //<< result //<< endl;
		if (result > answer.price)
		{
			answer = Variant(WV_FIELD_RATION, result);
		}
	}

	bool CanShoot = false;
	if (f.CanShoot())
	{
		for (int i = 0; i < (int)f.fighters.size(); ++i)
		{		// ищем цели, по которым можно стрелять	и рассматриваем его
			if (f.fighters[i].new_hitpoints > 0 &&
				!f.fighters[i].isTeammate() && f.IsVisible[t][i])
			{
				CanShoot = true;
				double result = ShootVariant(i, f, world);
				//cerr //<< "Variant to shoot " //<< f.fighters[i].name //<< " is " //<< result //<< endl;
				if (result > answer.price)
				{
					answer = Variant(WV_SHOOT, f.fighters[i].x, f.fighters[i].y, result);
				}
			}
		}
	}
	else
		CanShoot = true;

	if (f.CanGo())
	{
		vector<int> var_x, var_y, dist;
		CheckVars(f, var_x, var_y, dist, world);
		for (int i = 0; i < (int)var_x.size(); ++i)
		{  // рассматриваем вариант с походом.
			if (var_x[i] != self.getX() || var_y[i] != self.getY())
			{
				double result = GoVariant(var_x[i], var_y[i], dist[i], f, world);
				//cerr //<< "Variant to go to (" //<< var_x[i] //<< ", " //<< var_y[i] //<< ") is " //<< result //<< endl;
				if (result > answer.price)
				{
					answer.update(WV_GO, var_x[i], var_y[i], result);
				}
			}
		}
	}

	if (f.CanCure())
	{
		for (int i = 0; i < (int)f.fighters.size(); ++i)
		{		// ищем цели, которых можно лечить умением медика
			if (f.DoesNeedCure(i))
			{
				double result = CureVariant(i, f, world);
				//cerr //<< "Variant to cure " //<< f.fighters[i].name //<< " is " //<< result //<< endl;
				if (result > answer.price)
				{
					answer = Variant(WV_CURE, f.fighters[i].x, f.fighters[i].y, result);
				}
			}
		}
	}

	if (f.CanUseMedikit())
	{
		for (int i = 0; i < (int)f.fighters.size(); ++i)
		{		// ищем цели, которых можно лечить
			if (f.DoesNeedCure(i))
			{
				double result = UseMedikitVariant(i, f, world);
				//cerr //<< "Variant to use medikit on " //<< f.fighters[i].name //<< " is " //<< result //<< endl;
				if (result > answer.price)
				{
					answer = Variant(WV_MEDIKIT, f.fighters[i].x, f.fighters[i].y, result);
				}
			}
		}
	}
	if (f.CanUseGrenade())
	{
		set< pair<int, int> > coord;
		if (f.GrenadeVariants(coord))
		{
			for (set< pair<int, int> >::iterator it = coord.begin(); it != coord.end(); ++it)
			{
				double result = UseGrenadeVariant(it->first, it->second, f, world);
				//cerr //<< "Variant to use greenade on (" //<< it->first //<< ", " //<< it->second 
				//<< ") is " //<< result //<< endl;
					
				if (result > answer.price)
				{
					answer = Variant(WV_GRENADE, it->first, it->second, result);
				}
			}
		}
	}
	
	if (f.CanStand())
	{
		double result = StandVariant(f, world);
		//cerr //<< "Variant to stand is " //<< result //<< endl;
		if (result > answer.price)
		{
			answer = Variant(WV_STAND, result);
		}
	}
	if (f.CanKneel())
	{
		double result = KneelVariant(f, world);
		//cerr //<< "Variant to kneel is " //<< result //<< endl;
		if (result > answer.price)
		{
			answer = Variant(WV_KNEEL, result);
		}
	}
	if (f.CanProne())
	{
		double result = ProneVariant(f, world);
		//cerr //<< "Variant to prone is " //<< result //<< endl;
		if (result > answer.price)
		{
			answer = Variant(WV_PRONE, result);
		}
	}

	return answer;
}
