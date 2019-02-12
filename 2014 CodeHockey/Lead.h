#pragma once
#include "Chase.h"

const int BENEFIT_FOR_GIVING_PASS = 5;
const double PASS_ITERATION = PI / 90;
const double PASS_HQ_ITERATION = PI / 1440;

int Danger = INF;
bool AreThereEnemies;
int DefendersNum;

int AttackTime = INF;

vector <PassInformation> passes;
double best_benefit;
int best_pass;

const int MAX_TO_RUN_FROM = 100;
const int MIN_TO_RUN_FROM = 10;

const int DANGER_TO_STRIKE = 7;

const double GOALIE_DISTANCE = 250;
const double DIAPAZONE = PI / 80;
const double NEAR_GOALIE_DIAPAZONE = 60;
void ProcessPass(double ang, double power)
{
	PassInformation pass = Benefit(leader, ang, power);
	
	if (pass.benefit > best_benefit)  // Обновляем данные
	{
		best_benefit = pass.benefit;
		passes.clear();
		passes.push_back(pass);
	}
	if (pass.benefit == best_benefit)  // Обновляем данные
	{
		passes.push_back(pass);
	}
}
void GetPassInformation()
{
	passes.clear();
	passes.push_back(PassInformation());
	best_benefit = -INF;
	best_pass = 0;
	if (leader.getRemainingCooldownTicks() != 0)
		return;

	for (double ang = -G.getPassSector() / 2; ang <= G.getPassSector() / 2; ang += PASS_ITERATION)
	{
		for (double power = 0.2; power <= 1; power += 0.2)
		{
			ProcessPass(ang, power);
		}		
	}

	double tar_angle = leader.angleTo(EnemyGoalNetTarget());
	for (double ang = tar_angle - DIAPAZONE; ang <= tar_angle + DIAPAZONE; ang += PASS_HQ_ITERATION)
	{
		if (Fabs(ang) <= G.getPassSector() / 2)
			ProcessPass(ang, 1.0);
	}

	if (passes.size() != 0 && best_benefit == INF)
	{
		double strongest = 0;
		for (int i = 0; i < (int)passes.size(); ++i)
		if (passes[i].power > strongest)
			strongest = passes[i].power;

		for (int i = 0; i < (int)passes.size(); ++i)
		{
			if (passes[i].power < strongest - 0.01)
			{
				passes.erase(passes.begin() + i);
				--i;
			}
		}
	}

	best_pass = (int)passes.size() / 2;
	Benefit(leader, passes[best_pass].angle, passes[best_pass].power);  // Ещё раз лучшее, чтобы записать в секции паса данные цели
}
void DiscussEnemy(MyHockeyist &h)
{
	h.Target.PuckTime = max(min(TimeToGetToTarget(h, puck.getX(), puck.getY(), h.getX(), h.getY(), h.getAngle()), TimeToGetToTarget(h, leader.getX(), leader.getY(), h.getX(), h.getY(), h.getAngle())),	h.getRemainingCooldownTicks());

	if (h.DistanceTo(EnemyOppositeNetRink()) <= GOALIE_DISTANCE &&
		h.getY() < G.getGoalNetHeight() + G.getGoalNetTop() + NEAR_GOALIE_DIAPAZONE &&
		h.getY() > G.getGoalNetTop() - NEAR_GOALIE_DIAPAZONE)
	{
		h.state = "defender";
		h.maintask_state = "defender";
        ++DefendersNum;
	}
	else if ((h.getDistanceTo(leader) <= leader.getDistanceTo(h.getX() + h.getSpeedX(), h.getY() + h.getSpeedY()) &&
		leader.getDistanceTo(h) > GOALIE_DISTANCE) || h.Target.PuckTime > MAX_TO_RUN_FROM)
	{
		h.state = "outsider";
		h.maintask_state = "outsider";
	}
	else
	{
		AreThereEnemies = true;
		h.state = "enemy";
		h.maintask_state = "enemy";
	}

	if (h.Target.PuckTime <= MIN_TO_RUN_FROM)
	{
		AreThereEnemies = true;
		h.maintask_state = "enemy";
	}

	if (h.Target.PuckTime < Danger && h.maintask_state == "enemy")
	{
		Danger = h.Target.PuckTime;
	}
}

class IntervalMove
{
public:
	vector <pair<double, double> > forb;
	IntervalMove() {}
	void Add(double a, double b)
	{
		if (a < -PI)
		{
			Add(-PI, b);
			Add(Norm(a), PI);
			return;
		}
		if (b > PI)
		{
			Add(a, PI);
			Add(-PI, Norm(b));
			return;
		}
		if (b < a)
		{
			Add(-PI, b);
			Add(a, PI);
			return;
		}

		forb.push_back(pair<double, double>(a, b));
		int j = (int)forb.size() - 1;
		while (j > 0 && forb[j - 1].first > forb[j].first)
		{
			swap(forb[j], forb[j - 1]);
			--j;
		}

		for (int i = 1; i < (int)forb.size(); ++i)
		{
			if (forb[i].first <= forb[i - 1].second)
			{
				forb[i - 1].second = max(forb[i - 1].second, forb[i].second);
				forb.erase(forb.begin() + i);
				--i;
			}
		}
	}
	double Answer(double V)
	{
		if (forb.size() == 1 && forb[0].first <= -PI && forb[0].second >= PI)
			return -INF;
		if (forb.size() > 0 && forb[0].first <= -PI + 0.0001 && forb[forb.size() - 1].second >= PI - 0.0001)
		{
			forb[0].first = forb[forb.size() - 1].first - 2 * PI;
			forb[forb.size() - 1].second = forb[0].second + 2 * PI;
		}

		int i = 0;
		while (i < (int)forb.size())
		{
			if (forb[i].first <= V && V <= forb[i].second)
			{
				if (Fabs(Norm(V - forb[i].first)) < Fabs(Norm(V - forb[i].second)))
					return forb[i].first;
				else
					return forb[i].second;
			}
			++i;
		}
		return V;
	}
};
class SpecialMove
{
public:
	double up, down, left, right;
	SpecialMove()
	{
		up = 0;
		down = 0;
		left = 0;
		right = 0;
	}
	void Add(double X, double Y)
	{
		if (X > 0)
			right += X;
		if (X < 0)
			left -= X;
		if (Y > 0)
			down += Y;
		if (Y < 0)
			up -= Y;
	}
	void Add(MathVector M)
	{
		Add(M.x, M.y);
	}
	double Absolute(bool AmIUp)
	{
		double ans = 0;
		while (min(up, down) > 0 || min(right, left) > 0)
		{
			if (/*right > left || (right == left && */!bool_IsLeft)//)
				right += min(up, down);
			else
				left += min(up, down);
			ans += min(up, down);
			up -= min(up, down);
			down -= min(up, down);

			if (up > down || (up == down && !AmIUp))
				up += min(right, left);
			else
				down += min(right, left);
			ans += min(right, left);
			left -= min(right, left);
			right -= min(right, left);
		}
		return ans;
	}
	MathVector Answer()
	{
		return MathVector(right - left, down - up);
	}
};

double Price(MyHockeyist &h, SpecialMove way, MathVector tar, bool AmIUp)
{
	SpecialMove copy = SpecialMove(way);
    copy.Add(tar - h.position);
	return -copy.Absolute(AmIUp);// *cos(PI - Fabs(Norm(h.angleTo(tar) - (EnemyGoalNetTarget() - tar).angle))) / PI;

	//return PI - Fabs(Norm(way.Answer().angle - (tar - h.position).angle));
}
double Price(MyHockeyist &h, MathVector tar)
{
    int mytime = TimeToGetToTarget(h, tar.x, tar.y, h.getX(), h.getY(), h.getAngle());
    /*if (mytime == 0)
        return INF;
    
    int minenemy = INF;
    for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
    {
		if (!IT->second.isTeammate() && IT->second.getState() != RESTING)
        {
			int time = TimeToGetToTarget(IT->second, tar.x, tar.y, IT->second.getX(), IT->second.getY(), IT->second.getAngle());
            if (time < minenemy)
                minenemy = time;
        }
    }

    return (minenemy - mytime) * cos(PI - Fabs(Norm(h.angleTo(tar) - (EnemyGoalNetTarget() - tar).angle)));*/
	return INF - mytime;
}

MathVector GetMyUpTarget()
{
	int SwingTicks = min(max(0, Danger - AttackTime / 2 - DANGER_TO_STRIKE), G.getMaxEffectiveSwingTicks());
	MathVector ans = EnemyUp(leader, SwingTicks).center();

	if (!IsMyPart(puck.getX()))
	{
		double up_limit = ans.y;
		double down_limit = EnemyUp(leader, SwingTicks).y - G.getPuckBindingRange();

		if (leader.position.y > down_limit)
			ans.y = up_limit;
		else if (leader.position.y < up_limit)
			ans.y = down_limit;
		else
			ans.y = leader.position.y;
	}

	return ans;
}
MathVector GetMyDownTarget()
{
	int SwingTicks = min(max(0, Danger - AttackTime / 2 - DANGER_TO_STRIKE), G.getMaxEffectiveSwingTicks());
	MathVector ans = EnemyDown(leader, SwingTicks).center();

	if (!IsMyPart(puck.getX()))
	{
		double down_limit = ans.y;
		double up_limit = EnemyDown(leader, SwingTicks).y + G.getPuckBindingRange();

		if (leader.position.y > down_limit)
			ans.y = up_limit;
		else if (leader.position.y < up_limit)
			ans.y = down_limit;
		else
			ans.y = leader.position.y;
	}

	return ans;
}

const double CRITIC_DISTANCE = 100;
const int WALL_PROBLEM = 5;
void GoVer2Add(IntervalMove &way, MathVector pos)
{
	MathVector dist = pos - leader.position;
	if (dist.module() > MAX_TO_RUN_FROM * 2 * GET_MID_VEL(leader))
		return;
	
	double key = CRITIC_DISTANCE / dist.module();
	
	double angle = PI / 2;
	if (key < 1)
		angle = asin(key);
	else if (key < 2)
		angle = PI / 2 + asin(key - 1);
	way.Add(dist.angle - angle, dist.angle + angle);
}
void GoVer2(MyHockeyist &h)
{
	IntervalMove way;
	for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
	{
		if (!hcit->second.isTeammate() &&  // Если противник и приближается
			hcit->second.maintask_state == "enemy")
		{
			GoVer2Add(way, hcit->second.position);
		}
	}
	GoVer2Add(way, MathVector(h.getX(), G.getRinkTop()));
	GoVer2Add(way, MathVector(h.getX(), G.getRinkBottom()));
	GoVer2Add(way, MathVector(G.getRinkRight(), h.getY()));
	GoVer2Add(way, MathVector(G.getRinkLeft(), h.getY()));

	MathVector ToWar;
	MathVector ToWarUp = EnemyDown(h).center() - h.position;
	MathVector ToWarDown = EnemyUp(h).center() - h.position;
	if (EnemyDown(h).IsIn(puck.getX(), puck.getY()) || EnemyUp(h).IsIn(puck.getX(), puck.getY()))
	{
		ToWar = EnemyGoalNetTarget() - h.position;
	}
	else
	{
		if (Fabs(way.Answer(ToWarUp.angle) - ToWarUp.angle) ==
			Fabs(way.Answer(ToWarDown.angle) - ToWarDown.angle))
		{
			if (ToWarUp.module() <= ToWarDown.module())
				ToWar = ToWarUp;
			else
				ToWar = ToWarDown;
		}
		if (Fabs(way.Answer(ToWarUp.angle) - ToWarUp.angle) < 
			Fabs(way.Answer(ToWarDown.angle) - ToWarDown.angle))
			ToWar = ToWarUp;
		else
			ToWar = ToWarDown;
	}
	double ans = way.Answer(ToWar.angle);
	if (ans == -INF)
	{
		if (ToWarUp.module() <= ToWarDown.module())
			ans = ToWarUp.angle;
		else
			ans = ToWarDown.angle;
	}

	h.Target.x = h.getX() + GO_TO_WAR * cos(ans);
	h.Target.y = h.getY() + GO_TO_WAR * sin(ans);
	GoTo(h, h.Target.x, h.Target.y);
}
void GoVer1(MyHockeyist &h, bool FromEnemies, bool GoToWar)
{
	bool AmIUp = h.getY() >= (G.getRinkTop() + G.getRinkBottom()) / 2.0;
	SpecialMove way;

	if (FromEnemies)
	{
		for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
		{
			if (!hcit->second.isTeammate() &&  // Если противник и приближается
				hcit->second.maintask_state == "enemy")
			{
				MathVector dist = h.position - hcit->second.position;
				dist *= (RUN_FROM_HOCKEYISTS * (max(0, MAX_TO_RUN_FROM - (double)hcit->second.Target.PuckTime)) / MAX_TO_RUN_FROM) / dist.module();
				way.Add(dist);
			}
		}
	}

	way.Absolute(AmIUp);

	if (GoToWar)
	{
		MathVector ToWar;
		if (!DoesGoalieExist)
		{
			ToWar = EnemyGoalNetTarget() - h.position;
		}
		else
		{
			MathVector ToWarUp = GetMyUpTarget();
			MathVector ToWarDown = GetMyDownTarget();

			if (Price(h, way, ToWarUp, AmIUp) > Price(h, way, ToWarDown, AmIUp))
				ToWar = ToWarUp - h.position;
			else
				ToWar = ToWarDown - h.position;
		}

		//ToWar *= WALL_PROBLEM;
		way.Add(ToWar * (GO_TO_WAR / ToWar.module()));
	}

	way.Absolute(AmIUp);

	h.Target.x = h.getX() + way.right - way.left;
	h.Target.y = h.getY() + way.down - way.up;
	TargetCorrection(h, h.Target.x, h.Target.y);
	GoTo(h, h.Target.x, h.Target.y);
}

const double CORRECT_ANGLE_DURING_NOT_OVERTIME = PI / 270;
const double CORRECTION = 0.001;
const double ATTACK_ANGLE = PI / 40;
const int FLYING_PUCK_TIME = 10;
const int DANGER_LIMIT_TRY = 15;
const double BEST_RADIUS = 475;
void Attack(MyHockeyist &h)
{
	MathVector target = EnemyGoalNetTarget();
	double tar_ang = h.getAngleTo(target.x, target.y);
	double ra = Norm(h.getAngle() + tar_ang);

	double CORRECT_ANGLE = CORRECT_ANGLE_DURING_NOT_OVERTIME;
	if (!DoesGoalieExist)
	{
		CORRECT_ANGLE = Fabs(Norm(tar_ang - h.angleTo(EnemyGoalNetTarget(false))));
	}

	AttackTime = max(floor(max(0, Fabs(h.getAngleTo(target.x, target.y)) - CORRECT_ANGLE) / h.MaxTurnAngle()),
		h.getRemainingCooldownTicks());
	
	int SwingTicks = min(max(0, Danger - AttackTime / 2 - G.getSwingActionCooldownTicks() - DANGER_TO_STRIKE) + h.getSwingTicks(), G.getMaxEffectiveSwingTicks());
	if (SwingTicks > 0)
		SwingTicks += G.getSwingActionCooldownTicks();

	AttackTime += SwingTicks - h.getSwingTicks();

	if (EnemyDown(h, SwingTicks).IsIn(RightPuckPos(h, ra)) || EnemyUp(h, SwingTicks).IsIn(RightPuckPos(h, ra))
		|| h.getState() == SWINGING || !DoesGoalieExist)
	{
		if (AttackTime > Danger + DANGER_LIMIT_TRY && h.getState() != SWINGING)
		{
			GoVer1(h, true, false);
			cerr << "no attack as I have no time" << endl;
			return;
		}
		if (h.getState() != SWINGING && AttackTime + DANGER_TO_STRIKE < Danger &&
			!Circle(target.x, target.y, BEST_RADIUS).IsIn(RightPuckPos(h, ra)))
		{
			MathVector gotar;
			if (EnemyDown(h, SwingTicks).IsIn(RightPuckPos(h, ra)))
			{
				gotar = EnemyDown(h, SwingTicks).center();
			}
			else
			{
				gotar = EnemyUp(h, SwingTicks).center();
			}
			GoTo(h, gotar.x, gotar.y);

			cerr << "have better chances in future" << endl;
			return;
		}
		GoVer1(h, true, true);

		MathVector pos = h.position;
		MathVector speed = h.speed;
		for (int i = h.getSwingTicks() + 1; i <= SwingTicks; ++i)
		{
			double y = INF;
			Goals g;
			MovePuck(h, HOCKEYIST_SPEED_LOWING_COEFF, pos.x, pos.y, speed.x, speed.y, y, g, false);
		}
		tar_ang = Norm((target - pos).angle - h.getAngle() - h.getAngularSpeed() * (SwingTicks - h.getSwingTicks()));

		if (Fabs(tar_ang) <= CORRECT_ANGLE || h.getState() == SWINGING)
		{
			cerr << "RIGHT ANGLE ACHIEVED" << endl;
			pos = h.position;
			speed = h.speed;
			bool Possible = false;
			for (int i = h.getSwingTicks() + 1; i <= SwingTicks && !Possible; ++i)
			{
				double y = INF;
				Goals g;
				MovePuck(h, HOCKEYIST_SPEED_LOWING_COEFF, pos.x, pos.y, speed.x, speed.y, y, g, false);
				
				if (((EnemyDown(h, i).IsIn(RightPuckPos(h, ra) + pos - h.position) ||
					EnemyUp(h, i).IsIn(RightPuckPos(h, ra) + pos - h.position))) &&
					Fabs(Norm((target - pos).angle - h.getAngle() - h.getAngularSpeed() * (i - h.getSwingTicks()))) <= CORRECT_ANGLE / 2.0)
				{
					Possible = (i - G.getSwingActionCooldownTicks() > h.getSwingTicks());
					cerr << "Possible attack: " << i << endl;
				}
			}

			bool Now = (EnemyDown(h, h.getSwingTicks()).IsIn(RightPuckPos(h, ra)) ||
				EnemyUp(h, h.getSwingTicks()).IsIn(RightPuckPos(h, ra))
				|| h.getState() == SWINGING) && h.angleTo(target) <= CORRECT_ANGLE + CORRECTION * h.getSwingTicks();

			if (h.getSwingTicks() < G.getMaxEffectiveSwingTicks() &&
				Danger > DANGER_TO_STRIKE && Possible)
				h.answer.action = SWING;
            else if (Now &&
				(Danger <= DANGER_TO_STRIKE || h.getSwingTicks() >= G.getMaxEffectiveSwingTicks() || !Possible))
				h.answer.action = STRIKE;
			else
			{
				cerr << "No attack decision. Going" << endl;
			}
		}

		h.answer.turn = tar_ang;
        if (Fabs(Norm(h.speed.angle - ra)) <= CORRECT_ANGLE || h.speed.module() < 0.01)
			h.answer.speed_up = 1.0;
		else if (h.speed.module() < 0.01)
			h.answer.speed_up = 0;
		else if (Fabs(Norm(h.speed.angle - h.getAngle())) < PI / 2)
			h.answer.speed_up = -1.0;
		else
			h.answer.speed_up = 1.0;
	}
	else
	{
		GoVer1(h, true, true);
		AttackTime = INF;
	}
}
void Pass(MyHockeyist &h)
{
	if (BENEFIT_FOR_GIVING_PASS < best_benefit - Danger || best_benefit == INF)
	{
		if (best_benefit == INF)
		{
			bool Defender = false;
			for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
			{
				if (!IT->second.isTeammate() && IT->second.PassTarget.PuckTime < passes[best_pass].goals.goal_time)
				{
					Defender = true;
				}
			}

			if (Danger > DANGER_TO_STRIKE && 
				passes[best_pass].V.module() <= h.StrikeStrength(Danger - DANGER_TO_STRIKE))
				return;
		}

		cerr << "GIVE PASS: " << passes[best_pass].angle << " " << passes[best_pass].power << " " <<
			best_benefit << " " << Danger << " " << endl;
		h.answer.PassAngle = passes[best_pass].angle;
		h.answer.PassPower = passes[best_pass].power;
		h.answer.action = PASS;
	}
}

void LeaderAction(MyHockeyist &h)
{
	Attack(h);
	if (h.getState() != SWINGING)
		Pass(h);
}

const int GREATNESS_ENOUGH_TO_FIGHT = 10;
void L_HowAboutFight(MyHockeyist &h)
{
	if (h.state == "substitute" && CanBeSubstituted(h))
	{
		Substitute(h);
	}

	if (h.getRemainingCooldownTicks() > 0 || CanStrikePuck(h) || CanStrikeHockeyist(h, leader))
		return;

	int greatness = 0;
	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
	{
		if (!IT->second.isTeammate() && IT->second.getState() != RESTING
			&& CanStrikeHockeyist(h, IT->second))
		{
			if (IT->second.state == "defender")
			{
				greatness += INF;
			}
			else
			{
				PuckSearch ans;
				GetTime(IT->second, IT->second.position.x, IT->second.position.y,
					0, 0, false, IfFighted(h, IT->second) + IT->second.speed, 
					IT->second.getRemainingCooldownTicks(), IT->second.getRemainingKnockdownTicks(), ans, empty);
				greatness += ans.PuckTime - IT->second.Target.PuckTime;
			}
		}
	}

	if (greatness > GREATNESS_ENOUGH_TO_FIGHT)
		h.answer.action = STRIKE;
}

const int DEFENDER_BEAT = 6;
const double ATTACK_NEED = 1 / 5.0;
const double OVERTIME_DEFENCE = 10;
void Act(MyHockeyist &h)
{
	if (h.state == "leader")
	{  // Лидер
		return;
	}
	else  // Иначе защищаем ворота или идём за пасом.
	{
		variants.clear();
		if (AreThereEnemies)
		{
			DefendNet(h, 0, 1 + (OVERTIME_DEFENCE - 1) * (!DoesGoalieExist));
			if (DefendersNum <= 1)
				DefendNet(h, 1);
		}

		GoAttack(h, ATTACK_NEED);
		ExchangeHockeyists(h);

		for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
			if (!IT->second.isTeammate() && IT->second.getState() != RESTING && IT->second.maintask_state != "outsider")
				variants.push_back(Variant(h, IT->second.getX(), IT->second.getY(), "striker", 
					false, 1 + DefendersNum * (DEFENDER_BEAT - 1)*(IT->second.state == "defender")));

		ChooseBestVariant(h);

		L_HowAboutFight(h);
	}
}
void Lead()
{
	AreThereEnemies = false;
    DefendersNum = 0;
	AttackTime = INF;

	hc[W.getPuck().getOwnerHockeyistId()].state = "leader";
	leader = hc[W.getPuck().getOwnerHockeyistId()];

	Danger = INF;
	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
		if (!IT->second.isTeammate() && IT->second.getState() != RESTING)
			DiscussEnemy(IT->second);
	
	GetPassInformation();
	cerr << "PASS BENEFIT: " << best_benefit << endl;
	
	LeaderAction(hc[leader.getId()]);
	for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
		if (IT->second.isTeammate() && IT->second.getState() != RESTING)
			Act(IT->second);
}