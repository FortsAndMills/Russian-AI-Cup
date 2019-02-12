#pragma once
#include "ConstantsAndElementary.h"

const double INERTION_DISTANCE = 120;
const double NEAR = 30;

double HOCKEYIST_SPEED_LOWING_COEFF = 0.98;
double SPEED_TO_DISTANCE = 45;

void TargetCorrection(MyHockeyist h, double &x, double &y)
{
	double r = 4 * MyGoalie.getRadius();
	if (x < G.getRinkLeft() + r)
	{
		if (y < h.getY())
			y -= G.getRinkLeft() + r - x;
		else
			y += G.getRinkLeft() + r - x;
		x = G.getRinkLeft() + r;
	}
	if (x + r > G.getRinkRight())
	{
		if (y < h.getY())
			y += G.getRinkRight() - x - r;
		else
			y -= G.getRinkRight() - x - r;
		x = G.getRinkRight() - r;
	}
	if (y < G.getRinkTop() + r)
	{
		if (x < h.getX())
			x -= G.getRinkTop() + r - y;
		else
			x += G.getRinkTop() + r - y;
		y = G.getRinkTop() + r;
	}
	if (y > G.getRinkBottom() - r)
	{
		if (x < h.getX())
			x += G.getRinkBottom() - r - y;
		else
			x -= G.getRinkBottom() - r - y;
		y = G.getRinkBottom() - r;
	}
}

double TOO_BIG_ANGLE = PI / 2;
double SMALL_PUCK_ANGLE = PI / 4;
double TOO_BIG_DISTANCE = 250;
void GoTo(MyHockeyist &h, double X, double Y, bool BackGoingAllowed = false)
{
	if (h.getDistanceTo(X, Y) == 0)
	{
		h.answer.turn = h.getAngleTo(W.getPuck().getX(), W.getPuck().getY());
		h.answer.speed_up = 0;
		return;
	}

	h.answer.turn = h.getAngleTo(X, Y);

	if (Fabs(h.answer.turn) >= TOO_BIG_ANGLE)
	{
		h.answer.speed_up = -1.0;
	}
	else
	{
		h.answer.speed_up = 1.0;
	}

	if (Fabs(Norm(h.answer.turn - h.getAngleTo(puck))) >= TOO_BIG_ANGLE && 
		BackGoingAllowed && h.getDistanceTo(X, Y) < TOO_BIG_DISTANCE)
	{
		h.answer.turn = Norm(h.answer.turn + PI);
		cerr << h.getTeammateIndex() + 1 << " going back and -1" << endl;
	}
}
void GoSlowlyTo(MyHockeyist &h, double tar_x, double tar_y)
{
	double inertion = h.speed.module() * SPEED_TO_DISTANCE;

	GoTo(h, tar_x, tar_y, true);
	
	double D = h.getDistanceTo(tar_x, tar_y);
	if (max(D, NEAR) <= inertion)
	{
		if (h.speed.module() < 0.01)
			h.answer.speed_up = 0;
		else if (Fabs(Norm(h.speed.angle - h.getAngle())) < PI / 2)
			h.answer.speed_up = -1.0;
		else
			h.answer.speed_up = 1.0;
	}
	else if (D < NEAR)
	{
		if (h.state != "attack" && (state != "lead" || Fabs(h.getAngleTo(W.getPuck().getX(), W.getPuck().getY())) > SMALL_PUCK_ANGLE))
			h.answer.turn = h.getAngleTo(W.getPuck().getX(), W.getPuck().getY());
        if (h.state == "attack")
			h.answer.turn = h.angleTo(EnemyGoalNetTarget());
		h.answer.speed_up = 0;
	}
}
void GoDefenging(MyHockeyist &h, double tar_x, double tar_y)
{
	GoSlowlyTo(h, tar_x, tar_y);

	/*double D = h.getDistanceTo(tar_x, tar_y);

	if (D > max(NEAR, pow(h.speed.module(), 3)) &&  // Если мы далеко, но не очень
		D <= TOO_BIG_DISTANCE)
	{
		GoTo(h, tar_x, tar_y, ((h.getX() <= tar_x) ^ bool_IsLeft));  // запрещаем backgo, если мы по краям
		h.answer.speed_up *= (D - pow(h.speed.module(), 3)) / INERTION_DISTANCE;
	}
	else if (D <= max(NEAR, pow(h.speed.module(), 3)))
	{
		//GoTo(h, tar_x, tar_y, true);
		//h.answer.speed_up /= INERTION_DISTANCE;

		h.answer.turn = h.getAngleTo(W.getPuck().getX(), W.getPuck().getY());
	}
	else
	{  // Например, если мы очень далеко.
		GoTo(h, tar_x, tar_y);
	}*/
}

int GO_TO_WAR = 5;
int RUN_FROM_HOCKEYISTS = 5;

double MISTAKE = 3;
Trapetion getTrap(double maxpower, bool left, bool up)
{
	double R = W.getHockeyists()[0].getRadius();
	double r = W.getPuck().getRadius();
	double key = (G.getGoalNetHeight() - R) * sqrt(pow((maxpower - MISTAKE) / G.getGoalieMaxSpeed(), 2) - 1);

	if (left && up)
	{
		Line A = Line(G.getRinkLeft(), G.getGoalNetTop() + G.getGoalNetHeight() - R, G.getRinkLeft() + key, G.getGoalNetTop() - R);
		Line SecA = Line(G.getRinkLeft(), G.getGoalNetTop() + G.getGoalNetHeight() - R, G.getRinkLeft() + 4 * R, G.getGoalNetTop() - R);
		return Trapetion(A, SecA, G.getGoalNetTop(), false, true);
	}
	if (left && !up)
	{
		Line B = Line(G.getRinkLeft(), G.getGoalNetTop() + R, G.getRinkLeft() + key, G.getGoalNetTop() + G.getGoalNetHeight() + R);
		Line SecB = Line(G.getRinkLeft(), G.getGoalNetTop() + R, G.getRinkLeft() + 4 * R, G.getGoalNetTop() + G.getGoalNetHeight() + R);
		return Trapetion(B, SecB, G.getGoalNetTop() + G.getGoalNetHeight(), true, true);
	}
	if (!left && up)
	{
		Line C = Line(G.getRinkRight(), G.getGoalNetTop() + G.getGoalNetHeight() - R, G.getRinkRight() - key, G.getGoalNetTop() - R);
		Line SecC = Line(G.getRinkRight(), G.getGoalNetTop() + G.getGoalNetHeight() - R, G.getRinkRight() - 4 * R, G.getGoalNetTop() - R);
		return Trapetion(C, SecC, G.getGoalNetTop(), false, false);
	}
	if (!left && !up)
	{
		Line D = Line(G.getRinkRight(), G.getGoalNetTop() + R, G.getRinkRight() - key, G.getGoalNetTop() + G.getGoalNetHeight() + R);
		Line SecD = Line(G.getRinkRight(), G.getGoalNetTop() + R, G.getRinkRight() - 4 * R, G.getGoalNetTop() + G.getGoalNetHeight() + R);
		return Trapetion(D, SecD, G.getGoalNetTop() + G.getGoalNetHeight(), true, false);
	}
}
Trapetion EnemyUp(MyHockeyist &h, int SwingTicks = G.getMaxEffectiveSwingTicks())
{
	return getTrap(h.StrikeStrength(SwingTicks), !bool_IsLeft, true);
}
Trapetion EnemyDown(MyHockeyist &h, int SwingTicks = G.getMaxEffectiveSwingTicks())
{
	return getTrap(h.StrikeStrength(SwingTicks), !bool_IsLeft, false);
}
Trapetion MyDown(MyHockeyist &h, int SwingTicks = G.getMaxEffectiveSwingTicks())
{
	return getTrap(h.StrikeStrength(SwingTicks), bool_IsLeft, false);
}
Trapetion MyUp(MyHockeyist &h, int SwingTicks = G.getMaxEffectiveSwingTicks())
{
	return getTrap(h.StrikeStrength(SwingTicks), bool_IsLeft, true);
}
void Starter()  // вызывается один раз в самом начале и должна высчитать константы и прочее
{
	int k = 0;
	vector <Hockeyist> H = W.getHockeyists();
	MyHockeyist empty;
	for (int i = 0; i < (int)H.size(); ++i)
	{
		if (H[i].getType() == GOALIE)  // ищем координаты вратарей и ссылки на них
		{
			GoalieX[k] = H[i].getX();
			++k;
			if (H[i].isTeammate())
			{
				MyGoalie = H[i];
				if (MyGoalie.getX() < W.getWidth() / 2)  // Определяем свою сторону
				{
					int_IsLeft = 1;
					bool_IsLeft = true;
				}
			}
			else
			{
				EnemyGoalie = H[i];
			}
		}
        else
		{
			hc[H[i].getId()] = MyHockeyist(H[i], empty);  // ссылки в перечне
		}
	}

	TEAM_SIZE = min(3, (H.size() - 2) / 2);
	GO_TO_WAR *= TEAM_SIZE;
	if (TEAM_SIZE == 3)
		MID_VEL = 2.8;
}
void Update()  // ежетичные обновления
{
	tick = W.getTick();
	
	vector <Hockeyist> H = W.getHockeyists();
	vector < pair<double, long long int> > marks;
	for (int i = 0; i < (int)H.size(); ++i)
	{
		if (H[i].getType() != GOALIE)  // обновления ссылок
		{
			hc[H[i].getId()] = MyHockeyist(H[i], hc[H[i].getId()]);
			if (H[i].isTeammate())
			{
				marks.push_back(make_pair(hc[H[i].getId()].mark, H[i].getId()));
				
				int j = marks.size() - 1;
				while (j > 0 && marks[j].first > marks[j - 1].first)
				{
					swap(marks[j], marks[j - 1]);
					--j;
				}
			}
		}
		else if (H[i].isTeammate())  // и на вратарей
		{
			MyGoalie = H[i];
		}
		else
		{
			EnemyGoalie = H[i];
		}
	}

	if (TEAM_SIZE > 2)
	{
		best_hock.clear();
		best_hock.push_back(marks[0].second);
		best_hock.push_back(marks[1].second);
		best_hock.push_back(marks[2].second);
	}

	puck = W.getPuck();
	DoesGoalieExist = W.getMyPlayer().getGoalCount() != 0 || W.getOpponentPlayer().getGoalCount() != 0 ||
		tick <= G.getTickCount();
}

const int CORNER_BADNESS = 4;
double GetAttackProcess(MyHockeyist& h, double x, double y)
{
	MathVector tar1_center = EnemyUp(h).center();    
	MathVector tar2_center = EnemyDown(h).center();

	double coeff = 1 + ((x < tar1_center.x) ^ bool_IsLeft) * (CORNER_BADNESS - 1);
	return min(Distance(x, y, tar1_center.x, tar1_center.y),
		Distance(x, y, tar2_center.x, tar2_center.y)) * coeff;
}

void Substitute(MyHockeyist &h)
{
	h.answer.action = SUBSTITUTE;
	h.answer.teammateindex = hc[best_hock[0]].getTeammateIndex();
	cerr << h.getTeammateIndex() << " is substituted to " << h.answer.teammateindex << endl;

	best_hock[0] = h.getId();
	swap(best_hock[0], best_hock[1]);
	swap(best_hock[1], best_hock[2]);
}
bool CanBeSubstituted(MyHockeyist& h)
{
	return h.getY() <= G.getRinkTop() + G.getSubstitutionAreaHeight() &&
		h.speed.module() <= G.getMaxSpeedToAllowSubstitute() &&
		IsMyPart(h.getX()) && h.getRemainingCooldownTicks() == 0 && h.getState() == ACTIVE;
}

long long int IsPlaceTaken(string state, MyHockeyist &h)
{
	map <long long int, MyHockeyist>::iterator IT = hc.begin();
	while (IT != hc.end() &&
		(IT->second.getId() == h.getId() ||
		!IT->second.isTeammate() || IT->second.getState() == RESTING || 
		IT->second.state != state))
		++IT;

	if (IT != hc.end())
		return IT->second.getId();
	else
		return puck.getId();
}

MathVector RightPuckPos(MyHockeyist& h, double right_angle)
{
	return h.position + G.getPuckBindingRange() * MathVector(cos(right_angle), sin(right_angle));
}

MathVector IfFighted(MyHockeyist& h, MyHockeyist& h2)
{
	double Module = G.getStruckHockeyistInitialSpeedFactor() * G.getStrikePowerBaseFactor() * h.strength()
		+ h.speed.module() * cos(h.getAngleTo(h2) - h.speed.angle);

	if (Module < 0)  // Если получилось меньше 0, считаем, что удар странный.
		Module = 0;

	double SpX = Module * cos(h.getAngleTo(h2));  // Вычисляем каждую часть
	double SpY = Module * sin(h.getAngleTo(h2));
	return MathVector(SpX, SpY);
}