#pragma once
#include "Lead.h"

Goals Rushgoals;

void Statings()
{
	vector <long long int> ids;  // Сортируем время до шайбы
	for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
	{
		if (hcit->second.getState() != RESTING)
		{
			ids.push_back(hcit->first);

			int j = ids.size() - 1;
			while (j > 0 && hc[ids[j]].Target.PuckTime < hc[ids[j - 1]].Target.PuckTime)
			{
				swap(ids[j], ids[j - 1]);
				--j;
			}
		}
	}

	int countdown = hc[ids[0]].Target.PuckTime;  // Минимум
	for (int i = 0; i < (int)ids.size(); ++i)
	{
		if ((double)hc[ids[i]].Target.PuckTime / countdown <= 1 + CANDIDATION ||
			hc[ids[i]].Target.PuckTime - hc[ids[0]].Target.PuckTime <= CANDIDATION_DIFFERENCE)
		{
			if (hc[ids[i]].isTeammate())
				hc[ids[i]].maintask_state = "candidate";  // Те кто может взять шайбу
			else
				hc[ids[i]].maintask_state = "enemy";
		}
		else
		{
			hc[ids[i]].maintask_state = "outsider";  // Те кто не может
		}
	}

	for (int i = 0; i < (int)ids.size(); ++i)
	{
		hc[ids[i]].Target.x = hc[ids[0]].Target.x;
		hc[ids[i]].Target.y = hc[ids[0]].Target.y;

		/*if (hc[ids[i]].isTeammate())
		{
			int j = i;
			while (j >= 0 && hc[ids[j]].isTeammate())
				--j;
			if (j >= 0)
			{
				hc[ids[i]].Target.x = hc[ids[j]].Target.x;
				hc[ids[i]].Target.y = hc[ids[j]].Target.y;
			}
		}*/
	}

	int i = 0;
	while (!(hc[ids[i]].isTeammate()))
		++i;
	if (hc[ids[i]].maintask_state == "candidate")  // наша надежда или пустая попытка из принципа
		hc[ids[i]].maintask_state = "our hope";
	else
		hc[ids[i]].maintask_state = "luck hope";

	for (int i = 0; i < (int)ids.size(); ++i)
	{
		cerr << hc[ids[i]].getTeammateIndex() + 1 << ") " << hc[ids[i]].state << ", " << hc[ids[i]].Target.PuckTime << endl;
	}
}

void HowAboutFight(MyHockeyist &h)
{
	if (h.getRemainingCooldownTicks() > 0)
		return;
	if (CanPickPuck(h) && Rushgoals.Increment == -1)
	{
		h.answer.action = TAKE_PUCK;
	}

	if (CanStrikePuck(h) && Rushgoals.Increment != -1 && !WillBeGoalToEnemy(h, h.getAngle(), h.getSwingTicks()))
	{
		cerr << "Rushfight says " << h.getTeammateIndex() + 1 << " not to fight as there will be goal" << endl;
		return;
	}
	if (CanStrikePuck(h) && ((Rushgoals.Decrement != -1 && !WillBeGoalToMe(h)) || WillBeGoalToEnemy(h)))  // осторожно, здесь инкремент-декремент обнуляются
	{
		h.answer.action = STRIKE;
	}

	int greatness = 0;
	for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
	{
		if (CanStrikeHockeyist(h, hcit->second) && hcit->second.getState() != RESTING)
		{
			if (Rushgoals.Increment > hcit->second.Target.PuckTime)
			{
				cerr << "Rushfight says " << h.getTeammateIndex() + 1 << " to FIGHT FOR GOAL!!!" << endl;
				greatness = INF;
				break;
			}

			int new_time = GetTime(hcit->second, IfFighted(h, hcit->second), (int)G.getKnockdownTicksFactor());
			if (hcit->second.isTeammate() && Rushgoals.Increment == -1)
			{
				if (new_time > hcit->second.Target.PuckTime)
					return;
				else
					greatness += -new_time + hcit->second.Target.PuckTime;
			}
			else
			{
				if (Rushgoals.Increment != -1)
					greatness += INF;
				else if (new_time < hcit->second.Target.PuckTime)
					return;
				else
					greatness += new_time - hcit->second.Target.PuckTime;
			}
		}
	}

	if (greatness > GREATNESS_ENOUGH_TO_FIGHT)
	{
		h.answer.action = STRIKE;
		cerr << h.getId() << " decided fight; " << greatness << endl;
	}
}

const double SPEED_TO_PICK = 10;
const double BEAT_DEFENDER = 10;
void MakeAnswer(MyHockeyist &h)
{
	long long int id = h.getId();
	if (h.getState() == KNOCKED_DOWN)
		return;

	if ((hc[id].maintask_state == "candidate" ||
		hc[id].maintask_state == "our hope" ||
		hc[id].maintask_state == "luck hope") &&
		Rushgoals.Increment == -1)
	{       
		GoTo(h, hc[id].Target.x, hc[id].Target.y, true);  // Идём за шайбой
	}
	else
	{
		variants.clear();
		DefendNet(h);
		DefendNet(h, 1);
		ExchangeHockeyists(h);
		GoAttack(h);

        variants.push_back(Variant(h, hc[id].Target.x, hc[id].Target.y, "candidate", false));

		for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
			if (!IT->second.isTeammate() && IT->second.getState() != RESTING && IT->second.Target.PuckTime < Rushgoals.Increment)
				variants.push_back(Variant(h, IT->second.getX(), IT->second.getY(), "striker", false, BEAT_DEFENDER));

		ChooseBestVariant(h);
		
		if (h.state == "substitute" && CanBeSubstituted(h))
		{
			Substitute(h);
		}
	}
}

void Rush()
{
	Rushgoals.Clear();
	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
		if (IT->second.getState() != RESTING)
			GetTime(IT->second, Rushgoals);

	Statings();
	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
		if (IT->second.isTeammate() && IT->second.getState() != RESTING)
			HowAboutFight(IT->second);
	
	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
		if (IT->second.isTeammate() && IT->second.getState() != RESTING)
			MakeAnswer(IT->second);
}