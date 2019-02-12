#include "TroopersCheck.h"

enum WarVariant
{
	WV_DO_NOTHING = -1,
	WV_SHOOT = 0,
	WV_GO = 1,
	WV_CURE = 2,
	WV_MEDIKIT = 3,
	WV_FIELD_RATION = 4,
	WV_GRENADE = 5,
	WV_KNEEL = 6,
	WV_PRONE = 7,
	WV_STAND = 8,
	WV_RUN = 111
};
class Variant  // Выбранный вариант хода
{
public:
	WarVariant type;
	int x, y;
	int index;
	vector<int> bv_1, bv_2;
	double price;

	Variant()
	{
		type = WV_DO_NOTHING;
		price = 0;
	}
	Variant(WarVariant t, double price1)
	{
		type = t;
		price = price1;
	}
	Variant(WarVariant t, int i, double price1)
	{
		type = t;
		index = i;
		price = price1;
	}
	Variant(WarVariant t, int b1, int b2, double price1)
	{
		type = t;
		x = b1;
		y = b2;
		price = price1;
	}
	Variant(WarVariant t, int b1, int b2, int dist, double price1)
	{
		type = t;
		x = b1;
		y = b2;
		index = dist;
		price = price1;
	}

	void update(WarVariant t, int b1, int b2, double price1)
	{
		if (type != t || price1 > price)
		{
			bv_1.clear();
			bv_2.clear();
		}

		type = t;
		bv_1.push_back(b1);
		bv_2.push_back(b2);
		price = price1;
	}
};

const double LEFT_HITPOINTS_K = 0.01;
class MexicanDeadlock
{
public:
	vector<Fighter> fighters; // Перестрелка! Бабах! Бабах!
	int t;  // Текущий файтер, у которого ход.
	double points, enemy_points;  // Итоговые очки.
	double Now;  // Текущая ситуация для игрока.
	double TimeCoeff;
	static const int ROUNDS = 4;
    int MyStartHealth;

	vector< vector <bool> > IsVisible;
	vector< vector <bool> > IsSeenOnMap;
	map<int, map<int, bool> > IsPlayerSeen;  // Заметим, что ключ - это id игрока.
	/* Здесь мы считаем, что за один ход вполне можно уменьшить или увеличить
	это расстояние на один, идя по большему катету. В целом эту теорию подтверждают константные масивы.
	*/

    set< pair<int, int> > RealTroopersCoord;

	MexicanDeadlock(vector<Fighter> tr, int T, const World& world)
	{
		fighters = tr;		
		t = T;

		points = 0;
		enemy_points = 0;
		TimeCoeff = 1;
        RealTroopersCoord = TroopersCoord;

		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			IsVisible.push_back(vector<bool>());
			IsSeenOnMap.push_back(vector<bool>());
			for (int j = 0; j < (int)fighters.size(); ++j)
			{
				IsVisible[i].push_back(true);
				IsSeenOnMap[i].push_back(true);
			}
		}

		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			for (int j = 0; j < (int)fighters.size(); ++j)
			{
				IsVisible[i][j] = world.isVisible(fighters[i].getShootingRange(), fighters[i].x, fighters[i].y, fighters[i].stance, fighters[j].x, fighters[j].y, fighters[j].stance);
				IsVisible[j][i] = world.isVisible(fighters[j].getShootingRange(), fighters[j].x, fighters[j].y, fighters[j].stance, fighters[i].x, fighters[i].y, fighters[i].stance);

				IsSeenOnMap[i][j] = world.isVisible(fighters[i].getVisionRange(), fighters[i].x, fighters[i].y, fighters[i].stance, fighters[j].x, fighters[j].y, fighters[j].stance);
				IsSeenOnMap[j][i] = world.isVisible(fighters[j].getVisionRange(), fighters[j].x, fighters[j].y, fighters[j].stance, fighters[i].x, fighters[i].y, fighters[i].stance);
			}
		}

		UpdateVisability(world);

        MyStartHealth = 0;
        for (int i = 0; i < (int)fighters.size(); ++i)
        {
            if (fighters[i].isTeammate() && fighters[i].new_hitpoints > 0)
            {
                MyStartHealth += fighters[i].new_hitpoints;
            }
			IsPlayerSeen[self.getPlayerId()][fighters[i].getPlayerId()] = true;
        }
	}
	void UpdateVisability(const World& world)
	{
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			IsPlayerSeen[fighters[i].getPlayerId()][t] = false;
            IsPlayerSeen[fighters[t].getPlayerId()][i] = false;
		}

		ShootingVisabilityUpdate(world);
	}
	void ShootingVisabilityUpdate(const World& world)
	{
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			IsVisible[i][t] = world.isVisible(fighters[i].getShootingRange(), fighters[i].x, fighters[i].y, fighters[i].stance, fighters[t].x, fighters[t].y, fighters[t].stance);
            IsVisible[t][i] = world.isVisible(fighters[t].getShootingRange(), fighters[t].x, fighters[t].y, fighters[t].stance, fighters[i].x, fighters[i].y, fighters[i].stance);

			IsSeenOnMap[i][t] = world.isVisible(fighters[i].getVisionRange(), fighters[i].x, fighters[i].y, fighters[i].stance, fighters[t].x, fighters[t].y, fighters[t].stance);
            IsSeenOnMap[t][i] = world.isVisible(fighters[t].getVisionRange(), fighters[t].x, fighters[t].y, fighters[t].stance, fighters[i].x, fighters[i].y, fighters[i].stance);
		}

		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			for (int j = 0; j < (int)fighters.size(); ++j)
			{
                if (IsSeenOnMap[i][j] && fighters[i].new_hitpoints > 0)
				    IsPlayerSeen[fighters[i].getPlayerId()][j] = true;
				if (IsSeenOnMap[j][i] && fighters[j].new_hitpoints > 0)
				    IsPlayerSeen[fighters[j].getPlayerId()][i] = true;
			}
		}
	}

	bool DoesSee(int i)
	{
		return IsPlayerSeen[fighters[t].getPlayerId()][i];
	}
	bool DoesSee(int i, int j)
	{
		return IsPlayerSeen[fighters[i].getPlayerId()][j];
	}
	bool AreEnemies(int i)
	{
		return fighters[i].getPlayerId() != fighters[t].getPlayerId();
	}
	bool AreEnemies(int i, int j)
	{
		return fighters[i].getPlayerId() != fighters[j].getPlayerId();
	}
	double IfIsShot(int i)
	{
		double damage = fighters[t].CurrentDanger();
		if (damage >= fighters[i].new_hitpoints)
		{
			damage = game.getTrooperEliminationScore() + fighters[i].new_hitpoints +
				fighters[t].getDamage() * 
				(fighters[t].NumOfShoots() - (fighters[i].new_hitpoints + fighters[t].getDamage() - 1) / fighters[t].getDamage());
		}
		return damage;
	}
	void Damage(int i, int dam)
	{
		int _points = dam;
		if (fighters[i].new_hitpoints <= _points)
		{  // Бонус за элиминацию
			_points = game.getTrooperEliminationScore() + fighters[i].new_hitpoints;
			fighters[i].new_hitpoints = 0;
		}
		else
		{
			fighters[i].new_hitpoints -= _points;  // Здоровье
		}

		//cerr //<< "+" //<< _points * TimeCoeff //<< ", " //<< fighters[i].new_hitpoints //<< "hp is left!" //<< endl;
		
		if (fighters[t].isTeammate() && !fighters[i].isTeammate())  // и изменяем результаты
			points += _points * TimeCoeff;
		else
			enemy_points += _points * TimeCoeff;

		if (fighters[i].isTeammate() && fighters[i].new_hitpoints <= 0)
		{
			//cerr //<< "    My man's dead. +" //<< 50 * TimeCoeff //<< " to enemies!" //<< endl;
			enemy_points += 50 * TimeCoeff;
		}
	}
	void Shoot(int i)
	{
		int dam = fighters[t].getDamage();  // Очки
		//cerr //<< "    " //<< fighters[t].name //<< " shoot " //<< fighters[i].name //<< ", ";
		Damage(i, dam);
		fighters[t].new_action_points -= fighters[t].getShootCost();  // оставшиеся очки		 
	}
	void NextTurn(const World& world)  // Передаём ход следующему
	{
		fighters[t].WasGoing = false;
		UpdateVisability(world);

		do
		{
			t = (t + 1) % (int)fighters.size();
			TimeCoeff -= 1.0 / ((double)ROUNDS * (int)fighters.size());
		}
		while (fighters[t].new_hitpoints <= 0);

		// Даём ему начальные экшнпоинты
		fighters[t].new_action_points = fighters[t].getInitialActionPoints();
		if ((Opponents[fighters[t].getPlayerId()].fighters[COMMANDER].new_hitpoints > 0 || 
			Opponents[fighters[t].getPlayerId()].fighters[COMMANDER].status == "unknown") &&
			fighters[t].getType() != COMMANDER && fighters[t].getType() != SCOUT)
		{
			fighters[t].new_action_points += game.getCommanderAuraBonusActionPoints();
		}
	}
	bool CheckFight(const World& world)
	{
		if (TimeCoeff <= 1.0 / UNDEFINED)
			return false;

		// Драки нет, если нет живых врагов или живых друзей;
		bool no_me = true, no_enemies = true;
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (fighters[i].new_hitpoints > 0)
			{
				if (fighters[i].isTeammate())
					no_me = false;
				else
					no_enemies = false;
			}
		}

		return (!no_me) && (!no_enemies); 
	}
	bool CanShoot()
	{
		return fighters[t].new_action_points >= fighters[t].getShootCost();
	}
	bool CanGo()
	{
		return fighters[t].new_action_points >= MoveCost(fighters[t]) &&
			!fighters[t].WasGoing;
	}
	bool CanCure()
	{
		return fighters[t].getType() == FIELD_MEDIC &&
			fighters[t].new_action_points >= game.getFieldMedicHealCost();
	}
	bool DoesNeedCure(int i)
	{
		return (!AreEnemies(i)) &&
			fighters[i].new_hitpoints < fighters[i].getMaximalHitpoints() &&
			abs(fighters[t].x - fighters[i].x) + abs(fighters[t].y - fighters[i].y) <= 1;
	}
	bool CanUseMedikit()
	{
		return fighters[t].has_medikit &&
			fighters[t].new_action_points >= game.getMedikitUseCost();
	}
	bool CanUseGrenade()
	{
		return fighters[t].has_grenade &&
			fighters[t].new_action_points >= game.getGrenadeThrowCost();
	}
	bool CanUseFieldRation()
	{
		return fighters[t].has_field_ration &&
			fighters[t].new_action_points >= game.getFieldRationEatCost() &&
			fighters[t].new_action_points 
			- game.getFieldRationEatCost() + game.getFieldRationBonusActionPoints() <=
			fighters[t].getInitialActionPoints();
	}
	bool GrenadeVariants(set< pair<int, int> >& coord)
	{
		bool ans = false;
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (AreEnemies(i) && DoesSee(i) && fighters[i].new_hitpoints > 0)
			{
				////cerr //<< "       (" //<< fighters[t].name //<< " dist to " //<< fighters[i].name 
					////<< " = " //<< fighters[t].DistanceTo(fighters[i].x, fighters[i].y) //<< endl;

				if (fighters[t].DistanceTo(fighters[i]) <= game.getGrenadeThrowRange())
				{
					ans = true;
					coord.insert(make_pair(fighters[i].x, fighters[i].y));
				}
				if (fighters[t].DistanceTo(fighters[i].x + 1, fighters[i].y) <= game.getGrenadeThrowRange())
				{
					ans = true;
					coord.insert(make_pair(fighters[i].x + 1, fighters[i].y));
				}
				if (fighters[t].DistanceTo(fighters[i].x - 1, fighters[i].y) <= game.getGrenadeThrowRange())
				{
					ans = true;
					coord.insert(make_pair(fighters[i].x - 1, fighters[i].y));
				}
				if (fighters[t].DistanceTo(fighters[i].x, fighters[i].y + 1) <= game.getGrenadeThrowRange())
				{
					ans = true;
					coord.insert(make_pair(fighters[i].x, fighters[i].y + 1));
				}
				if (fighters[t].DistanceTo(fighters[i].x, fighters[i].y - 1) <= game.getGrenadeThrowRange())
				{
					ans = true;
					coord.insert(make_pair(fighters[i].x, fighters[i].y - 1));
				}
			}
		}
		return ans;
	}
	double IfGrenade(int x, int y)
	{
		double ans = 0;
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			int k = abs(fighters[i].x - x) + abs(fighters[i].y - y);
			if (k == 0 && fighters[i].new_hitpoints > 0 && DoesSee(i))
			{
				if (!AreEnemies(i))
				{
					ans -= fighters[i].IfDamaged(game.getGrenadeDirectDamage());
				}
				else
				{
					ans += fighters[i].IfDamaged(game.getGrenadeDirectDamage());
				}
			}
			else if (k == 1 && fighters[i].new_hitpoints > 0 && DoesSee(i))
			{
				if (!AreEnemies(i))
				{
					ans -= fighters[i].IfDamaged(game.getGrenadeCollateralDamage());
				}
				else
				{
					ans += fighters[i].IfDamaged(game.getGrenadeCollateralDamage());
				}
			}
		}
		return ans;
	}
	void Grenade(int x, int y)
	{
		//cerr //<< "    " //<< fighters[t].name //<< " uses GRENADE on (" 
			   //<< x //<< ", " //<< y //<< ")!";

		fighters[t].new_action_points -= game.getGrenadeThrowCost();
		fighters[t].has_grenade = false;

		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (fighters[i].new_hitpoints > 0)
			{
				int k = abs(fighters[i].x - x) + abs(fighters[i].y - y);
				if (k == 0)
				{
					Damage(i, game.getGrenadeDirectDamage());
				}
				else if (k == 1)
				{
					Damage(i, game.getGrenadeCollateralDamage());
				}
			}
		}
	}
	bool CanKneel()
	{
		return fighters[t].stance != KNEELING &&
			fighters[t].new_action_points >= game.getStanceChangeCost();
	}
	bool CanProne()
	{
		return (fighters[t].stance == KNEELING &&
			fighters[t].new_action_points >= game.getStanceChangeCost()) ||
			(fighters[t].stance == STANDING &&
			fighters[t].new_action_points >= 2 * game.getStanceChangeCost());
	}
	bool CanStand()
	{
		return (fighters[t].stance == KNEELING &&
			fighters[t].new_action_points >= game.getStanceChangeCost()) ||
			(fighters[t].stance == PRONE &&
			fighters[t].new_action_points >= 2 * game.getStanceChangeCost());
	}
	int EstimateFieldPos(int nx1, int ny1, const World& world)
	{
		if (!DoExist(nx1, ny1, world))
			return -UNDEFINED;

		int ans = 0;
		bool bonus = true;
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (fighters[i].new_hitpoints > 0)
			{
				if (AreEnemies(i))
				{
					if (fighters[i].DistanceTo(fighters[t]) <= fighters[i].getShootingRange())
						--ans;
					if (fighters[i].DistanceTo(fighters[t]) <= fighters[i].getVisionRange())
						bonus = false;
					if (fighters[t].DistanceTo(fighters[i]) <= fighters[t].getShootingRange())
						++ans;
				}
			}
		}

		if (bonus)
			ans += 20;

		return ans;
	}
	void GoTo(int x, int y, int dist, const World& world)
	{
		fighters[t].WasGoing = true;

		fighters[t].new_action_points -= dist * MoveCost(fighters[t]);
		fighters[t].x = x;
		fighters[t].y = y;
		ShootingVisabilityUpdate(world);
		//cerr //<< "    " //<< fighters[t].name //<< " goes to (" //<< x //<< ", " //<< y //<< ")" //<< endl;
	}
	void Cure(int i)
	{
		fighters[t].new_action_points -= game.getFieldMedicHealCost();
		if (i == t)
			fighters[t].new_hitpoints = min(
			fighters[t].new_hitpoints + game.getFieldMedicHealSelfBonusHitpoints(),
			fighters[t].getMaximalHitpoints());
		else
			fighters[t].new_hitpoints = min(
			fighters[t].new_hitpoints + game.getFieldMedicHealBonusHitpoints(),
			fighters[t].getMaximalHitpoints());
	}
	void UseMedikit(int i)
	{
		fighters[t].new_action_points -= game.getMedikitUseCost();
		fighters[t].has_medikit = false;
		if (i == t)
			fighters[t].new_hitpoints = min(
			fighters[t].new_hitpoints + game.getMedikitHealSelfBonusHitpoints(),
			fighters[t].getMaximalHitpoints());
		else
			fighters[t].new_hitpoints = min(
			fighters[t].new_hitpoints + game.getMedikitBonusHitpoints(),
			fighters[t].getMaximalHitpoints());
	}
	void UseFieldRation()
	{
		fighters[t].has_field_ration = false;
		fighters[t].new_action_points -= game.getFieldRationEatCost();
		fighters[t].new_action_points += game.getFieldRationBonusActionPoints();
	}
	void Kneel(const World& world)
	{
		//cerr //<< "    " //<< fighters[t].name //<< " kneels!" //<< endl;
		fighters[t].new_action_points -= game.getStanceChangeCost();
		fighters[t].stance = KNEELING;
		ShootingVisabilityUpdate(world);
	}
	void Prone(const World& world)
	{
		//cerr //<< "    " //<< fighters[t].name //<< " prones!" //<< endl;
		if (fighters[t].stance == KNEELING)
			fighters[t].new_action_points -= game.getStanceChangeCost();
		else
			fighters[t].new_action_points -= 2 * game.getStanceChangeCost();
	
		fighters[t].stance = PRONE;
		ShootingVisabilityUpdate(world);
	}
	void Stand(const World& world)
	{
		//cerr //<< "    " //<< fighters[t].name //<< " stands!" //<< endl;
		if (fighters[t].stance == KNEELING)
			fighters[t].new_action_points -= game.getStanceChangeCost();
		else
			fighters[t].new_action_points -= 2 * game.getStanceChangeCost();
	
		fighters[t].stance = STANDING;
		ShootingVisabilityUpdate(world);
	}
	
	void CheckShooting(const World& world, Variant& best)
	{
		if (CanShoot())  // Сначала смотрим, кого можно подстрелить.
		{
			for (int i = 0; i < (int)fighters.size(); ++i)
			{			
				if (fighters[i].new_hitpoints > 0 &&
					AreEnemies(i) && IsVisible[t][i] && DoesSee(i))
				{
					double points = IfIsShot(i);
					double danger = fighters[i].Danger() / 100.0;

					double result = points + danger;

					////cerr //<< "       (" //<< fighters[t].name //<< " shoot " //<< fighters[i].name //<< " full summary = " //<< points //<< " + " //<< danger //<< endl;

					if (result > best.price)
					{
						best = Variant(WV_SHOOT, i, result);
					}
				}
				else if (AreEnemies(i) && fighters[i].new_hitpoints > 0)
				{
					if (!IsVisible[t][i])
					{
						////cerr //<< "       (" //<< fighters[t].name //<< " cann't shoot " //<< fighters[i].name //<< " because it's too far or there are tryouts" //<< endl;
					}
					else
					{
						////cerr //<< "       (" //<< fighters[t].name //<< " cann't shoot " //<< fighters[i].name //<< " because he doesn't see him" //<< endl;
					}
				}
			}
		}
	}
	double TryToShoot(const World& world)
	{
		double final_answer = 0;
		int ap = fighters[t].new_action_points;

		vector <int> hps;
		for (int i = 0; i < (int)fighters.size(); ++i)
			hps.push_back(fighters[i].new_hitpoints);

		while (CanShoot())  // Сначала смотрим, кого можно подстрелить.
		{
			double ans = 0;
			int target = -1;
			for (int i = 0; i < (int)fighters.size(); ++i)
			{			
				if (fighters[i].new_hitpoints > 0 && AreEnemies(i)
					&& world.isVisible(fighters[t].getShootingRange(), fighters[t].x, fighters[t].y, fighters[t].stance, fighters[i].x, fighters[i].y, fighters[i].stance)
					&& DoesSee(i))
				{
					double points = fighters[i].IfDamaged(fighters[t].getDamage());
					double danger = fighters[i].Danger() / 100.0;
					fighters[t].new_action_points -= fighters[t].getShootCost();

					double result = points + danger;
					if (result > ans)
					{
						ans = result;
						target = i;
					}
				}
			}

			if (target < 0)
				break;
			final_answer += fighters[target].GetDamaged(fighters[t].getDamage());
		}

		for (int i = 0; i < (int)fighters.size(); ++i)
			fighters[i].new_hitpoints = hps[i];
		fighters[t].new_action_points = ap;

		return final_answer;
	}
	void CheckGoVars(const World& world, vector <target>& targets)
	{
		int moves = fighters[t].new_action_points / MoveCost(fighters[t]);

		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (fighters[i].new_hitpoints > 0 &&
				AreEnemies(i) && !IsVisible[t][i])
			{
				targets.push_back(target(fighters[i].x, fighters[i].y, fighters[i].stance, 
					fighters[t].stance, fighters[t].getShootingRange(), i, moves)); 
			}
            if (fighters[i].new_hitpoints > 0 &&
					AreEnemies(i) && IsVisible[i][t])
			{
                targets.push_back(target(fighters[i].x, fighters[i].y, fighters[i].stance,
                    fighters[i].getShootingRange(), fighters[t].stance, i, moves)); 
            }
			if (CanUseGrenade() && fighters[i].new_hitpoints > 0 &&
				AreEnemies(i) && DoesSee(i) &&
				fighters[t].DistanceTo(fighters[i]) > game.getGrenadeThrowRange())
			{
				targets.push_back(target(fighters[i].x, fighters[i].y, i, moves, true));
			}
			if ((CanCure() || CanUseMedikit())
				&& DoesNeedCure(i) && i != t)
			{
				targets.push_back(target(fighters[i].x, fighters[i].y, i, moves, false));
			}
		}

		ClearCurDis();
		TargetDistance(targets, fighters[t].x, fighters[t].y, 0, world);
	}
	void CheckAttack(const World& world, vector <target>& targets, Variant& best)
	{
		for (int i = 0; i < (int)targets.size(); ++i)
		{
			if (targets[i].type == "GO TO RADIUS" && targets[i].var_x.size() > 0)
			{
				int backup_x = fighters[t].x;
				int backup_y = fighters[t].y;
				int ap = fighters[t].new_action_points;

				fighters[t].new_action_points -= targets[i].limit * MoveCost(fighters[t]);
				fighters[t].x = targets[i].var_x[0];
				fighters[t].y = targets[i].var_y[0];
					
				double points = IfIsShot(targets[i].target_index);
				double danger = fighters[targets[i].target_index].Danger() / 100.0;

				double result = points + danger;
				if (!DoesSee(targets[i].target_index))
				{
					if (fighters[t].WasDoingNothing)
						result /= 4.0;
					else
						result = 0;
				}

				////cerr //<< "       (" //<< fighters[t].name //<< " attack " 
						////<< fighters[targets[i].target_index].name //<< " summary = " //<< result;

				result *= Opponents[fighters[t].getPlayerId()].Attack_predict;

				////cerr //<< " * " //<< Opponents[fighters[t].getPlayerId()].Attack_predict //<< " = " //<< result //<< endl;

				if (result > best.price)
				{
					best = Variant(WV_GO, fighters[t].x, fighters[t].y, targets[i].limit, result);
				}
						
				fighters[t].new_action_points = ap;
				fighters[t].x = backup_x;
				fighters[t].y = backup_y;
			}
		}
	}
	void CheckGoGrenade(const World& world, vector <target>& targets, Variant& best)
	{
		for (int i = 0; i < (int)targets.size(); ++i)
		{
			if (targets[i].type == "GRENADE" && targets[i].var_x.size() > 0)
			{
				int backup_x = fighters[t].x;
				int backup_y = fighters[t].y;
				int ap = fighters[t].new_action_points;

				fighters[t].new_action_points -= targets[i].limit * MoveCost(fighters[t]);
				fighters[t].x = targets[i].var_x[0];
				fighters[t].y = targets[i].var_y[0];
					
				double result = -UNDEFINED;
				double danger = fighters[targets[i].target_index].Danger() / 100.0;
				if (fighters[t].new_action_points >= game.getGrenadeThrowCost())
				{
					result = game.getGrenadeDirectDamage() + danger;
					////cerr //<< "       (" //<< fighters[t].name //<< " attack with grenade " 
							////<< fighters[targets[i].target_index].name //<< " summary = " //<< result //<< endl;
				}

				if (result > best.price)
				{
					best = Variant(WV_GO, fighters[t].x, fighters[t].y, targets[i].limit, result);
				}
						
				fighters[t].new_action_points = ap;
				fighters[t].x = backup_x;
				fighters[t].y = backup_y;
			}
		}
	}
	void CheckDeviation(const World& world, vector <target>& targets, Variant& best)
	{
		for (int i = 0; i < (int)targets.size(); ++i)
		{
			if (targets[i].type == "GO OUT OF RADIUS" && targets[i].var_x.size() > 0)
			{
				double now = TryToShoot(world);

				int _t = t;
                t = targets[i].target_index;
				now -= TryToShoot(world);
				t = _t;

				int backup_x = fighters[t].x;
				int backup_y = fighters[t].y;
				int ap = fighters[t].new_action_points;

				fighters[t].new_action_points -= targets[i].limit * MoveCost(fighters[t]);
                fighters[t].x = targets[i].var_x[0];
                fighters[t].y = targets[i].var_y[0];
					
				double Then = TryToShoot(world);
				_t = t;
				t = targets[i].target_index;
				Then -= TryToShoot(world);
				t = _t;

				double points = (Then - now) / 2;

				////cerr //<< "       (" //<< fighters[t].name //<< " run from " 
						////<< fighters[targets[i].target_index].name //<< " summary = (" //<< Then //<< " - " //<< now //<< ")";
					
				points *= Opponents[fighters[t].getPlayerId()].Run_predict;

				////cerr //<< " * " //<< Opponents[fighters[t].getPlayerId()].Run_predict //<< " = " //<< points //<< endl;					

				double result = points;
				if (result > best.price)
				{
					best = Variant(WV_GO, fighters[t].x, fighters[t].y, targets[i].limit, result);
				}
						
				fighters[t].new_action_points = ap;
				fighters[t].x = backup_x;
				fighters[t].y = backup_y;
			}
		}
	}
	void CheckGoCure(const World& world, vector <target>& targets, Variant& best)
	{
		for (int i = 0; i < (int)targets.size(); ++i)
		{
			if (targets[i].type == "CURE" && targets[i].var_x.size() > 0)
			{
				int dist = targets[i].limit;
				int ap = fighters[t].new_action_points;
					
				fighters[t].new_action_points -= dist * MoveCost(fighters[t]);
					
				double points = 0;
				if (CanUseMedikit())
				{
					points += game.getMedikitBonusHitpoints();
					fighters[t].new_action_points -= game.getMedikitUseCost();
				}
				while (CanCure())
				{
					points += game.getFieldMedicHealBonusHitpoints();
					fighters[t].new_action_points -= game.getFieldMedicHealCost();
				}
				if (points > fighters[targets[i].target_index].getMaximalHitpoints() - fighters[targets[i].target_index].new_hitpoints)
				{
					points = fighters[targets[i].target_index].getMaximalHitpoints() - fighters[targets[i].target_index].new_hitpoints;
				}

				////cerr //<< "       (" //<< fighters[t].name //<< " go cure " 
					 ////<< fighters[targets[i].target_index].name //<< " summary = " //<< points;
					
				points *= Opponents[fighters[t].getPlayerId()].Cure_predict;

				////cerr //<< " * " //<< Opponents[fighters[t].getPlayerId()].Cure_predict //<< " = " //<< points //<< endl;

				if (points > best.price)
				{
					best = Variant(WV_GO, targets[i].var_x[0], targets[i].var_y[0], dist, points);
				}
						
				fighters[t].new_action_points = ap;
			}
		}
	}
	double BadMark(const World& world)
	{
		double FullSummary = 0;
		double enemies_best = 0, my_best = 0;
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (fighters[i].new_hitpoints > 0)
			{
				int j = 0;
				while (j < (int)fighters.size() && 
					(fighters[j].new_hitpoints <= 0 || !AreEnemies(i, j) || !DoesSee(i, j) ||
					!world.isVisible(fighters[i].getShootingRange(), fighters[i].x, fighters[i].y, fighters[i].stance, fighters[j].x, fighters[j].y, fighters[j].stance)))
				{
					++j;
				}
				
				if (j < (int)fighters.size())
				{
					double k = fighters[i].Danger();
					if (AreEnemies(i))
					{
						FullSummary -= k;
						if (k > enemies_best)
							enemies_best = k;
					}
					else
					{
						FullSummary += k;
						if (k > my_best)
							my_best = k;
					}
				}
			}
		}
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (fighters[i].new_hitpoints > 0)
			{
				int j = 0;
				while (j < (int)fighters.size() && (!AreEnemies(i, j) || 
					!world.isVisible(fighters[j].getShootingRange(), fighters[j].x, fighters[j].y, fighters[j].stance, fighters[i].x, fighters[i].y, fighters[i].stance)))
				{
					++j;
				}

				if (j < (int)fighters.size())
				{
					if (AreEnemies(i) &&
						fighters[i].new_hitpoints <= my_best)
					{
						FullSummary += game.getTrooperEliminationScore();
					}
					else if (!AreEnemies(i) && 
						fighters[i].new_hitpoints <= enemies_best)
					{
						FullSummary -= game.getTrooperEliminationScore();
					}
				}
			}
		}

		return FullSummary;
	}
	void CheckGrenade(const World& world, Variant& best)
	{
		if (CanUseGrenade())
		{
			set < pair<int, int> > coord;
			if (GrenadeVariants(coord))
			{
				for (set < pair<int, int> >::iterator it = coord.begin(); it != coord.end(); ++it)
				{
					double Price = IfGrenade(it->first, it->second);
					
					////cerr //<< "       (" //<< fighters[t].name //<< " use grenade on  (" 
						 ////<< it->first //<< ", " //<< it->second //<< ") summary = " //<< Price //<< endl;

					if (Price > best.price)
					{
						best = Variant(WV_GRENADE, it->first, it->second, Price);
					}
				}
			}
		}
	}
	void CheckStances(const World& world, Variant& best)
	{
		if (CanProne())
		{
			TrooperStance ts = fighters[t].stance;
			fighters[t].stance = PRONE;

			int ap;
			if (ts == KNEELING)
				ap = game.getStanceChangeCost();
			else
				ap = 2 * game.getStanceChangeCost();
			double Then = BadMark(world) - 
				((fighters[t].getInitialActionPoints() - fighters[t].new_action_points + ap) / 
				fighters[t].getShootCost()) * fighters[t].getDamage();

			double result = Then - Now;
			////cerr //<< "       (" //<< fighters[t].name //<< " prone summary = " //<< result //<< endl;
			if (result > best.price)
			{
				best = Variant(WV_PRONE, result);
			}

			fighters[t].stance = ts;
		}
		if (CanKneel())
		{
			TrooperStance ts = fighters[t].stance;
			fighters[t].stance = KNEELING;

			int ap = game.getStanceChangeCost();
			
			double Then = BadMark(world) - 
				((fighters[t].getInitialActionPoints() - fighters[t].new_action_points + ap) / fighters[t].getShootCost()) * fighters[t].getDamage();

			double result = Then - Now;
			////cerr //<< "	   (" //<< fighters[t].name //<< " kneel summary = " //<< result //<< endl;
			if (result > best.price)
			{
				best = Variant(WV_KNEEL, result);
			}

			fighters[t].stance = ts;
		}
		if (CanStand())
		{
			TrooperStance ts = fighters[t].stance;
			fighters[t].stance = STANDING;

			int ap;
			if (ts == KNEELING)
				ap = game.getStanceChangeCost();
			else
				ap = 2 * game.getStanceChangeCost();
			double Then = BadMark(world) - 
				((fighters[t].getInitialActionPoints() - fighters[t].new_action_points + ap) / fighters[t].getShootCost()) * fighters[t].getDamage();

			double result = Then - Now;
			////cerr //<< "	   (" //<< fighters[t].name //<< " stand summary = " //<< result //<< endl;
			if (result > best.price)
			{
				best = Variant(WV_STAND, result);
			}

			fighters[t].stance = ts;
		}
	}
	void CheckMedikit(const World& world, Variant& best)
	{
		if (CanUseMedikit())
		{
			int ap = fighters[t].new_action_points;

			int i;
			for (i = 0; i < (int)fighters.size(); ++i)
			{
				int hp = fighters[i].new_hitpoints;
				if (DoesNeedCure(i))
				{
					UseMedikit(i);
					double Then = BadMark(world);
				
					double price = Then - Now + fighters[i].new_hitpoints - hp;

					////cerr //<< "       (" //<< fighters[t].name //<< " use medikit on " 
						 ////<< fighters[i].name //<< " summary = " //<< price;
					
					price *= Opponents[fighters[t].getPlayerId()].Medikit_predict;

					////cerr //<< " * " //<< Opponents[fighters[t].getPlayerId()].Medikit_predict //<< " = " //<< price //<< endl;	
					if (price > best.price)
					{
						best = Variant(WV_MEDIKIT, i, price);
					}
				}

				fighters[i].new_hitpoints = hp;
				fighters[t].new_action_points = ap;
				fighters[t].has_medikit = true;
			}

			fighters[t].has_medikit = true;
		}
	}
	void CheckCure(const World& world, Variant& best)
	{
		if (CanCure())
		{
			int ap = fighters[t].new_action_points;

			int i;
			for (i = 0; i < (int)fighters.size(); ++i)
			{
				int hp = fighters[i].new_hitpoints;
				while (CanCure() && DoesNeedCure(i))
				{
					Cure(i);
				}
				
				double Then = BadMark(world) + (fighters[i].new_hitpoints - hp) / 4.0;
				
				double price = Then - Now;

				////cerr //<< "       (" //<< fighters[t].name //<< " cure " 
					 ////<< fighters[i].name //<< " summary = " //<< price;
					
				price *= Opponents[fighters[t].getPlayerId()].Cure_predict;

				////cerr //<< " * " //<< Opponents[fighters[t].getPlayerId()].Cure_predict //<< " = " //<< price //<< endl;

				if (price > best.price)
				{
					best = Variant(WV_CURE, i, price);
				}

				fighters[i].new_hitpoints = hp;
				fighters[t].new_action_points = ap;
			}
		}
	}
	void GetMidBonus()
	{
        double ans = 0;
        int num_of = 0;
		for (int i = 0; i < (int)fighters.size(); ++i)
		{
			if (fighters[i].new_hitpoints > 0)
			{
                double best = UNDEFINED;
				bool succesed = false;
				for (int j = 0; j < (int)fighters.size(); ++j)
                {
                    if (!AreEnemies(i, j) && i != j && fighters[j].new_hitpoints > 0)
                    {
                        int k = abs(fighters[i].x - fighters[j].x) + abs(fighters[i].y - fighters[j].y);
                        if (k < best)
                        {
							succesed = true;
                            best = k;
                        }
                    }
                }
				if (succesed)
				{
					ans += best;
					++num_of;
				}
			}
		}

        if (num_of == 0)
            return;

		points += (10 - ans / num_of) / 1000;
		//cerr //<< "      !mid_bonus is " //<< (10 - ans / num_of) / 1000 //<< endl;
	}
	void GetHealthBonus()
    {
        int NewHealth = 0;
        for (int i = 0; i < (int)fighters.size(); ++i)
        {
            if (fighters[i].isTeammate() && fighters[i].new_hitpoints > 0)
            {
                NewHealth += fighters[i].new_hitpoints;
            }
        }

        points += NewHealth / MyStartHealth;
    }
    void SetNewTroopersCoord(const World& world)
    {
        TroopersCoord.clear();
        for (int i = 0; i < (int)fighters.size(); ++i)
        {
            TroopersCoord.insert(make_pair(fighters[i].x, fighters[i].y));
        }
    }
    double Exodus(const World& world)
	{
		while (CheckFight(world))
		{
            SetNewTroopersCoord(world);
			Now = BadMark(world);
			Variant best;

			if (CanUseFieldRation())
			{
				//cerr //<< "    " //<< fighters[t].name //<< " eats FieldRation!" //<< endl;
				UseFieldRation();
			}

			CheckMedikit(world, best);			
			CheckCure(world, best);			
			CheckShooting(world, best);
			if (CanGo())
			{
				vector <target> targets;
				CheckGoVars(world, targets);

                CheckAttack(world, targets, best);
			    CheckDeviation(world, targets, best);				
				CheckGoGrenade(world, targets, best);
				CheckGoCure(world, targets, best);
			}
			CheckGrenade(world, best);
			CheckStances(world, best);

			fighters[t].WasDoingNothing = false;
			if (best.type == WV_SHOOT)  // Выбрали стрельбу
				Shoot(best.index);
			else if (best.type == WV_GO)
			{
				GoTo(best.x, best.y, best.index, world);
			}
			else if (best.type == WV_GRENADE)
				Grenade(best.x, best.y);
			else if (best.type == WV_PRONE)
				Prone(world);
			else if (best.type == WV_KNEEL)
				Kneel(world);
			else if (best.type == WV_STAND)
				Stand(world);
			else if (best.type == WV_CURE)
			{
				//cerr //<< "    " //<< fighters[t].name //<< " cures " //<< fighters[best.index].name //<< "!" //<< endl;
				Cure(best.index);
			}
			else if (best.type == WV_MEDIKIT)
			{
				//cerr //<< "    " //<< fighters[t].name //<< " uses medikit on " //<< fighters[best.index].name //<< "!" //<< endl;
				UseMedikit(best.index);
			}
			else
			{  // выбрали безделье
				if (fighters[t].new_action_points >= MoveCost(fighters[t]))
					fighters[t].WasDoingNothing = true;

				fighters[t].new_action_points = 0;
				//cerr //<< "    " //<< fighters[t].name //<< " finishes turn!" //<< endl;
				NextTurn(world);
			}
		}

        TroopersCoord = RealTroopersCoord;

		//cerr //<< "	result without bonuses: " //<< points //<< " : " //<< enemy_points //<< endl;

		if (fighters[t].isTeammate())
		{
			GetMidBonus();
		}

		if (fighters[t].isTeammate())
			points += fighters[t].new_hitpoints * LEFT_HITPOINTS_K;
		else
			enemy_points += fighters[t].new_hitpoints * LEFT_HITPOINTS_K;

		return points - enemy_points;
	}
};

double ShootVariant(int i, MexicanDeadlock f, const World& world)
{
	f.Shoot(i);
	return f.Exodus(world);
}
double GoVariant(int x, int y, int dist, MexicanDeadlock f, const World& world)
{
	f.GoTo(x, y, dist, world);
	if (BonusesCoord[MEDIKIT].count(make_pair(x, y)))
		f.fighters[f.t].has_medikit = true;
	if (BonusesCoord[GRENADE].count(make_pair(x, y)))
		f.fighters[f.t].has_grenade = true;
	if (BonusesCoord[FIELD_RATION].count(make_pair(x, y)))
		f.fighters[f.t].has_field_ration = true;
	return f.Exodus(world);
}
double DoNothingVariant(MexicanDeadlock f, const World& world)
{
	f.NextTurn(world);
	return f.Exodus(world);
}
double CureVariant(int i, MexicanDeadlock f, const World& world)
{
	f.Cure(i);
	return f.Exodus(world);
}
double UseMedikitVariant(int i, MexicanDeadlock f, const World& world)
{
	f.UseMedikit(i);
	return f.Exodus(world);
}
double UseGrenadeVariant(int x, int y, MexicanDeadlock f, const World& world)
{	
	f.Grenade(x, y);
	return f.Exodus(world);
}
double UseFieldRationVariant(MexicanDeadlock f, const World& world)
{
	f.UseFieldRation();
	return f.Exodus(world);
}
double KneelVariant(MexicanDeadlock f, const World& world)
{
	f.Kneel(world);
	return f.Exodus(world);
}
double ProneVariant(MexicanDeadlock f, const World& world)
{
	f.Prone(world);
	return f.Exodus(world);
}
double StandVariant(MexicanDeadlock f, const World& world)
{
	f.Stand(world);
	return f.Exodus(world);
}
