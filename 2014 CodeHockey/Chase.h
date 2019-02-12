#pragma once
#include "AbstractActions.h"

int DefenceTime = INF;

const double CANDIDATION = 0.2;
const int CANDIDATION_DIFFERENCE = 4;
double bestpucktime;
void CStatings()
{
	vector <long long int> ids;  // Сортируем по возрастанию времени догона
	for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
	{
		if (hcit->second.isTeammate() && hcit->second.getState() != RESTING)
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

	bestpucktime = hc[ids[0]].Target.PuckTime;
    hc[ids[0]].maintask_state = "fighter";
	for (int i = 0; i < (int)ids.size(); ++i)
	{
        hc[ids[i]].maintask_state = "outsider";
		/*if ((double)hc[ids[i]].Target.PuckTime / countdown <= CANDIDATION ||
			hc[ids[i]].Target.PuckTime - hc[ids[0]].Target.PuckTime <= CANDIDATION_DIFFERENCE)
		{  // Те, кому имеет смысл бежать за шайбой
			hc[ids[i]].maintask_state = "fighter";
		}
		else
		{  // И те, кому нет
			hc[ids[i]].maintask_state = "outsider";
		}*/
	}
}
void DefenceStates()
{
	DefenceTime = INF;
	if (MyUp(en_leader).IsIn(puck.getX(), puck.getY()) || MyDown(en_leader).IsIn(puck.getX(), puck.getY()))
	{
		DefenceTime = (int)floor(en_leader.angleTo(MyGoalNetTarget()) / en_leader.MaxTurnAngle());

		if (en_leader.getRemainingCooldownTicks() > DefenceTime)
			DefenceTime = en_leader.getRemainingCooldownTicks();

		if (en_leader.getState() == SWINGING)
		{
			if (WillBeGoalToMe(en_leader))
				DefenceTime = 0;
			else
			{
				DefenceTime = G.getSwingActionCooldownTicks() + G.getCancelStrikeActionCooldownTicks();
				cerr << "Lying attack, no panic!" << endl;
			}
		}
	}
}

const double NORMWAY_COEFF = 4 / 5.0;
void GetTimeAndTarget(MyHockeyist &h)
{
    MathVector puck_pos = MathVector(puck.getX(), puck.getY());
    Goals g;

    MathVector trust = MathVector(cos(en_leader.getAngle()), sin(en_leader.getAngle())) * en_leader.speed.module() - MathVector(int_IsLeft, 0);

    GetTime(h, puck_pos.x, puck_pos.y, MyGoalie.getY(), h.getX(), h.getY(), 
        trust.x, trust.y, false, h.speed, 0, h.getRemainingKnockdownTicks(), h.Target, g);

	/*MathVector NormWay = en_leader.speed;//MyGoalNetTarget() - MathVector(puck.getX(), puck.getY());

	NormWay *= h.getDistanceTo(puck) / NormWay.module();

	MathVector target = NormWay * NORMWAY_COEFF + (en_leader.position + puck_pos) / 2;

	h.Target.x = target.x;
	h.Target.y = target.y;
	h.Target.PuckTime = TimeToGetToTarget(h, h.Target.x, h.Target.y, h.getX(), h.getY(), h.getAngle());*/
}

const int DEFENCE_BENEFIT = 3;
const int DEFEND_NEED = 3;
const int LOST_FIGHTER_FIGHTS = 5;
const double FORB_ANGLE = PI / 4;
const double CHASE_ATTACK_NEED = 1 / 20.0;
const double CHASE_NEED = 5;
const double CRITICAL_POS_SHIFT = 150;
const double CRITICAL_DANGER_TO_FIGHT = 2;

void ChaseFight(MyHockeyist &h)
{
	if (h.state == "substitute" && CanBeSubstituted(h))
	{
		Substitute(h);
	}

	if (h.state == "fighter")
	{
		if (CanStrikePuck(h) && !WillBeGoalToMe(h) &&
			((IsMyPart(W.getPuck().getX()) &&
			(!CanStrikeHockeyist(h, en_leader) || Fabs(Norm(h.getAngleTo(en_leader) - en_leader.angleTo(MyGoalNetTarget()))) > FORB_ANGLE))
			|| WillBeGoalToEnemy(h)))  // Бьёт
		{
			if (WillBeGoalToEnemy(h))
				cerr << "STRIKE as there will be goal" << endl;
			h.answer.action = STRIKE;
		}
		else if (CanPickPuck(h))  // или пытается отобрать
		{
			h.answer.action = TAKE_PUCK;
		}
	}
	else if (!CanStrikePuck(h) || !WillBeGoalToMe(h))
	{
		if (CanStrikePuck(h) || (h.state == "striker" && CanStrikeHockeyist(h, en_leader) &&
			(IsMyPart(W.getPuck().getX() + int_IsLeft * CRITICAL_POS_SHIFT) || DefenceTime < CRITICAL_DANGER_TO_FIGHT)))
			h.answer.action = STRIKE;
		else if ((h.state == "defender" || h.state == "defender ca") && 
			(DefenceTime >= G.getDefaultActionCooldownTicks() + (G.getPassSector() / 2) / en_leader.MaxTurnAngle()))
		{
            bool me = false;
            bool him = false;
			for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
            {
				if (!IT->second.isTeammate() && 
					CanStrikeHockeyist(h, IT->second) && IT->second.getState() != RESTING)
                {
					him = true;	
                }
				else if (IT->second.isTeammate() && 
					CanStrikeHockeyist(h, IT->second) && IT->second.getState() != RESTING)
                {
                    me = true;
                }
            }

            if (him && !me)
            {
				cerr << h.getTeammateIndex() + 1 << ") decided to STRIKE as there is one more defender" << endl;
                h.answer.action = STRIKE;
            }
		}
	}
}
void CMakeAnswer(MyHockeyist &h)
{
	variants.clear();
	
	variants.push_back(Variant(h, h.Target.x, h.Target.y, "fighter", false, 1 + (CHASE_NEED - 1) * bestpucktime / h.Target.PuckTime));

    if (h.maintask_state != "fighter")
	    DefendNet(h, 0, DEFEND_NEED);

	ExchangeHockeyists(h);

	GoAttack(h, CHASE_ATTACK_NEED);

	if (DefenceTime != INF ||
		((DefenceTime < h.Target.PuckTime) && (h.maintask_state == "fighter")))
	{
		variants.push_back(Variant(h, en_leader.getX(), en_leader.getY(), "striker", false,
			1 + (h.maintask_state == "fighter") * (LOST_FIGHTER_FIGHTS - 1)));
	}
	else
	{
		DefendNet(h, 1);
	}

	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
	if (!IT->second.isTeammate() && IT->second.getState() != RESTING)
		PassBreak(h, en_leader, IT->second); // Перехват паса

	ChooseBestVariant(h);
}

void Chase()
{
	hc[W.getPuck().getOwnerHockeyistId()].state = "enemy leader";
	en_leader = hc[W.getPuck().getOwnerHockeyistId()];

	vector <Hockeyist> H = W.getHockeyists();
	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
		if (IT->second.isTeammate() && IT->second.getState() != RESTING)
			GetTimeAndTarget(IT->second);

	CStatings();  // Распределяем статусы
	DefenceStates();

	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
	if (IT->second.isTeammate() && IT->second.getState() != RESTING)
			CMakeAnswer(IT->second); // Принимаем решение

    for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
	if (IT->second.isTeammate() && IT->second.getState() != RESTING)
			ChaseFight(IT->second); // Принимаем решение    
}