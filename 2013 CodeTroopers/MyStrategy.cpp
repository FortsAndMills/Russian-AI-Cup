#include "War.h"
#include "Peace.h"
#include "Bonuses.h"
#include <algorithm>

MyStrategy::MyStrategy() { }

bool FindTrooper(int x, int y)
{
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		if (troopers[i].getX() == x && troopers[i].getY() == y)
		{
			return true;
		}
	}
	return false;
}
bool FindTrooperNear(int x, int y)
{
	for (int i = 0; i < (int)enemies.size(); ++i)
	{
		if (abs(enemies[i].getX() - x) + abs(enemies[i].getY() - y) <= 1)
		{
			return true;
		}
	}
	return false;
}

bool MoveTrooper(Move& move, const World& world)
{
	if (self.getActionPoints() < MoveCost(self))
	{
		move.setAction(END_TURN);
		return true;
	}

	//cerr //<< "do_not_move = " //<< do_not_move //<< endl;
	double best = do_not_move;  // выбираем лучший ход
	vector <int> indexes;
	
	for (int i = 0; i < (int)new_x.size(); ++i)
	{
		//cerr //<< "Move to (" //<< new_x[i] //<< ", " //<< new_y[i] //<< "): " //<< price[i] //<< endl;
		if (price[i] > best)
		{
			best = price[i];
			indexes.clear();
			indexes.push_back(i);
		}
		else if (price[i] == best && best != 0)
		{
			indexes.push_back(i);
		}
	}

	if (indexes.size() > 0)  // если он отрицательный, да ну всё это.
	{
		if (!CapDeadlock[self.getType()] && best < 1000)
			CheckCapDeadlock(indexes, world);

		if (indexes.size() == 0)
		{
			if (NewTurnStart && CapDeadlockMove(best, world))
				return MoveTrooper(move, world);
			else
				return false;
		}

		//cerr //<< "Decided to go to (" //<< new_x[indexes[0]] //<< ", " //<< new_y[indexes[0]] //<< ")" //<< endl;
		move.setAction(MOVE);
		move.setX(new_x[indexes[0]]);
		move.setY(new_y[indexes[0]]);
		return true;
	}
	return false;
}
void FinalDecision(Move& move, const World& world)
{
	if (STATE == "peace")  // В состоянии мира в округе нет никаких врагов.
	{	
		Union(world);

		if (HowManyMovesCanDo() <= SEARCH_SAFETY_LIMITS)
			DangerCheck(world);

		DangerSearch(world);
		if (CheckCure(world, move))  // Проверяем лечилку. Если кого-то вылечили - баюньки
			return;
		if (StanceCheck(move))
			return;

		if (CapDeadlock[self.getType()])
			CapDeadlockRealize(world);

		if (MoveTrooper(move, world))
			return;
		
		if (HasCrusade)
		{
			if (Crusade(world))
			{
				move.setAction(REQUEST_ENEMY_DISPOSITION);
				return;
			}
			else if (MoveTrooper(move, world))
				return;
		}		

		if (CheckRandomShoot(move, world))
			return;
		else if (self.getActionPoints() >= game.getStanceChangeCost())
		{
			//cerr //<< "Decided to do NOTHING by RASING STANCE" //<< endl;
			move.setAction(RAISE_STANCE);
			return;
		}

		//cerr //<< "Decided to do NOTHING" //<< endl;
		move.setAction(END_TURN);

		return;
	}
	else if (STATE == "war")
	{
		Variant bv = Fight_starter(world);
		if (bv.type == WV_DO_NOTHING)
			++IsWaitingForAttack;
		else
			IsWaitingForAttack = 0;

		if (bv.type == WV_SHOOT)
		{
			ShootTrooper(bv.x, bv.y);
			if (self.getActionPoints() < self.getShootCost())
			{
				int k = 0;
			}
			//cerr //<< "SUMMARY: shoot" //<< endl;
			move.setAction(SHOOT);
			move.setX(bv.x);
			move.setY(bv.y);
			return;
		}
		else if (bv.type == WV_CURE)
		{
			if (self.getActionPoints() < game.getFieldMedicHealCost() ||
				self.getType() != FIELD_MEDIC ||
				abs(bv.x - self.getX()) + abs(bv.y - self.getY()) > 1 ||
				!FindTrooper(bv.x, bv.y))
			{
				int k = 0;
			}
			//cerr //<< "SUMMARY: HEAL" //<< endl;
			move.setAction(HEAL);
			move.setX(bv.x);
			move.setY(bv.y);
			return;
		}
		else if (bv.type == WV_MEDIKIT)
		{
			if (self.getActionPoints() < game.getMedikitUseCost() ||
				!self.isHoldingMedikit() ||
				abs(bv.x - self.getX()) + abs(bv.y - self.getY()) > 1 ||
				!FindTrooper(bv.x, bv.y))
			{
				int k = 0;
			}
			//cerr //<< "SUMMARY: USE MEDIKIT" //<< endl;
			move.setAction(USE_MEDIKIT);
			move.setX(bv.x);
			move.setY(bv.y);
			return;
		}
		else if (bv.type == WV_GRENADE)
		{
			if (self.getActionPoints() < game.getGrenadeThrowCost() ||
				!self.isHoldingGrenade() ||
				self.getDistanceTo(bv.x, bv.y) > game.getGrenadeThrowRange() ||
				!FindTrooperNear(bv.x, bv.y))
			{
				int k = 0;
			}
			GrenadeTroopers(bv.x, bv.y);
			//cerr //<< "SUMMARY: THROW GRENADE" //<< endl;
			move.setAction(THROW_GRENADE);
			move.setX(bv.x);
			move.setY(bv.y);
			return;
		}
		else if (bv.type == WV_FIELD_RATION)
		{
			if (self.getActionPoints() < game.getFieldRationEatCost() ||
				!self.isHoldingFieldRation())
			{
				int k = 0;
			}
			//cerr //<< "SUMMARY: EAT FIELD RATION" //<< endl;
			move.setAction(EAT_FIELD_RATION);
			return;
		}
		else if (bv.type == WV_KNEEL)
		{
			if (self.getActionPoints() < game.getStanceChangeCost())
			{
				int k = 0;
			}

			if (self.getStance() == STANDING)
			{
				//cerr //<< "SUMMARY: LOWER STANCE" //<< endl;
				move.setAction(LOWER_STANCE);
			}
			else
			{
				//cerr //<< "SUMMARY: RAISE STANCE" //<< endl;
				move.setAction(RAISE_STANCE);
			}
		}
		else if (bv.type == WV_STAND)
		{
			if (self.getActionPoints() < game.getStanceChangeCost() ||
				self.getStance() == WV_STAND)
			{
				int k = 0;
			}
			//cerr //<< "SUMMARY: RAISE STANCE" //<< endl;
			move.setAction(RAISE_STANCE);
		}
		else if (bv.type == WV_PRONE)
		{
			if (self.getActionPoints() < game.getStanceChangeCost() ||
				self.getStance() == WV_PRONE)
			{
				int k = 0;
			}
			//cerr //<< "SUMMARY: LOWER STANCE" //<< endl;
			move.setAction(LOWER_STANCE);
		}
		else if (bv.type == WV_RUN)
		{
			if (self.getActionPoints() < MoveCost(self))
				int a = 0;

			//cerr //<< "SUMMARY: RUN" //<< endl;
			for (int i = 0; i < (int)enemies.size(); ++i)
			{
				WantToGoHere(enemies[i].getX(), enemies[i].getY(), -1000, 1, world);
			}
			MoveTrooper(move, world);
			return;
		}
		else if (bv.type == WV_GO)
		{
			if (self.getStance() != STANDING)
			{
				int j = 0;
				while (j < (int)bv.bv_1.size() && dis[bv.bv_1[j]][bv.bv_2[j]] <= 1)
				{
					++j;
				}
				if (j < (int)bv.bv_1.size())
				{
					int a = 0;
				}
			}

			if (self.getActionPoints() < MoveCost(self))
				int a = 0;
			WantToGoHere(bv.bv_1, bv.bv_2, bv.price + 1000, 0, world);
			MoveTrooper(move, world);
			return;
		}
		else
		{
			//cerr //<< "SUMMARY: WV_DO_NOTHING :(" //<< endl;
			move.setAction(END_TURN);
			return;
		}
	}
}

void FinishTurn(const World& world, Move& move)
{
	if (move.getAction() == MOVE)
	{
		for (int i = 0; i < world.getWidth(); ++i)
				for (int j = 0; j < world.getHeight(); ++j)
					dis[i][j] = UNDEFINED;

		FillDistances(move.getX(), move.getY(), world);
	}

	for (int i = 0; i < (int)BA.size(); ++i)
	{
		BA[i].dist[self.getType()] = dis[BA[i].x][BA[i].y];
	}
}
void MyStrategy::move(const Trooper& _self, const World& world, const Game& _game, Move& move)
{
	Starter(world, _self, _game);  // Начинаем новый ход

	//cerr //<< endl //<< "NEW TURN: " //<< Fighter(self).name 
	//<< ", ap = " //<< self.getActionPoints() //<< ", hp = " //<< self.getHitpoints() 
	//<< ", (" //<< self.getX() //<< ", " //<< self.getY() //<< "), stance = " //<< self.getStance() //<< endl;
	
	UpdateBasicThings(world);
	TroopersCheck(world);  // Проверяем, нет ли клеток, с которых видно нас, но не видно клетку
    BonusesCheck(world);  // Проверяем, к каким бонусам можно пойти
	DangerSearchUpdate(world);
	
	FinalDecision(move, world);  // Принимаем решение

	FinishTurn(world, move);
}
