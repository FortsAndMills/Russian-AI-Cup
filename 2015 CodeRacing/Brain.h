#pragma once

#include "PathEstimation.h"

const double ANGLE_COEFF = 5;
const double SMALL_VELOCITY = 11;
const double STUCK_VELOCITY = 2;
double goToAngle(PhysMe & P, double target)
{
	if (P.speed.module() > 0 && fabs(norm(P.speed.angle() - P.ang)) > PI / 2)
		target = norm(target + PI);
	return norm(target - P.ang) * ANGLE_COEFF;
}

double FullbonusesAchievment()
{
	double ans = 0;
	for (int i = 0; i < bonuses[ID].size(); ++i)
	{
		if (bonuses[ID][i]->isTaken)
			ans += bonuses[ID][i]->estimation;
	}
	return ans;
}

BorderInfo whereBreak;
bool Fly(Path * path, PhysMe & P, double tar_angle, int turn_index, int target_index, bool IsCheck = false, MyBonus * B = NULL, int try_time = -1, int leftright = 0)
{
	if (path->path.size() <= target_index)
		return false;

	P.restart();
	if (IsCheck)
	{
		P.speed = Vector(0, 0);
		P.engP = 0;
		P.spAng = 0;
	}
	for (int i = 0; i < bonuses[ID].size(); ++i)
	{
		bonuses[ID][i]->isTaken = false;
	}

	while (!path->path[target_index]->whereIs(P.x, P.y).inside)
	{
		int p_min = 0;
		int p_max = P.points.size();

		bool IsAnywhere = false;

		int p = p_min;
		while (p < p_max)
		{
			int i = 0;
			while (i <= target_index)
			{
				whereBreak = path->path[i]->whereIs(P.points[p]);

				IsAnywhere = IsAnywhere || !whereBreak.outside;

				if (whereBreak.inside || whereBreak.outside ||
					(!IsCheck && turn_index >= 0 && i == 0 && p > 2))
					++i;
				else
					break;
			}
			whereBreak.path_index = i;

			if (i <= target_index)
			{
				break;
			}

			++p;
		}

		

		if (p == p_max)
		{
			if (!IsAnywhere)  // 25.2
			{
				int x = P.x / game->getTrackTileSize();
				int y = P.y / game->getTrackTileSize();
				if (IsCheck || !isAdjacent(x, y, path->path[0]))
				{
					whereBreak.outside = true;
					return true;
				}
			}

			if (B != NULL && !B->isTaken && P.tick - world->getTick() <= try_time)
			{
				if (B->estimation >= 0)
				{
					P.ans.setWheels(goToAngle(P, Vector(B->x - P.x, B->y - P.y).angle()));
				}
				else
				{
					P.ans.setWheels(leftright);
				}
			}
			else
				P.ans.setWheels(goToAngle(P, tar_angle));

			if (B == NULL && P.tick - world->getTick() <= try_time)
			{
				P.ans.setWheels(-P.ans.wheels());
			}
		}
		else
		{
			return true;
		}

		P.NextTick();

		for (int i = 0; i < bonuses[ID].size(); ++i)
		{
			bonuses[ID][i]->checkIfTaken(&P);
		}

		if (P.speed.module2() < E && P.tick - world->getTick() > 3)
		{
			whereBreak.outside = true;
			return true;
		}
		////cout << "   (" << P.x << " " << P.y << "), speed = (" << P.speed.x << ", " << P.speed.y << "); " << P.ang << ", " << P.spAng << "; " << P.engP << " and " << P.wheT << endl;
	}
	return false;
}

double target_angle;
MyBonus * tar_bonus = NULL;

double Gotbonuses = 0;
int leftright = 0;
bool NewBonusesVariant(MyBonus * bonus, int lr)
{
	double newVar = FullbonusesAchievment();
	if (newVar > Gotbonuses)
	{
		//cout << "good time: " << try_time << endl;
		Gotbonuses = newVar;
		tar_bonus = bonus;
		leftright = lr;

		for (int i = 0; i < bonuses[ID].size(); ++i)
		{
			bonuses[ID][i]->isPlannedToBeTaken = bonuses[ID][i]->isTaken;
		}
	}

	if (bonus->isTaken && bonus->estimation >= 0)
	{
		return true;
	}
	if (!bonus->isTaken && bonus->estimation < 0)
	{
		return true;
	}
	return false;
}

int chosen_turn;
bool TryTurn(Path * path2, PhysMe & P, int turn, int time = -1)
{
	if (path2->path.size() <= turn + 1)
		return false;

	Path * path = new Path(path2, turn);
	
	bool IsDiagonal = (path2->types[turn] == TURN_LEFT || path2->types[turn] == TURN_RIGHT) &&
					  (path2->types[turn + 1] == TURN_LEFT || path2->types[turn + 1] == TURN_RIGHT) &&
					  (path2->types[turn + 1] != path2->types[turn]);

	double supposedAngle = WayToAngle(path2->from[turn]);
	if (IsDiagonal)
	{
		supposedAngle = MiddleAngle(path2->from[turn] | path2->from[turn + 1]);

		path->add(path2->from[turn]);
		path->add(path2->from[turn + 1]);
		path->path[path->path.size() - 1]->setType(path->path[turn]->type);
	}
	else
	{
		path->add(path2->from[turn]);
		path->add(path2->from[turn]);
		path->path[path->path.size() - 1]->setType(path->path[turn + 1]->type);
	}

	if (Fly(path, P, supposedAngle, turn, turn + 2, false, NULL, time))
	{
		if (!whereBreak.outside && (whereBreak.path_index > turn ||
			(turn != 0 && whereBreak.path_index == turn && (whereBreak.isRect ||
				path->path[turn]->edges_turns[whereBreak.i] != (path->to[turn] | path->from[turn])))))
		{
			P.ans.brake = true;
			target_angle = supposedAngle;
			chosen_turn = turn;

			//cout << "TURN " << turn << " WITH BRAKE (" << P.x << " " << P.y << ")" << endl;

			return true;
		}
		else
		{
			//cout << "DECLINE TURN " << turn << " (" << P.x << " " << P.y << ")" << endl;
			return false;
		}
	}
	else
	{
		//cout << "TURN " << turn << " (" << P.x << " " << P.y << ")" << endl;
		target_angle = supposedAngle;
		chosen_turn = turn;
		return true;
	}
}

map <int, bool> IsDrivingBack;
map <int, int> StuckTime;
map <int, int> StuckBruter;
void Think(Path * path)
{
	if (self[ID]->getDurability() == 0 || world->getTick() < game->getInitialFreezeDurationTicks())
		IsDrivingBack[ID] = false;

	PhysMe P(Answer(IsDrivingBack[ID] ? -1 : 1, 0, false));
	tar_bonus = NULL;

	for (int i = 0; i < bonuses[ID].size(); ++i)
	{
		delete bonuses[ID][i];
	}
	bonuses[ID].clear();
	for (int i = 0; i < 3; ++i)
	{
		for (int k = 0; k < bonusesOnTile[path->path[i]->x][path->path[i]->y].size(); ++k)
		{
			bonuses[ID].push_back(new MyBonus(bonusesOnTile[path->path[i]->x][path->path[i]->y][k], i));
		}
	}

	target_angle = WayToAngle(path->from[0]);
	chosen_turn = -1;

	// œŒ¬Œ–¿◊»¬¿≈Ã
	if (path->types[1] != STRAIGHT)
	{
		TryTurn(path, P, 1);
	}
	else if (path->types[2] != STRAIGHT)
	{
		TryTurn(path, P, 2);
	}
	else if (path->types[3] != STRAIGHT)
	{
		TryTurn(path, P, 3);
	}

	// ¡ŒÕ”—€
	if (!P.ans.brake)
	{
		Fly(path, P, target_angle, chosen_turn, 3);

		Gotbonuses = FullbonusesAchievment();

		vector <bool> targets;
		for (int i = 0; i < bonuses[ID].size(); ++i)
		{
			if (bonuses[ID][i]->estimation >= 0)
			{
				targets.push_back(!bonuses[ID][i]->isTaken);
			}
			else
			{
				targets.push_back(bonuses[ID][i]->isTaken);
			}
			bonuses[ID][i]->isPlannedToBeTaken = bonuses[ID][i]->isTaken;
		}

		//cout << "---" << endl;
		for (int i = 0; i < bonuses[ID].size(); ++i)
		{
			if (targets[i])
			{
				for (int try_time = 1; try_time <= 25; try_time += 2)
				{
					if (!Fly(path, P, target_angle, chosen_turn, 3, false, bonuses[ID][i], try_time, 1))
					{
						if (NewBonusesVariant(bonuses[ID][i], 1))
						{
							break;
						}
					}
					if (bonuses[ID][i]->estimation < 0 &&
						!Fly(path, P, target_angle, chosen_turn, 3, false, bonuses[ID][i], try_time, -1))
					{
						if (NewBonusesVariant(bonuses[ID][i], -1))
						{
							break;
						}
					}
				}
			}
		}
	}

	// ¬»ÀﬂÕ»≈
	bool moving = false;
	if (tar_bonus == NULL && chosen_turn == -1)
	{
		PhysMe T(Answer(P.ans.Engine, 0, 0));
		if (path->types[1] != STRAIGHT)
		{
			for (int try_time = 1; try_time <= 40; try_time += 2)
			{
				if (TryTurn(path, T, 1, try_time))
				{
					moving = true;
					target_angle = norm(target_angle + PI);
					break;
				}
			}
		}
		else if (path->types[2] != STRAIGHT)
		{
			for (int try_time = 1; try_time <= 40; try_time += 2)
			{
				if (TryTurn(path, T, 2, try_time))
				{
					moving = true;
					target_angle = norm(target_angle + PI);
					break;
				}
			}
		}

		/*if (!moving && panic)
		{
			for (int try_time = 1; try_time <= 40; try_time += 2)
			{
				if (TryTurn(path, T, 0, try_time))
				{
					moving = true;
					target_angle = norm(target_angle + PI);
					break;
				}
			}
		}*/
	}
	
	// “Œ–ÃŒ«¿
	bool panic = Fly(path, P, target_angle, 2, 2);
	if (!moving && chosen_turn == -1 && tar_bonus == NULL)
	{
		if (panic &&
			fabs(norm(target_angle - self[ID]->getAngle())) >= pow(self[ID]->getDurability(), 2) * PI / 4 ||
			(self[ID]->getRemainingOiledTicks() > 0 && fabs(norm(target_angle - self[ID]->getAngle())) >= PI / 180))  //?
		{
			P.ans.brake = true;
		}

		if (panic)
		{
			Vector A = path->path[0]->MainLeavePoint(path->from[0]);
			target_angle = (A - Vector(self[ID]->getX(), self[ID]->getY())).angle();
		}
	}
	
	Vector MySpeed = Vector(self[ID]->getSpeedX(), self[ID]->getSpeedY());
	P.ans.brake = P.ans.brake && MySpeed.module() > SMALL_VELOCITY;
	
	// Œ“⁄≈«ƒ Õ¿«¿ƒ
	if ((MySpeed.module() <= STUCK_VELOCITY || IsDrivingBack[ID]))
	{
		bool a = Fly(path, P, target_angle, 2, 2, true);
		PhysMe Test(Answer(IsDrivingBack[ID] ? 1 : -1, 0, 0));
		bool b = Fly(path, Test, target_angle, 2, 2, true);

		if (a && P.speed.module2() > 0 && (!b || Test.tick > P.tick))
		{
			//cout << "CHANGE DIRECTION!" << endl;
			//P.ans.Engine = -1;
			//P.ans.brake = false;

 			StuckBruter[ID] = 0;
			IsDrivingBack[ID] = !IsDrivingBack[ID];
		}
	}

	// ¡–”“≈–
	if (world->getTick() > game->getInitialFreezeDurationTicks() && self[ID]->getDurability() > 0)
	{
		if (StuckBruter[ID] != 0)
		{
			P.ans.Engine = StuckBruter[ID];
			P.ans.brake = false;
			//target_angle = norm(target_angle + PI);

			--StuckTime[ID];
			if (StuckTime[ID] == 0)
				StuckBruter[ID] = 0;
		}
		else if (MySpeed.module() <= 0.5)
		{
			++StuckTime[ID];
			if (StuckTime[ID] > 60)
			{
				StuckTime[ID] = 80;
				StuckBruter[ID] = IsDrivingBack[ID] ? 1 : -1;
			}
		}
		else
			StuckTime[ID] = 0;
	}
	else
		StuckTime[ID] = 0;

	P.restart();
	if (tar_bonus != NULL)
	{
		if (tar_bonus->estimation >= 0)
			P.ans.setWheels(goToAngle(P, Vector(tar_bonus->x - self[ID]->getX(), tar_bonus->y - self[ID]->getY()).angle()));
		else
			P.ans.setWheels(leftright);
	}
	else
		P.ans.setWheels(goToAngle(P, target_angle));
	
	path->ans = P.ans;
}