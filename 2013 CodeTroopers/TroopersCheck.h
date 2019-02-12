#pragma once

#include "Danger.h"
#include "Going.h"

const int DANGER_TIME = 3;

map<int, TrooperType> back_order;
vector <target> BonusTargets;

class BonusInfo
{
public:
	BonusType type;
	int x, y;
	map<TrooperType, int> dist;

	BonusInfo(Bonus& b)
	{
		x = b.getX();
		y = b.getY();
		type = b.getType();
		for (int i = 0; i < (int)back_order.size(); ++i)
		{
			dist[back_order[i]] = UNDEFINED;
		}
	}
	BonusInfo(Bonus& b, int k, const World& world)
	{
		type = b.getType();
		if (k == 0)
		{
			x = b.getX();
			y = b.getY();
		}
		else if (k == 1)
		{
			x = world.getWidth() - 1 - b.getX();
			y = b.getY();
		}
		else if (k == 2)
		{
			x = world.getWidth() - 1 - b.getX();
			y = world.getHeight() - 1 - b.getY();
		}
		else if (k == 3)
		{
			x = b.getX();
			y = world.getHeight() - 1 - b.getY();
		}

		for (int i = 0; i < (int)back_order.size(); ++i)
		{
			dist[back_order[i]] = UNDEFINED;
		}
	}

	void MoveTo(double Price, const World& world, bool IsTarget, bool Need)
	{
		dist[self.getType()] = dis[x][y];

		int i = 0;
		while (i < (int)back_order.size() && (dist[back_order[i]] >= dis[x][y] - 1))
			++i;

		int X, Y;
		if (i == (int)back_order.size())
		{
			X = x;
			Y = y;
		}
		else
		{
			int j = 0;
			int best = dis[x][y] - 1;
			int ans = self_index;
			for (j = 0; j < (int)troopers.size(); ++j)
			{
				if (dist.count(troopers[j].getType()) &&
					dist[troopers[j].getType()] < best)
				{
					best = dist[troopers[j].getType()];
					ans = j;
				}
			}

			if (troopers[ans].getId() != self.getId())
			{
				if (dist[troopers[ans].getType()] <= 0)
					int a = 0;
				X = troopers[ans].getX();
				Y = troopers[ans].getY();
			}
			else
			{
				X = x;
				Y = y;
				dist[back_order[i]] = UNDEFINED;
			}
		}
		
		if (Need || ((X != x || Y != y) && dis[x][y] > game.getCommanderAuraRange()))
			WantToGoHere(X, Y, Price, 0, world);
		else
			BonusTargets.push_back(target(x, y, WANT_BONUS_LIMIT, Price));

		if (IsTarget && Need)
		{
			int j = 0;
			while (j < (int)ways.size() && 
				(ways[j].target_x[0] != x || ways[j].target_y[0] != y))
				++j;

			if (j == (int)ways.size())
				ways.push_back(Way(x, y, Price * 100, world));
		}
	}
};

vector <BonusInfo> BA;  // Архив невзятых бонусов.

class Fighter : public Trooper
{
public:
	int new_hitpoints;  // Класс для "изменяющегося" трупера. У него своё здоровье
	int new_action_points;  // количество очков действия
	int x, y; // координаты
	string name;  // имя в помощь
	bool has_grenade, has_medikit, has_field_ration;
	TrooperStance stance;
	int sr;

	int update_time;
	string status;
	bool WasGoing;
	bool WasDoingNothing;

	Fighter() : Trooper()
	{
		update_time = -UNDEFINED;
		status = "unknown";
	}
	Fighter(Trooper& t) : Trooper(t)
	{
		new_hitpoints = getHitpoints();
		x = getX();
		y = getY();
		has_grenade = t.isHoldingGrenade();
		has_medikit = t.isHoldingMedikit();
		has_field_ration = t.isHoldingFieldRation();
		stance = t.getStance();
		WasGoing = false;
		WasDoingNothing = false;
		sr = t.getShootingRange();

		update_time = my_time;
		status = "seen";

		if (t.getId() == self.getId())
			new_action_points = self.getActionPoints();
		else
			new_action_points = 0;

		if (!t.isTeammate())
			name = "ENEMY ";
		else
			name = "";

		if (t.getType() == COMMANDER)
			name += "COMMANDER";
		else if (t.getType() == FIELD_MEDIC)
			name += "MEDIC";
		else if (t.getType() == SNIPER)
			name += "SNIPER";
		else if (t.getType() == SCOUT)
			name += "SCOUT";
		else
			name += "SOLDIER";
	}

	double getShootingRange()
	{
		if (getType() != SNIPER)
			return sr;
		else if (stance == KNEELING)
			return 10 + game.getSniperKneelingShootingRangeBonus();
		else if (stance == PRONE)
			return 10 + game.getSniperProneShootingRangeBonus();
        else
            return 10;
	}
	int getDamage()
	{
		if (stance == STANDING)
			return getStandingDamage();
		if (stance == KNEELING)
			return getKneelingDamage();
		else
			return getProneDamage();
	}
	double Danger()  // Сколько может нанести урона за ход
	{
		if (has_field_ration)
			return ((getInitialActionPoints() - game.getFieldRationEatCost() + game.getFieldRationBonusActionPoints()) / getShootCost()) * getDamage();
		return (getInitialActionPoints() / getShootCost()) * getDamage();
	}
	double CurrentDanger()  // Сколько может нанести урона за текущий ход
	{
		return NumOfShoots() * getDamage();
	}
	int NumOfShoots()  // Сколько может нанести урона за текущий ход
	{
		if (new_action_points < 0)
			return 0;

		if (has_field_ration &&
			new_action_points >= game.getFieldRationEatCost() &&
			new_action_points - game.getFieldRationEatCost() + game.getFieldRationBonusActionPoints() <=
			getInitialActionPoints())
			return ((new_action_points + game.getFieldRationBonusActionPoints() - game.getFieldRationEatCost()) / getShootCost());
		
		return (new_action_points / getShootCost());
	}
	int GetMoveCost()
	{
		return MoveCost(stance);
	}
	int NumOfMoves()  // Сколько может нанести урона за текущий ход
	{
		if (new_action_points < 0)
			return 0;

		if (has_field_ration &&
			new_action_points >= game.getFieldRationEatCost() &&
			new_action_points - game.getFieldRationEatCost() + game.getFieldRationBonusActionPoints() <=
			getInitialActionPoints())
			return ((new_action_points + game.getFieldRationBonusActionPoints() - game.getFieldRationEatCost()) / GetMoveCost());
		
		return (new_action_points / GetMoveCost());
	}
	double DistanceTo(Fighter& f)
	{
		return sqrt((double)((f.x - x) * (f.x - x) + (f.y - y) * (f.y - y)));
	}
	double DistanceTo(int X, int Y)
	{
		return sqrt((double)((X - x) * (X - x) + (Y - y) * (Y - y)));
	}
	int ComeCloser(Fighter& f, bool IsSeen)
	{
		int ans = 0;
		do
		{
			++ans;
			if (abs(x - f.x) > abs(y - f.y))
			{
				if (x < f.x)
					++x;
				else
					--x;
			}
			else
			{
				if (y < f.y)
					++y;
				else
					--y;
			}
		}
		while ((IsSeen && DistanceTo(f) > getShootingRange()) ||
			(!IsSeen && DistanceTo(f) > min(getShootingRange(), getVisionRange())));
		return ans;
	}
	int ComeCloserAtGrenade(Fighter& f)
	{
		int ans = 0;
		do
		{
			++ans;
			if (abs(x - f.x) > abs(y - f.y))
			{
				if (x < f.x)
					++x;
				else
					--x;
			}
			else
			{
				if (y < f.y)
					++y;
				else
					--y;
			}
		}
		while ((DistanceTo(f) > game.getGrenadeThrowRange()));
		return ans;
	}
	int ComeCloserAtGrenadeUndirectly(Fighter& f)
	{
		int ans = 0;
		do
		{
			++ans;
			if (abs(x - f.x) > abs(y - f.y))
			{
				if (x < f.x)
					++x;
				else
					--x;
			}
			else
			{
				if (y < f.y)
					++y;
				else
					--y;
			}
		}
		while ((DistanceTo(f) > game.getGrenadeThrowRange() + 1));
		return ans;
	}
	int Deviate(Fighter& f)
	{
		int ans = 0;
		do
		{
			++ans;
			if (abs(x - f.x) > abs(y - f.y))
			{
				if (x < f.x)
					--x;
				else
					++x;
			}
			else
			{
				if (y < f.y)
					--y;
				else
					++y;
			}
		}
		while (DistanceTo(f) <= f.getShootingRange());
		return ans;
	}
	int IfDamaged(int dam)
	{
		int ans = dam;
		if (new_hitpoints <= ans)
		{  // Бонус за элиминацию
			ans = game.getTrooperEliminationScore() + new_hitpoints;
		}
		return ans;
	}
	int GetDamaged(int dam)
	{
		int ans = dam;
		if (new_hitpoints <= ans)
		{  // Бонус за элиминацию
			ans = game.getTrooperEliminationScore() + new_hitpoints;
		}
		new_hitpoints -= ans;
		return ans;
	}
};
int MoveCost(Fighter& f)
{
	return MoveCost(f.stance);
}

vector<int> players_order;
class Opponent : public Player
{
private:
	double Cure_for, Cure_against;
	double Run_for, Run_against;
	double Medikit_for, Medikit_against;
	double Attack_for, Attack_against;

public:
	map<TrooperType, Fighter> fighters;
	bool IsOrderStated;
	bool Alive;

	double Cure_predict;
	double Medikit_predict;
	double Run_predict;
	double Attack_predict;

	Opponent() : Player(UNDEFINED, "", 0, 0, -1, -1)
	{}
	Opponent(Player& p) : Player(p)
	{
		Cure_for = 1;
		Cure_against = 0;
		Cure_predict = 1;

		Medikit_for = 1;
		Medikit_against = 0;
		Medikit_predict = 1;

		Run_for = 1;
		Run_against = 0;
		Run_predict = 1;

		Attack_for = 1;
		Attack_against = 0;
		Attack_predict = 1;

		IsOrderStated = false;
		Alive = true;
	}

	bool IsAlive()
	{
		if (!Alive)
			return false;

		int i = 0;
		while (i < (int)back_order.size() && fighters[back_order[i]].new_hitpoints <= 0)
			++i;

		return i < (int)back_order.size();
	}

	bool IsHiding(TrooperType t)
	{
		return !fighters[t].isTeammate() && 
			fighters[t].new_hitpoints > 0 && 
			fighters[t].status == "hide";
	}
	bool IsJustOutOfVision(TrooperType t, const World& world)
	{
		return IsHiding(t);
	}
	void Dehide(TrooperType t, const World& world)
	{
		if (fighters[t].status == "hide")
		{
			fighters[t].status = "ran";
			SuddenRun(fighters[t].x, fighters[t].y, world);
		}
	}

	void Moved(Trooper& t, const World& world)
	{
		if (fighters[t.getType()].has_medikit)
		{
			if (!t.isHoldingMedikit())
			{
				//cerr //<< "Noticed, that " //<< fighters[t.getType()].name //<< " used medikit!" //<< endl;
				++Medikit_for;
			}
			else if (fighters[t.getType()].new_hitpoints < fighters[t.getType()].getMaximalHitpoints())
			{
				//cerr //<< "Noticed, that " //<< fighters[t.getType()].name //<< " didn't use medikit!" //<< endl;
				++Medikit_against;
			}
			Medikit_predict = (Medikit_for) / (1 + Medikit_against);
		}
		if (fighters[FIELD_MEDIC].new_hitpoints > 0 &&
			(abs(fighters[FIELD_MEDIC].x - fighters[t.getType()].x) + 
			abs(fighters[FIELD_MEDIC].y - fighters[t.getType()].y) <= 1 || 
			abs(fighters[FIELD_MEDIC].x - t.getX()) + 
			abs(fighters[FIELD_MEDIC].y - t.getY()) <= 1) && 
			fighters[t.getType()].new_hitpoints >= t.getHitpoints())
		{
			//cerr //<< "Noticed, that " //<< fighters[t.getType()].name //<< " didn't cure!" //<< endl;
			++Cure_against;
			Cure_predict = (Cure_for) / (1 + Cure_against);
		}
		if (fighters[t.getType()].x != t.getX() ||
			fighters[t.getType()].y != t.getY() ||
			fighters[t.getType()].stance != t.getStance())
		{
			bool CanBeAttack = false;
			bool WasAttack = false;
			bool CanBeRun = false;
			bool WasRun = false;
			for (int i = 0; i < (int)troopers.size(); ++i)
			{
				if (!world.isVisible(fighters[t.getType()].getShootingRange(),
					fighters[t.getType()].x, fighters[t.getType()].y, fighters[t.getType()].stance,
					troopers[i].getX(), troopers[i].getY(), troopers[i].getStance()))
				{
					CanBeAttack = true;
					if (world.isVisible(fighters[t.getType()].getShootingRange(),
					t.getX(), t.getY(), t.getStance(),
					troopers[i].getX(), troopers[i].getY(), troopers[i].getStance()))
					{
						WasAttack = true;						
					}
				}
				else
				{
					CanBeRun = true;
					if (!world.isVisible(fighters[t.getType()].getShootingRange(),
					t.getX(), t.getY(), t.getStance(),
					troopers[i].getX(), troopers[i].getY(), troopers[i].getStance()))
					{
						WasRun = true;						
					}
				}
			}

			if (CanBeAttack)
			{
				if (WasAttack)
				{
					//cerr //<< "Noticed, that " //<< fighters[t.getType()].name //<< " attacked!" //<< endl;
					++Attack_for;
				}
				else
				{
					//cerr //<< "Noticed, that " //<< fighters[t.getType()].name //<< " didn't attack!" //<< endl;
					++Attack_against;
				}
				Attack_predict = Attack_for / (1 + Attack_against);
			}

			if (CanBeRun)
			{
				if (WasRun)
				{
					//cerr //<< "Noticed, that " //<< fighters[t.getType()].name //<< " ran!" //<< endl;
					++Run_for;
				}
				else
				{
					//cerr //<< "Noticed, that " //<< fighters[t.getType()].name //<< " didn't run!" //<< endl;
					++Run_against;
				}
				Run_predict = Run_for / (1 + Run_against);
			}
		}
	}

	void SetFirst()
	{
		IsOrderStated = true;
		int i = 0;
		while (players_order[i] != getId())
			++i;
		while (i > 0)
		{
			swap(players_order[i], players_order[i - 1]);
			--i;
		}
	}
	void SetLast()
	{
		IsOrderStated = true;
		int i = 0;
		while (players_order[i] != getId())
			++i;
		while (i < (int)players_order.size() - 1)
		{
			swap(players_order[i], players_order[i + 1]);
			++i;
		}
	}
	void Update(Trooper& t, const World& world)
	{
		if (fighters[t.getType()].status != "seen")
			return;

		if (fighters[t.getType()].x != t.getX() || 
			fighters[t.getType()].y != t.getY() || 
			fighters[t.getType()].has_grenade != t.isHoldingGrenade() ||
			fighters[t.getType()].has_medikit != t.isHoldingMedikit() || 
			fighters[t.getType()].has_field_ration != t.isHoldingFieldRation() ||
			fighters[t.getType()].stance != t.getStance())
		{
			if (t.getType() == self.getType())
			{
				//cerr //<< "Found that " //<< fighters[t.getType()].name //<< " did move: set him first" //<< endl;
				SetFirst();
			}
			else
			{
				//cerr //<< "Found that " //<< fighters[t.getType()].name //<< " did move: set him last" //<< endl;
				SetLast();
			}
			Moved(t, world);
		}		
		else if (fighters[t.getType()].new_hitpoints < t.getHitpoints())
		{
			int i = 0;
			while (i < NumOfTypes && 
				(fighters[back_order[i]].status != "unknown" ||
				fighters[back_order[i]].new_hitpoints <= 0 ||
				abs(fighters[back_order[i]].x - fighters[t.getType()].x) +
				abs(fighters[back_order[i]].y - fighters[t.getType()].y) != 1))
			{
				++i;
			}

			if (i == NumOfTypes)
			{
				++Cure_for;
				Cure_predict = (Cure_for) / (1 + Cure_against);
				if (self.getType() == FIELD_MEDIC)
				{
					//cerr //<< "Found that " //<< fighters[t.getType()].name //<< " cured: set him first" //<< endl;
					SetFirst();
				}
				else
				{
					//cerr //<< "Found that " //<< fighters[t.getType()].name //<< " cured: set him last" //<< endl;
					SetLast();
				}
			}
		}
		else if (IsWaitingForAttack >= (int)troopers.size())
		{
			//cerr //<< "Waiting for attack, but nothing happens... (" //<< fighters[t.getType()].name //<< ")" //<< endl;
			++Attack_against;
			Attack_predict = Attack_for / (1 + Attack_against);
		}
	}

	void IsNotSeen(TrooperType t, const World& world)
	{
		if (fighters[t].status == "hide" &&
			IsSeen(self.getX(), self.getY(), fighters[t].x, fighters[t].y, world))
		{
			SuddenDissapear(fighters[t].x, fighters[t].y, world);
			fighters[t].status = "ran";
			fighters[t].x = -UNDEFINED;
			fighters[t].y = -UNDEFINED;
			//cerr //<< "Found that " //<< fighters[t].name //<< " ran away: no hiding" //<< endl;
			return;
		}

		if (fighters[t].status == "unknown" || 
            fighters[t].status == "ran" ||
            fighters[t].status == "hide" ||
			my_time - fighters[t].update_time != 1 ||
            fighters[t].new_hitpoints <= 0)
			return;

		//cerr //<< fighters[t].name //<< " is not seen! We are in danger! Gets status ";

        if (getId() == self.getPlayerId())
		{
			SuddenAttack(fighters[t].x, fighters[t].y, world);
			for (int i = 0; i < (int)BA.size(); ++i)
			{
				BA[i].dist[t] = UNDEFINED;
			}
			//cerr //<< "killed - it's mine trooper" //<< endl;
			fighters[t].new_hitpoints = -100;
		}
		else if (IsSeen(self.getX(), self.getY(), fighters[t].x, fighters[t].y, world))
		{
			SuddenDissapear(fighters[t].x, fighters[t].y, world);
			fighters[t].status = "ran";
			fighters[t].x = -UNDEFINED;
			fighters[t].y = -UNDEFINED;

			if (t == self.getType())
			{
				//cerr //<< "Found that " //<< fighters[t].name //<< " ran: set him first" //<< endl;
				SetFirst();
			}
			else
			{
				//cerr //<< "Found that " //<< fighters[t].name //<< " ran: set him last" //<< endl;
				SetLast();
			}

			//cerr //<< "ran" //<< endl;
		}
		else
		{
			fighters[t].status = "hide";
			//cerr //<< "hide" //<< endl;
		}
	}

	void UpdateMe(Trooper& t, const World& world)
	{
		if (t.getHitpoints() < fighters[t.getType()].new_hitpoints && STATE != "war")
		{
			//cerr //<< fighters[t.getType()].name //<< " lost his hitpoints! We are in danger!" //<< endl;
			SuddenAttack(t.getX(), t.getY(), world);
		}
	}
};

map<int, Opponent> Opponents;
void CreateOpponents(const World& world)
{
	vector <Player> players = world.getPlayers();
	for (int i = 0; i < (int)players.size(); ++i)
	{
		Opponents[players[i].getId()] = Opponent(players[i]);
		
		if (players[i].getId() != self.getPlayerId())
		{
			players_order.push_back(players[i].getId());
		}
	}
	players_order.push_back(self.getPlayerId());

	vector <Trooper> troopers = world.getTroopers();
	NumOfTypes = 0;
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		if (troopers[i].isTeammate())
		{
			back_order[NumOfTypes] = troopers[i].getType();
			++NumOfTypes;
		}
	}
}
int CreateFight(vector <Fighter>& fighters, const World& world)
{
	int ans = 0;
	vector <Trooper> AllTroopers = world.getTroopers();
	set <int> was_here;
	was_here.insert(self.getPlayerId());
	for (int k = 0; k < (int)players_order.size(); ++k)
	{
		if ((!Opponents[players_order[k]].IsOrderStated) &&
			players_order[k] != self.getPlayerId() &&
			!was_here.count(players_order[k]))
		{
			int i = 0;
			while (i < (int)AllTroopers.size() && 
				!Opponents[players_order[k]].IsJustOutOfVision(back_order[self.getType()], world) &&
				(AllTroopers[i].getPlayerId() != players_order[k] ||
				AllTroopers[i].getType() != self.getType()))
			{
				++i;
			}
			
			int id = players_order[k];
			if (i < (int)AllTroopers.size())
			{
				Opponents[id].SetLast();
			}
			else
			{
				Opponents[id].SetFirst();
			}
			Opponents[id].IsOrderStated = false;
			was_here.insert(id);
			k = -1;
		}
	}

	for (int n = 0; n < NumOfTypes; ++n)
	{
		for (int k = 0; k < (int)players_order.size(); ++k)
		{
			int i = 0;
			while (i < (int)AllTroopers.size() &&
				(AllTroopers[i].getPlayerId() != players_order[k] ||
				AllTroopers[i].getType() != back_order[n]))
			{
				++i;
			}

			if (i < (int)AllTroopers.size())
			{
				fighters.push_back(Fighter(AllTroopers[i]));
				fighters[fighters.size() - 1].new_action_points = 0;
				
				if (AllTroopers[i].getId() == self.getId())
				{
					fighters[fighters.size() - 1].new_action_points = self.getActionPoints();
					ans = fighters.size() - 1;
				}
			}
			else if (Opponents[players_order[k]].IsJustOutOfVision(back_order[n], world))
			{
				//cerr //<< "Added phantom fighter: " //<< (Opponents[players_order[k]].fighters[back_order[n]]).name //<< endl;
				fighters.push_back(Opponents[players_order[k]].fighters[back_order[n]]);
				fighters[fighters.size() - 1].new_action_points = 0;

				if (Opponents[players_order[k]].fighters[back_order[n]].x < 0)
					int a = 0;
			}
		}
	}
	return ans;
}

bool WasShooting = false;
int TargetPlayerId;
int MyCurrentScore = 0;
int HadGetPoints;
void UpdateBasicThings(const World& world)
{	
	vector <Player> players = world.getPlayers();
	for (int i = 0; i < (int)players.size(); ++i)
	{
		if (players[i].getId() == self.getPlayerId())
		{
			MyCurrentScore = players[i].getScore();
		}
	}

	if (WasShooting && 
		MyCurrentScore - HadGetPoints == game.getPlayerEliminationScore() - game.getTrooperEliminationScore())
	{
		Opponents[TargetPlayerId].Alive = false;
		//cerr //<< Opponents[TargetPlayerId].getName() //<< " is not alive because I've killed him!" //<< endl;
	}
	WasShooting = false;
}
void ShootTrooper(int x, int y)
{
	for (int k = 0; k < (int)players_order.size(); ++k)
	{
		for (int n = 0; n < NumOfTypes; ++n)
		{
			if (Opponents[players_order[k]].fighters[back_order[n]].x == x &&
				Opponents[players_order[k]].fighters[back_order[n]].y == y &&
				Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints > 0)
			{
				Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints -= self.getDamage();
				//cerr //<< "BAD POINT: " //<< Opponents[players_order[k]].fighters[back_order[n]].name //<< ", " //<< x //<< ", " //<< y 
					//<< ", and now his hp = " //<< Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints //<< endl;

				WasShooting = true;
				TargetPlayerId = players_order[k];
				HadGetPoints = MyCurrentScore + self.getDamage();
				if (Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints <= 0)
				{
					HadGetPoints += Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints + game.getTrooperEliminationScore();
				}
				return;
			}
		}
	}
	int a = 0;
}
void GrenadeTroopers(int x, int y)
{
	for (int k = 0; k < (int)players_order.size(); ++k)
	{
		for (int n = 0; n < NumOfTypes; ++n)
		{
			if (Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints > 0)
			{
				int S = abs(Opponents[players_order[k]].fighters[back_order[n]].x - x) + 
					abs(Opponents[players_order[k]].fighters[back_order[n]].y - y);

				if (S == 0)
				{
					Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints -= 
							game.getGrenadeDirectDamage();
				}
				else if (S == 1)
				{
					Opponents[players_order[k]].fighters[back_order[n]].new_hitpoints -= 
							game.getGrenadeCollateralDamage();
				}
			}
		}
	}
}

void WriteInfo(const World& world)
{
	for (int i = 0; i < (int)enemies.size(); ++i)
	{
		Opponents[enemies[i].getPlayerId()].fighters[enemies[i].getType()] = 
			Fighter(enemies[i]);
	}
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		Opponents[troopers[i].getPlayerId()].fighters[troopers[i].getType()] = 
			Fighter(troopers[i]);
	}
}
void DeleteHidersByTurnOrder(const World& world)
{
	int k = 0;
	while (players_order[k] != self.getPlayerId())
		++k;

	for (k; k < (int)players_order.size(); ++k)
	{
		if (Opponents[players_order[k]].IsOrderStated ||
			(!Opponents[players_order[k]].IsOrderStated && 
			Opponents[players_order[k]].fighters[last_type].update_time != my_time))
		{
			//cerr //<< "There was a turn of " //<< Opponents[players_order[k]].fighters[last_type].name //<< " from " //<< Opponents[players_order[k]].getName() //<< endl;
			Opponents[players_order[k]].Dehide(last_type, world);
		}
	}

	int i = 0;
	while (back_order[i] != last_type)
		++i;

	++i;
	if (i >= (int)back_order.size())
		i = 0;

	while (back_order[i] != self.getType())
	{
		for (k = 0; k < (int)players_order.size(); ++k)
		{
			if (Opponents[players_order[k]].IsOrderStated ||
				(!Opponents[players_order[k]].IsOrderStated && 
				Opponents[players_order[k]].fighters[back_order[i]].update_time != my_time))
			{
				//cerr //<< "There was a turn of " //<< Opponents[players_order[k]].fighters[back_order[i]].name //<< " from " //<< Opponents[players_order[k]].getName() //<< endl;
				Opponents[players_order[k]].Dehide(back_order[i], world);
			}
		}
				
		++i;
		if (i >= (int)back_order.size())
			i = 0;
	}

	for (k = 0; players_order[k] != self.getPlayerId(); ++k)
	{
		if (Opponents[players_order[k]].IsOrderStated ||
			(!Opponents[players_order[k]].IsOrderStated && 
			Opponents[players_order[k]].fighters[self.getType()].update_time != my_time))
		{
			//cerr //<< "There was a turn of " //<< Opponents[players_order[k]].fighters[self.getType()].name //<< " from " //<< Opponents[players_order[k]].getName() //<< endl;
			Opponents[players_order[k]].Dehide(self.getType(), world);
		}
	}
}
void Analize(const World& world)
{
	if (world.getMoveIndex() == 0)
	{
		if (last_type != self.getType())
		{
			int j = 0;
			while (back_order[j] != self.getType())
				++j;

			while (j > 0 && back_order[j - 1] != last_type)
			{
				swap(back_order[j - 1], back_order[j]);
				--j;
			}

			back_order[j] = self.getType();
		}
	}
	else
	{
		if (NewTurnStart)
			DeleteHidersByTurnOrder(world);

		for (int i = 0; i < (int)enemies.size(); ++i)
		{
			Opponents[enemies[i].getPlayerId()].Update(enemies[i], world);
		}
		for (int i = 0; i < (int)troopers.size(); ++i)
		{
			Opponents[troopers[i].getPlayerId()].UpdateMe(troopers[i], world);
		}
	}

	WriteInfo(world);

	for (int k = 0; k < (int)players_order.size(); ++k)
	{
		for (int n = 0; n < NumOfTypes; ++n)
		{
			Opponents[players_order[k]].IsNotSeen(back_order[n], world);
		}
	}

	for (int n = 0; n < NumOfTypes; ++n)
	{
		for (int k = 0; k < (int)players_order.size(); ++k)
		{
			if (Opponents[players_order[k]].IsJustOutOfVision(back_order[n], world))
			{
				//cerr //<< "There is phantom fighter: " //<< (Opponents[players_order[k]].fighters[back_order[n]]).name //<< endl;
				STATE = "war";
			}
		}
	}
}

void TroopersCheck(const World& world)
{
	vector <Trooper> AllTroopers = world.getTroopers();
	for (int i = 0; i < (int)AllTroopers.size(); ++i)  // Для каждого трупера определяем
    {
		if (AllTroopers[i].isTeammate())  // Заполняем массив наших труперов.
		{
			troopers.push_back(AllTroopers[i]);

			if (troopers[troopers.size() - 1].getType() == COMMANDER)
				commander_index = troopers.size() - 1;
			if (troopers[troopers.size() - 1].getId() == self.getId())
				self_index = troopers.size() - 1;

			mid_x += AllTroopers[i].getX();
			mid_y += AllTroopers[i].getY();
		}
		else
		{
			STATE = "war";
			enemies.push_back(AllTroopers[i]);
		}
		
		for (int j = 0; j < (int)price.size(); ++j)  // Если трупер стоит в соседней клетке, то такой вариант ходьбы надо сразу вырезать.
		{
			if (world.getTroopers()[i].getX() == new_x[j] && world.getTroopers()[i].getY() == new_y[j])
			{
				new_x.erase(new_x.begin() + j, new_x.begin() + j + 1);
				new_y.erase(new_y.begin() + j, new_y.begin() + j + 1);
				price.erase(price.begin() + j, price.begin() + j + 1);
				break;
			}
		}
    }
	mid_x /= troopers.size();
	mid_y /= troopers.size();

	TroopersCoord.clear();
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		if (troopers[i].getId() != self.getId())
			TroopersCoord.insert(make_pair(troopers[i].getX(), troopers[i].getY()));
	}

	SetDis(world);  // составляем расстояния и радуемся

	Analize(world);

	if (STATE != "war")
		IsWaitingForAttack = 0;
	if (STATE != "peace")
		CapDeadlock[self.getType()] = false;

	//cerr //<< "State = " //<< STATE //<< endl;
}
