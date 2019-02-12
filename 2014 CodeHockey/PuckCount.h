#pragma once
#include "Basic.h"

const int TOO_FAST_SPEED = 8;
const double GOALIE_COLLISION_RESISTANCE = 3;
const double WALL_COLLISION_RESISTANCE = 3;
const double PUCK_RESISTANCE = 0.999;

Goals empty;

const double e = 0.01;
void GoalieYChange(double Y, double &GoalieY)
{
	if (Y - GoalieY > G.getGoalieMaxSpeed())  // Изменение позиции вратарей
		GoalieY += G.getGoalieMaxSpeed();
	else if (GoalieY - Y > G.getGoalieMaxSpeed())
		GoalieY -= G.getGoalieMaxSpeed();
	else
		GoalieY = Y;

	GoalieY = GetGoalieY(GoalieY);
}
bool GoalieCollision(Unit u, double &X, double &Y, double &SpX, double &SpY, double &GoalieY)  // Столкновение шайбы с вратарём!
{
	for (int i = 0; i < 2; ++i)
	{
		double radius = Distance(X, Y, GoalieX[i], GoalieY);
		if (radius <= u.getRadius() + MyGoalie.getRadius())  // Если расстояние меньше радиусов...
		{
			MathVector Vel = MathVector(SpX, SpY);  // Основные два вектора
			MathVector RadVec = MathVector(GoalieX[i] - X, GoalieY - Y);
			if (RadVec.x == 0 && RadVec.y == 0)
			{
				RadVec = Vel;
				X -= SpX;
				Y -= SpY;
			}

			MathVector Reducing = RadVec * ((u.getRadius() + MyGoalie.getRadius()) / RadVec.module() - 1);
			X -= Reducing.x;  // Убираем упругость удара
			Y -= Reducing.y;

			double angle = Norm(Vel.angle - RadVec.angle);  // Угол между векторами
			double projection_module = Vel.module() * cos(angle);  // Направленный модуль проекции

			MathVector Projection = RadVec * (projection_module / RadVec.module());  // Проекция
			MathVector Movement = Projection - Vel;  // Высота
			Movement *= 2;
			MathVector NewVel = (Vel + Movement);  // новая скорость
			if (cos(angle) >= 0)
				NewVel *= (-1);
			else
				NewVel = Vel - Reducing;

			SpX = NewVel.x / GOALIE_COLLISION_RESISTANCE;
			SpY = NewVel.y / GOALIE_COLLISION_RESISTANCE;
			return true;
		}
	}

	return false;
}
bool MovePuck(Unit& u, double RESISTANCE, double &X, double &Y, double &SpX, double &SpY, 
	double &GoalieY, Goals &goals, bool GoalieExists)
{
	X += SpX;  // Двигаем
	Y += SpY;
	SpX *= RESISTANCE;
	SpY *= RESISTANCE;

	if (GoalieExists)  // Учитываем вратарей
		GoalieCollision(u, X, Y, SpX, SpY, GoalieY);

	if (Y + u.getRadius() > G.getRinkBottom() && SpY > 0)  // Стенки
	{
		SpY = (-SpY);
		SpY /= WALL_COLLISION_RESISTANCE;
	}
	if (Y < G.getRinkTop() + u.getRadius() && SpY < 0)
	{
		SpY = (-SpY);
		SpY /= WALL_COLLISION_RESISTANCE;
	}
	if (X < G.getRinkLeft() + u.getRadius() && SpX < 0)
	{
		SpX = (-SpX);
		SpX /= WALL_COLLISION_RESISTANCE;

		if (Y + e >= G.getGoalNetTop() &&  // Фиксация гола: стенка в воротах
			Y - e <= G.getGoalNetTop() + G.getGoalNetHeight() &&
			u.getId() == puck.getId())
		{
			goals.was_left_goal = true;
			return true;
		}
	}
	if (X + u.getRadius() > G.getRinkRight() && SpX > 0)
	{
		SpX = (-SpX);
		SpX /= WALL_COLLISION_RESISTANCE; 

		if (Y + e >= G.getGoalNetTop() &&
			Y - e <= G.getGoalNetTop() + G.getGoalNetHeight() &&
			u.getId() == puck.getId())
		{
			goals.was_right_goal = true;
			return true;
		}
	}

	return false;  // Возвращаем был ли гол.
}

const int TIMES_TO_BE_SURE = 1;
const double SLOW_GO = 4;
const int TOO_LONG_PUCK_SEARCH = 400;
double TimeToGetToTarget(MyHockeyist h, double tar_x, double tar_y, double X, double Y, double angle, bool IsPass = false)
{
	double time1 = max((Fabs(Norm(MathVector(tar_x - X, tar_y - Y).angle - angle)) - G.getStickSector() / 2.0) / h.MaxTurnAngle(),
		(Distance(X, Y, tar_x, tar_y) - G.getStickLength()) / GET_MID_VEL(h) * (1 + (SLOW_GO - 1) * IsPass));
	double time2 = max((Fabs(Norm(MathVector(tar_x - X, tar_y - Y).angle - angle + PI)) - G.getStickSector() / 2.0) / h.MaxTurnAngle(),
		(Distance(X, Y, tar_x, tar_y) + 1) / GET_MID_BACK_VEL(h) * (1 + (SLOW_GO - 1) * IsPass));

	return max(0, min(time1, time2));
}
void GetTime(MyHockeyist &h, double puck_x, double puck_y, double goaliey, double X, double Y, double SpX, double SpY, bool must_be_beaten, 
	MathVector V, int how_much_cooldown, int how_much_knocked, PuckSearch &ans, Goals &goals, bool IsPass = false)
{
	ans.x = puck_x;
	ans.y = puck_y;
	ans.PuckTime = 0;
	ans.V.x = SpX;
	ans.V.y = SpY;
	double GoalieY = goaliey;

	bool GoalieAlwaysExists = W.getMyPlayer().getGoalCount() != 0 || W.getOpponentPlayer().getGoalCount() != 0;
	bool GoalieExists = GoalieAlwaysExists || tick <= G.getTickCount();
	while (!((!must_be_beaten || ans.V.module() <= TOO_FAST_SPEED) && // ещё не было удара
		ans.PuckTime >= how_much_cooldown && // ещё не может сделать действия
		TimeToGetToTarget(h, ans.x, ans.y, X, Y, h.getAngle(), IsPass) <= ans.PuckTime - how_much_knocked))
	{
		++ans.PuckTime;  // переходим к следующему тику
		if (ans.PuckTime >= TOO_LONG_PUCK_SEARCH)
		{
			ans.x = puck_x;
			ans.y = puck_y;
			ans.PuckTime = INF;
			return;
		}

		GoalieYChange(ans.y, GoalieY);

		GoalieExists = GoalieAlwaysExists || (GoalieExists && ((ans.PuckTime + tick) <= G.getTickCount()));
		MovePuck(MyGoalie, HOCKEYIST_SPEED_LOWING_COEFF, X, Y, V.x, V.y, GoalieY, goals, GoalieExists);

		if (MovePuck(puck, PUCK_RESISTANCE, ans.x, ans.y, ans.V.x, ans.V.y, GoalieY, goals, GoalieExists))
		{
			goals.GoalsCount(ans.PuckTime);  // иначе записываем момент гола и 
			ans.x = puck_x;
			ans.y = puck_y;
			ans.PuckTime = INF;
			return;
		}
	}
}
void GetTime(MyHockeyist &h, double X, double Y, double SpX, double SpY, bool must_be_beaten,
	MathVector V, int how_much_cooldown, int how_much_knocked, PuckSearch &ans, Goals &goals, bool IsPass = false)
{
	GetTime(h, puck.getX(), puck.getY(), MyGoalie.getY(), X, Y, SpX, SpY, 
		must_be_beaten, V, how_much_cooldown, how_much_knocked, ans, goals, IsPass);
}

void GetTime(MyHockeyist &h, double SpX, double SpY, bool must_be_beaten, Goals &goals = empty)
{
	GetTime(h, h.getX(), h.getY(), SpX, SpY, must_be_beaten, h.speed, 
		h.getRemainingCooldownTicks(), h.getRemainingKnockdownTicks(), h.Target, goals);
}
void GetTime(MyHockeyist &h, Goals &goals = empty)
{ 
	GetTime(h, h.getX(), h.getY(), W.getPuck().getSpeedX(), W.getPuck().getSpeedY(), 
		false, h.speed, h.getRemainingCooldownTicks(), h.getRemainingKnockdownTicks(), h.Target, goals);  // Получаем данные
}
void GetPassTime(MyHockeyist &h, double SpX, double SpY, int AddedCooldown, Goals &goals = empty)
{
	GetTime(h, h.getX(), h.getY(), SpX, SpY, h.isTeammate(), h.speed, 
		h.getRemainingCooldownTicks() + AddedCooldown, h.getRemainingKnockdownTicks(), h.PassTarget, goals,
		h.isTeammate() && h.state != "leader");  // Аналогично с пасом
}

const double DECONTROLED_SUSPENCE = 10;
int GetTime(MyHockeyist &h, MathVector V, int how_much_knocked, Goals &goals = empty)
{
	PuckSearch ans;
	GetTime(h, h.getX(), h.getY(), W.getPuck().getSpeedX(), W.getPuck().getSpeedY(), 
		false, h.speed + V, h.getRemainingCooldownTicks(),
		how_much_knocked * (h.isTeammate()) + h.getRemainingKnockdownTicks(), ans, goals);
	return ans.PuckTime;
}

const int FOR_FLYING = 100;
bool WillBeGoal(MyHockeyist &h, double Angle, int SwingTicks, Goals &g = empty)
{
	double goalie = MyGoalie.getY();

	double angle = Norm(Angle + h.getAngleTo(puck));
	double Module = h.StrikeStrength(SwingTicks) + h.speed.module() * cos(angle - h.speed.angle);

	double SpX = Module * cos(angle);  // Вычисляем каждую часть
	double SpY = Module * sin(angle);

	MathVector puck_pos = MathVector(puck.getX(), puck.getY());

	PuckSearch ans;
	g.Clear();
	GetTime(h, puck_pos.x, puck_pos.y, goalie, h.getX(), h.getY(), SpX, SpY, // вообще-то angle у h другой, но тут это неважно
		false, h.speed, FOR_FLYING, FOR_FLYING, ans, g);
	return g.was_left_goal || g.was_right_goal;
}
bool WillBeGoal(MyHockeyist &h) { return WillBeGoal(h, h.getAngle(), h.getSwingTicks()); }
bool WillBeGoal(MyHockeyist &h, Goals &g) { return WillBeGoal(h, h.getAngle(), h.getSwingTicks(), g); }
bool WillBeGoalToMe(MyHockeyist &h)
{ 
	Goals g;
	return WillBeGoal(h, g) && g.Decrement != -1;
}
bool WillBeGoalToEnemy(MyHockeyist &h, double angle, int SwingTicks)
{
	Goals g;
	return WillBeGoal(h, angle, SwingTicks, g) && g.Increment != -1;
}
bool WillBeGoalToEnemy(MyHockeyist &h)
{
	return WillBeGoalToEnemy(h, h.getAngle(), h.getSwingTicks());
}
bool WillBeGoalToEnemy(MyHockeyist &h, double angle)
{
	return WillBeGoal(h, angle, h.getSwingTicks());
}
bool WillBeStrikenGoalToEnemy(MyHockeyist &h)
{
	return WillBeGoalToEnemy(h, h.getAngle(), G.getMaxEffectiveSwingTicks());
}

const int ATTACK_PROGRESS = 90;
const double SLOWEST_GOAL_VELOCITY = 12;
PassInformation Benefit(MyHockeyist &h, double angle, double pass_power)
{
	PassInformation ans = PassInformation(h, angle, pass_power);

	int my_min = INF;
	int enemy_min = INF;
	int enemy_slow_min = INF;

	long long int my_min_id = h.getId();
	long long int my_personal;
	for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
	{
		if (hcit->second.getState() != RESTING && hcit->second.getType() != GOALIE)
		{
			GetPassTime(hcit->second, ans.V.x, ans.V.y,
				(hcit->second.getId() == h.getId()) * G.getDefaultActionCooldownTicks(), ans.goals);  // Считаем время
			if (hcit->second.getId() != h.getId())
			{
				if (hcit->second.PassTarget.PuckTime > 500 && hcit->second.PassTarget.PuckTime != INF)
				{
					cerr << "TOO LONG PUCK SEARCH" << endl;
				}

				if (hcit->second.isTeammate() && hcit->second.PassTarget.PuckTime < my_min)  // обновляем минимумы
				{
					my_min_id = hcit->second.getId();
					my_min = hcit->second.PassTarget.PuckTime;
				}
				if (!hcit->second.isTeammate() && hcit->second.PassTarget.PuckTime < enemy_min)
					enemy_min = hcit->second.PassTarget.PuckTime;
				if (!hcit->second.isTeammate() && hcit->second.PassTarget.PuckTime < enemy_slow_min &&
					hcit->second.PassTarget.V.module() <= SLOWEST_GOAL_VELOCITY)
					enemy_slow_min = hcit->second.PassTarget.PuckTime;
			}
			else
			{
				my_personal = hcit->second.PassTarget.PuckTime;
			}
		}
	}

	if (ans.goals.Decrement != -1)
		return ans; // Если нам забивается гол
	if (ans.goals.Increment != -1 && enemy_slow_min > ans.goals.goal_time)
	{
		ans.benefit = INF;
		return ans;
	}
	if (my_personal < my_min)
		return ans;

	ans.benefit = enemy_min - my_min  // очевидное возвращение
		- G.getDefaultActionCooldownTicks()  // Штраф за то, что какое-то время мы не будем иметь возможности что-то делать. Все мы.
		+ (my_min < enemy_min) * (ATTACK_PROGRESS * 2 / W.getWidth()) * (GetAttackProcess(h, puck.getX(), puck.getY())
		- GetAttackProcess(hc[my_min_id], hc[my_min_id].PassTarget.x, hc[my_min_id].PassTarget.y));  // Продвижение атаки.
	return ans;
}