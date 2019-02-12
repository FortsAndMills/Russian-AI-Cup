#include "TroopersCheck.h"

const int DANGER_PANIC = 1000;
bool CheckCure(const World& world, Move& move)
{
	bool global_problems = false;
	for (int i = 0; i < (int)price.size(); ++i)
	{
		global_problems = price[i] - do_not_move > DANGER_PANIC;
		if (global_problems)
			return false;
	}

	if (self.getType() == FIELD_MEDIC)
	{
		bool found = false;
		for (int i = 0; i < (int)troopers.size(); ++i)
		{  // Если есть раненный...
			if (troopers[i].getHitpoints() < troopers[i].getMaximalHitpoints() &&
				troopers[i].getId() != self.getId() && !global_problems)
			{
				if (self.getActionPoints() >= game.getFieldMedicHealCost() &&
					abs(troopers[i].getX() - self.getX()) + abs(troopers[i].getY() - self.getY()) == 1 &&
					troopers[i].getHitpoints() < troopers[i].getMaximalHitpoints())
				{
					//cerr //<< "Cured " //<< Fighter(troopers[i]).name //<< endl;
					move.setAction(HEAL);
					move.setX(troopers[i].getX());
					move.setY(troopers[i].getY());
					return true;
				}
				else
				{
					found = true;
					//cerr //<< "Decided to cure " //<< Fighter(troopers[i]).name //<< endl;
					WantToGoHere(troopers[i].getX(), troopers[i].getY(), 
						troopers[i].getMaximalHitpoints() - troopers[i].getHitpoints() + 100, 1, world);
					
					ways.push_back(Way(troopers[i].getX(), troopers[i].getY(), 
						(troopers[i].getMaximalHitpoints() - troopers[i].getHitpoints()) * 10000, world));
					ways[ways.size() - 1].id.insert(troopers[i].getId());
				}
			}
		}

		if (self.getActionPoints() >= game.getFieldMedicHealCost() &&
			!global_problems && !found && self.getHitpoints() < self.getMaximalHitpoints())
		{
			//cerr //<< "Cured myself" //<< endl;
			move.setAction(HEAL);
			move.setX(self.getX());
			move.setY(self.getY());
			return true;
		}
		return false;
	}
	
	if (self.getHitpoints() < self.getMaximalHitpoints())
	{
		for (int i = 0; i < (int)troopers.size(); ++i)
		{  // Если есть медик...
			if (troopers[i].getType() == FIELD_MEDIC &&
				self.getType() != FIELD_MEDIC && !global_problems)
			{
				//cerr //<< "Decided to be cured by " //<< Fighter(troopers[i]).name //<< endl;
				WantToGoHere(troopers[i].getX(), troopers[i].getY(), 
					self.getMaximalHitpoints() - self.getHitpoints() + 100, 1, world);
				
				ways.push_back(Way(troopers[i].getX(), troopers[i].getY(), 
						(self.getMaximalHitpoints() - self.getHitpoints()) * 10000, world));
				ways[ways.size() - 1].id.insert(troopers[i].getId());
				return false;
			}
		}

		bool found = false;
		for (int i = 0; i < (int)troopers.size(); ++i)
		{  // Если есть аптечка у друга...
			if (troopers[i].isHoldingMedikit() && troopers[i].getId() != self.getId())
			{
				found = true;
				//cerr //<< "Decided to use medikit of " //<< Fighter(troopers[i]).name //<< endl;
				WantToGoHere(troopers[i].getX(), troopers[i].getY(), 
					self.getMaximalHitpoints() - self.getHitpoints() + 100, 1, world);
				
				ways.push_back(Way(troopers[i].getX(), troopers[i].getY(), 
						(self.getMaximalHitpoints() - self.getHitpoints()) * 10000, world));
				ways[ways.size() - 1].id.insert(troopers[i].getId());
			}
		}

		if (!found && self.isHoldingMedikit() &&
			!global_problems && self.getHitpoints() < self.getMaximalHitpoints())
		{
			//cerr //<< "Cured myself with medikit" //<< endl;
			move.setAction(USE_MEDIKIT);
			move.setX(self.getX());
			move.setY(self.getY());
			return true;
		}
	}

	int i = 0;
	while (i < (int)troopers.size() && troopers[i].getType() != FIELD_MEDIC)
		++i;

	if (i < (int)troopers.size())
		return false;
	
	if (self.isHoldingMedikit())
	{
		for (int i = 0; i < (int)troopers.size(); ++i)
		{  // Если есть раненный...
			if (troopers[i].getHitpoints() < troopers[i].getMaximalHitpoints() &&
				troopers[i].getId() != self.getId())
			{
				//cerr //<< "Decided to cure " //<< Fighter(troopers[i]).name //<< endl;
				WantToGoHere(troopers[i].getX(), troopers[i].getY(), 
					troopers[i].getMaximalHitpoints() - troopers[i].getHitpoints() + 100, 1, world);
				
				ways.push_back(Way(troopers[i].getX(), troopers[i].getY(), 
						(troopers[i].getMaximalHitpoints() - troopers[i].getHitpoints()) * 10000, world));
				ways[ways.size() - 1].id.insert(troopers[i].getId());
			}
			
			// Если рядом стоит кто-то, кого прям сейчас можно вылечить.			
			if (!global_problems && self.getActionPoints() >= game.getMedikitUseCost() &&
				abs(troopers[i].getX() - self.getX()) + abs(troopers[i].getY() - self.getY()) == 1 &&
				troopers[i].getHitpoints() < troopers[i].getMaximalHitpoints())
			{
				//cerr //<< "Decided to cure " //<< Fighter(troopers[i]).name //<< endl;
				move.setAction(USE_MEDIKIT);
				move.setX(troopers[i].getX());
				move.setY(troopers[i].getY());
				return true;
			}
		}
	}
	return false;
}

void FindExistingField(int& x, int& y, const World& world)
{
	while (dis[x][y] == UNDEFINED || !DoExist(x, y, world))
	{
		++x;
		if (x >= world.getWidth())
		{
			x = world.getWidth() / 2;
			++y;
		}
	}
}

void CapDeadlockRealize(const World& world)
{
	if (CapDeadlockTargetId[self.getType()] < 0)
	{
		double dist = -1;
		CapDeadlockTargetId[self.getType()] = -1;
		for (int i = 0; i < (int)troopers.size(); ++i)
		{
			if (wt_dis[troopers[i].getX()][troopers[i].getY()] > dist)
			{
				dist = wt_dis[troopers[i].getX()][troopers[i].getY()];
				CapDeadlockTargetId[self.getType()] = troopers[i].getId();
			}
		}

		//cerr //<< "Special CapDeadlock: going to the farest (" //<< dist //<< ") person" //<< endl;
	}

	int CapDeadlockX = -1, CapDeadlockY = -1;
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		if (troopers[i].getId() == CapDeadlockTargetId[self.getType()])
		{
			CapDeadlockX = troopers[i].getX();
			CapDeadlockY = troopers[i].getY();
			break;
		}
	}

	if (CapDeadlockX == -1)
	{
		CapDeadlockTargetId[self.getType()] = -1;
		CapDeadlockRealize(world);
		return;
	}

	double dist = wt_dis[CapDeadlockX][CapDeadlockY];
	if (dist <= (int)troopers.size() - 1)
	{
		do_not_move += 1000;
		CapDeadlock[self.getType()] = false;
		CapDeadlockTargetId[self.getType()] = -1;
		return;
	}

	WantToGoHere(CapDeadlockX, CapDeadlockY, 1000, 1, world);
}
void CheckCapDeadlock(vector <int>& indexes, const World& world)
{
	bool AreWeIn = CommanderBonus(self.getX(), self.getY(), world);

	if (!CapDeadlock[self.getType()])
	{
		for (int i = 0; i < (int)indexes.size(); ++i)
		{
			if (STATE == "peace" && AreWeIn && 
				!CommanderBonus(new_x[indexes[i]], new_y[indexes[i]], world))
			{
				price[indexes[i]] -= 1000;
				indexes.erase(indexes.begin() + i, indexes.begin() + i + 1);
				--i;
			}
		}
	}
}
bool CapDeadlockMove(double best, const World& world)
{
	do_not_move += best;
	//cerr //<< "No move because CAP FORBIDDS" //<< endl;
	if (NewTurnStart)
	{
		double dist = 0;
		int crusade_i = 0;
		for (int k = 0; k < (int)troopers.size(); ++k)
		{
			double G = GetMarkDistance(troopers[k].getX(), troopers[k].getY(), mid_x, mid_y);
			if (G > dist)
			{
				dist = G;
				crusade_i = k;
			}
		}
					
		CapDeadlock[troopers[crusade_i].getType()] = true;
		if (troopers[crusade_i].getType() == self.getType())
		{
			CapDeadlockRealize(world);
			return true;
		}
		return false;
	}
	return false;
}

void Union(const World& world)
{
	if (CommanderBonus(self.getX(), self.getY(), world))
	{
		return;
	}

	HasCrusade = false;
	for (int i = 0; i < (int)troopers.size(); ++i)
		CapDeadlock[troopers[i].getType()] = true;
	CapDeadlockRealize(world);
}

bool GetInfo(const World& world)
{
	vector <Player> players = world.getPlayers();
	double best = UNDEFINED;
	int T = -1;
	for (int i = 0; i < (int)players.size(); ++i)
	{
		if (players[i].getId() != self.getPlayerId())
		{
			int x = players[i].getApproximateX();
			int y = players[i].getApproximateY();

			if (x != -1 && y != -1)
			{
				double dis = self.getDistanceTo(x, y);
				if (dis < best)
				{
					best = dis;
					T = i;
				}
			}
		}
	}

	if (T != -1)
	{
		for (int i = 0; i < (int)players.size(); ++i)
		{
			if (players[i].getId() != self.getPlayerId() && players[i].getApproximateX() == -1)
			{
				Opponents[players[i].getId()].Alive = false;
				//cerr //<< "Because of request I think, that " //<< Opponents[players[i].getId()].getName() //<< " is dead" //<< endl;
			}
		}

		CrusadeX = players[T].getApproximateX();
		CrusadeY = players[T].getApproximateY();
		CrusadeGetTime = world.getMoveIndex();

		FindExistingField(CrusadeX, CrusadeY, world);

		//cerr //<< "New crusade target: " //<< CrusadeX //<< ", " //<< CrusadeY //<< ")" //<< endl;
		return true;
	}
	return false;
}
bool Crusade(const World& world)
{
	if (!GetInfo(world) && self.getType() == COMMANDER &&
		self.getActionPoints() >= game.getCommanderRequestEnemyDispositionCost() &&
		(CrusadeX == -1 || world.getMoveIndex() - CrusadeGetTime >= REQUEST_MAX_TIME - 2))
	{
		//cerr //<< "SEND REQUEST TO DISPOSITION" //<< endl;
		return true;
	}

	int N = 3;
	while (commander_index < 0 && 
		(CrusadeX < 0 || (self.getX() == CrusadeX && self.getY() == CrusadeY)))
	{
		if (BA.size() != 0 && N > 0)
		{
			//cerr //<< "Crusading to random bonus!!!" //<< endl;
			int k = rand() % BA.size();
			CrusadeX = BA[k].x;
			CrusadeY = BA[k].y;
			--N;
		}
		else if (CrusadeX >= 0)
		{
			//cerr //<< "Crusading to another side!!!" //<< endl;
			CrusadeX = world.getWidth() - 1 - CrusadeX;
			CrusadeY = world.getHeight() - 1 - CrusadeY;
		}
		else
		{
			//cerr //<< "Crusading to 0, 0!!!" //<< endl;
			CrusadeX = 0;
			CrusadeY = 0;
		}
	}

	if (CrusadeX >= 0 && CrusadeY >= 0)
	{
		//cerr //<< "Crusading to (" //<< CrusadeX //<< ", " //<< CrusadeY //<< ")" //<< endl;
		WantToGoHere(CrusadeX, CrusadeY, 1, 0, world);
	}
	return false;
}

bool StanceCheck(Move &move)
{
	if (self.getStance() != STANDING &&
		self.getActionPoints() >= game.getStanceChangeCost())
	{
		move.setAction(RAISE_STANCE);
		return true;
	}
	return false;
}

bool CheckRandomShoot(Move &move, const World& world)
{
	if (self.getActionPoints() < self.getShootCost())
		return false;

	int min_x = max(0.0, self.getX() - self.getShootingRange());
	int max_x = min((double)world.getWidth(), self.getX() + self.getShootingRange());
	int min_y = max(0.0, self.getY() - self.getShootingRange());
	int max_y = min((double)world.getHeight(), self.getY() + self.getShootingRange());

	vector< pair<int, int> > variants;
	bool OnlyCurious = false;
	for (int x = min_x; x < max_x; ++x)
	{
		for (int y = min_y; y < max_y; ++y)
		{
			if (DoExistWT(x, y, world) && !IsSeen(self.getX(), self.getY(), x, y, world) &&
				world.isVisible(self.getShootingRange(), self.getX(), self.getY(), self.getStance(), x, y, STANDING))
			{
				if (CuriousFields.count(make_pair(x, y)))
				{
					if (!OnlyCurious)
					{
						OnlyCurious = true;
						variants.clear();
					}
					variants.push_back(make_pair(x, y));
				}
				else if (!OnlyCurious)
					variants.push_back(make_pair(x, y));
			}
		}
	}

	if (variants.size() > 0)
	{
		move.setAction(SHOOT);
		int k = rand() % (int)variants.size();
		move.setX(variants[k].first);
		move.setY(variants[k].second);
		//cerr //<< "Random shoot! On (" //<< variants[k].first //<< ", " //<< variants[k].second //<< ")" //<< endl;
		return true;
	}
	else
	{
		//cerr //<< "No vars for random shoot!" //<< endl;
	}
	return false;
}

void CheckSelfKilling(Move &move, const World& world)
{
	int MyScore;
	int Enemies = 0;
	int EnemiesScore = 0;

	vector <Player> p = world.getPlayers();
	for (int i = 0; i < p.size(); ++i)
	{
		if (p[i].getId() == self.getPlayerId())
		{
			MyScore = p[i].getScore();
		}
		else
		{
			int score = p[i].getScore();

			if (Opponents[p[i].getId()].IsAlive())
			{
				++Enemies;
				score += game.getLastPlayerEliminationScore();
			}

			if (score > EnemiesScore)
				EnemiesScore = score;
		}
	}

	if (self.isHoldingGrenade() && 
		self.getActionPoints() >= game.getGrenadeThrowCost() &&
		MyScore > EnemiesScore &&
		Enemies < 2 &&
		(int)troopers.size() < 2 &&
		self.getHitpoints() <= game.getGrenadeCollateralDamage())
	{
		move.setAction(THROW_GRENADE);
		move.setX(self.getX());
		move.setY(self.getY());
	}
}