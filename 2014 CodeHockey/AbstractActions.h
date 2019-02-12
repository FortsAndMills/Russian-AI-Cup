#pragma once
#include "PuckCount.h"

MyHockeyist leader;
MyHockeyist en_leader;

class Variant
{
public:
	double x, y;
	string state;
	bool Slowly;

	double Needness;
    double time;

	Variant()
	{
		Slowly = false;
		x = false;
		y = false;
		state = "nothing";
        time = INF;
	}
	Variant(MyHockeyist& h, double tar_x, double tar_y, string STATE, bool slow, double need = 1, int _time = -1)
	{
		Slowly = slow;
		x = tar_x;
		y = tar_y;
		state = STATE;
		Needness = need;

		time = max(_time, TimeToGetToTarget(h, x, y, h.getX(), h.getY(), h.getAngle()));
	}

	void Realize(MyHockeyist &h)
	{
		cerr << h.getTeammateIndex() + 1 << " decided to: " << state << endl;
		cerr << h.getTeammateIndex() + 1 << " target: " << x << ", " << y << 
			"     distance = " << h.getDistanceTo(x, y) << ", angle = " << h.getAngleTo(x, y) << endl;

		h.state = state;
		h.time_till_task = time;

		if (state == "defender" || state == "defender ca")
			GoDefenging(h, x, y);
		else if (Slowly)
			GoSlowlyTo(h, x, y);
		else
			GoTo(h, x, y);

		cerr << h.getTeammateIndex() + 1 << " answer: " << h.answer.speed_up << ", " << h.answer.turn << endl;
	}
};

vector <Variant> variants;

const double CRITIC_DISTANCE_TO_GOALIE = 2 / 3.0;
const double DEFENDERS_SHIFT[3] = {2.3, 7.9, 15};
const int TASK_TIME_DIFFERENCE = 15;
void DefendNet(MyHockeyist& h, int layer = 0, double need = 1)
{   // Защита ворот!
	if (layer == 1 && IsPlaceTaken("defender", h) == puck.getId())
	{
		return;
	}

	double tar_x = MyGoalie.getX() + int_IsLeft * MyGoalie.getRadius() * DEFENDERS_SHIFT[layer];  // Целевая точка
	double tar_y = (G.getGoalNetTop() * 2 + G.getGoalNetHeight()) / 2;

	if (layer == 0)
		variants.push_back(Variant(h, tar_x, tar_y, "defender", true, need));
	else
		variants.push_back(Variant(h, tar_x, tar_y, "defender ca", true, need));
	
	if (layer == 1 && IsPlaceTaken("defender ca", h) != puck.getId())
	{
		long long int id = IsPlaceTaken("defender ca", h);
		if (hc[id].time_till_task + TASK_TIME_DIFFERENCE < variants[variants.size() - 1].time)
		{
			variants.pop_back();
			return;
		}
	}
	if (layer == 0 && (IsPlaceTaken("defender", h) != puck.getId()))
	{
		long long int id = IsPlaceTaken("defender", h);
		if (hc[id].time_till_task + TASK_TIME_DIFFERENCE < variants[variants.size() - 1].time)
		{
			variants.pop_back();
			return;
		}
	}
}

const double ATTACK_DISTANCE = 358;
const double ATTACK_BASIC_PRICE = 179;
void GoAttack(MyHockeyist &h, double need = 1)
{
	if (IsPlaceTaken("attack", h) != puck.getId())
		return;

	MathVector ToWarUp = EnemyUp(h, 0).center();
	MathVector ToWarDown = EnemyDown(h, 0).center();

	if ((state != "lead" || (ToWarUp - leader.position).module() > ATTACK_DISTANCE) && 
		(state != "chase" || (ToWarUp - en_leader.position).module() > ATTACK_DISTANCE) &&
		(state != "rush" || (ToWarUp - MathVector(puck.getX(), puck.getY())).module() > ATTACK_DISTANCE))
		variants.push_back(Variant(h, ToWarUp.x, ToWarUp.y, "attack", true, need, ATTACK_BASIC_PRICE));
	
	if ((state != "lead" || (ToWarDown - leader.position).module() > ATTACK_DISTANCE) &&
		(state != "chase" || (ToWarDown - en_leader.position).module() > ATTACK_DISTANCE) &&
		(state != "rush" || (ToWarDown - MathVector(puck.getX(), puck.getY())).module() > ATTACK_DISTANCE))
		variants.push_back(Variant(h, ToWarDown.x, ToWarDown.y, "attack", true, need, ATTACK_BASIC_PRICE));
}

const double EXTRA_SUBSTITUTE_TIME = 60;
void ExchangeHockeyists(MyHockeyist &h, double need = 1)
{
	if (W.getHockeyists().size() < 12)
		return;

	int i = 0;
	while (i < 3 && best_hock[i] != h.getId())
		++i;
	if (i < 3)
		return;

	MathVector target = MathVector(h.getX(), G.getRinkTop() + G.getSubstitutionAreaHeight() - NEAR);
    if (h.getY() <= G.getRinkTop() + G.getSubstitutionAreaHeight() && IsMyPart(h.getX()))
        target = h.position;
	if (!IsMyPart(h.getX() + NEAR * int_IsLeft))
		target.x = (G.getRinkLeft() + G.getRinkRight()) / 2.0 - NEAR * int_IsLeft;

	variants.push_back(Variant(h, target.x, target.y, "substitute", true, need, h.getRemainingCooldownTicks() + 
		EXTRA_SUBSTITUTE_TIME * (1 + h.mark - hc[best_hock[0]].mark)));
}

const double MIN_DISTANCE_TO_PASS = 400;
void PassBreak(MyHockeyist& h, MyHockeyist pass_from, MyHockeyist pass_to)
{   // Перехват паса
	if (IsPlaceTaken("pass breaker", h) != puck.getId())
		return;
	if (pass_from.getDistanceTo(pass_to) <= MIN_DISTANCE_TO_PASS)
		return;

	MathVector Line = MathVector(pass_from.getX() - pass_to.getX(), pass_from.getY() - pass_to.getY());
	Line /= 2;

	variants.push_back(Variant(h, pass_to.getX() + Line.x, pass_to.getY() + Line.y, "pass breaker", true));
}

const double ADDED_TO_ATTACK = 100;
void ChooseBestVariant(MyHockeyist &h)
{   // Защита ворот!
	int best = -1;
	double best_dist = INF;
	for (int i = 0; i < (int)variants.size(); ++i)
	{
		double greatness = variants[i].time / variants[i].Needness;
		if (variants[i].state == "attack")
		{
			greatness += ADDED_TO_ATTACK;
		}

		if (greatness < best_dist)
		{
			best_dist = greatness;
			best = i;
		}
	}

	if (best != -1)
		variants[best].Realize(h);
	else
		cerr << h.getTeammateIndex() + 1 << ") NO IDEA" << endl;
}