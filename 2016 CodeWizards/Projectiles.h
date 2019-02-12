#include "WarFigures.h"

const int TURNING_RANGE_FAR = 50;
const int TURNING_RANGE_NEAR = 30;
class Missile  // класс виртуальной лет€щей ракеты. „ерез врем€ time она нанесЄт damage таргету
{
public:
	int time, damage;
	WarPiece * author;
	WarPiece * target;
	set <WarPiece *> possible_targets;
	double speed;
	bool magical;
	ProjectileType type;
	double R;

	Point pos;
	double angle;
	Point start;
	bool isFriend;

	Missile() {}
	void fillData(WarPiece * author, ProjectileType type)
	{
		if (type == PROJECTILE_DART)
		{
			speed = game->getDartSpeed();
			damage = game->getDartDirectDamage();
			magical = false;
			R = game->getDartRadius();
		}
		else if (type == PROJECTILE_MAGIC_MISSILE)
		{
			speed = game->getMagicMissileSpeed();
			damage = dynamic_cast<WarWizard*>(author)->missileDamage();
			magical = true;
			R = game->getMagicMissileRadius();
		}
		else if (type == PROJECTILE_FROST_BOLT)
		{
			speed = game->getFrostBoltSpeed();
			damage = game->getFrostBoltDirectDamage();
			magical = false;
			R = game->getFrostBoltRadius();
		}
		else if (type == PROJECTILE_FIREBALL)
		{
			speed = game->getFireballSpeed();
			damage = game->getFireballExplosionMaxDamage();
			magical = false;
			R = game->getFireballRadius();
		}

		this->author = author;
		this->type = type;
	}
	Missile(WarPiece * author, WarPiece * target, ProjectileType type, Point pos)
	{
		fillData(author, type);

		this->target = target;
		this->pos = pos;
		this->angle = (target->pos() - pos).angle();
		this->start = pos;
		this->isFriend = !target->isFriend;
		this->time = Max(0.0, sqrt((pos - target->pos()).norm2()) - target->R) / speed;
	}
	Missile(WarPiece * author, WarPiece * target, const Missile &another)
	{
		this->target = target;
		this->author = author;
		this->type = another.type;
		time = another.time;
		damage = another.damage;
		speed = another.speed;
		magical = another.magical;
		pos = another.pos;
		angle = another.angle;
		start = another.start;
		isFriend = another.isFriend;
	}
	
	// создаЄм снар€д по реальной ракете
	Missile(const Projectile & Proj)
	{
		// определ€ем, кто автор ракеты
		int ID = Proj.getOwnerUnitId();
		WarPiece * author = NULL;
		if (Proj.getType() == PROJECTILE_DART && minions.count(ID))
			author = minions[ID];
		else if (Proj.getType() != PROJECTILE_DART && wizards.count(ID))
			author = wizards[ID];
		
		fillData(author, Proj.getType());
		isFriend = Proj.getFaction() == me->faction;

		// определ€ем, откуда стартовала ракетами.
		pos = Point(Proj.getX(), Proj.getY());
		angle = Proj.getAngle();
		start = pos;
		if (author != NULL && author->isVisible())
			start = author->pos();
		else
		{
			Point speedVec = speed * Point(Proj.getAngle());
			do
			{
				start -= speedVec;
			} while (isPointVisible(start));

			if (author != NULL)
			{
				author->madeShot(start);
			}
		}
		
		updateTarget();
	}
	
	// поиск цели реальной ракеты
	Segment getTrajectory() const
	{
		int length = type == PROJECTILE_DART ? game->getFetishBlowdartAttackRange() : dynamic_cast<WarWizard*>(author)->getRange();

		// ƒлина может быть переоценена.
		return Segment(pos, start + length * Point(angle));
	}
	void updateTarget()
	{
		// начинаем искать цель.
		target = NULL;
		possible_targets.clear();

		double best = 10000000;
		Segment trajectory = getTrajectory();

		for (int j = 0; j < pieces.size(); ++j)
		{
			if (pieces[j]->isValid() && pieces[j]->isFriend != isFriend)
			{
				Circle C(pieces[j]->pos(), pieces[j]->R + R);
				if (trajectory.intersects(C))// && !C.isIn(trajectory.A))
				{
					possible_targets.insert(pieces[j]);

					double farness = trajectory.how_far(C);
					if (farness < best)
					{
						farness = best;
						target = pieces[j];
						time = ceil(sqrt((target->pos() - pos).norm2()) / speed);
					}
				}
			}
		}

		if (target != NULL)
		{
			if (type != PROJECTILE_DART && author != me)
			{
				dynamic_cast<WarWizard*>(author)->updateStrategy(target);
			}
			else
			{
				////////////cout << tick << ") strange: target not found!" << endl;
			}
		}
	}
	// обновл€ем, например, позицию, а если надо - цель
	void update(const Projectile & Proj)
	{
		// на вс€кий из-за скиллов
		fillData(author, type);
		pos = Point(Proj.getX(), Proj.getY());
		angle = Proj.getAngle();

		if (target == NULL ||
			(target->pos() - pos + Point(Proj.getSpeedX(), Proj.getSpeedY())).norm2() <= (target->R + R) * (target->R + R))
			updateTarget();
	}

	int fly(int ticks = 1)
	{
		time -= ticks;
		if (time <= 0)
		{
			/* имитируем наши убегушечки
			if (target->id == me->id && me->strategy != WarWizard::NEAREST_STAFF && me->strategy != WarWizard::ILL_STAFF &&
				type != PROJECTILE_DART)
			{
				double optimal_angle = (target->pos() - start).angle();
				optimal_angle -= Sign(normalize(optimal_angle - target->angle)) * PI / 2;
				optimal_angle = normalize(optimal_angle);

				int time_to_turn = ceil(normalize(optimal_angle - me->angle) / target->maxTurnAngle());
				if (dynamic_cast<WarWizard*>(target)->actionCooldownTicks + time_to_turn < game->getWizardActionCooldownTicks() &&
					target->getDist2To(start) <= pow((game->getWizardCastRange() + TURNING_RANGE_FAR) / me->getSpeedCoeff(), 2) &&
					target->getDist2To(start) >= pow(game->getWizardCastRange() / me->getSpeedCoeff(), 2))
				{
					//////cout << "TURN SUPPOSED!" << endl;
					return true;
				}
			}*/

			double dam = target->damage(damage, magical);
			target->life -= dam;

			if (type == PROJECTILE_FROST_BOLT)
				target->frozen = game->getFrozenDurationTicks();
			if (type == PROJECTILE_FIREBALL)
				target->burning.push_back(game->getBurningDurationTicks());

			return dam;
		}
		return 0;
	}
};
map<int, Missile> Rockets;

// ѕроводим эксперимент - от данной ракеты уклон€емс€, ид€ на угол angle
double STEP_ANGLE = PI / 200;
double experimentHatingRockets(Point angle, const Missile & M, const vector<Obstacle> & obstacles)
{
	Point pos = M.pos;
	int time = ceil(M.getTrajectory().len() / M.speed);
	WarWizard phantom(*me);

	double quality = INFINITY;

	for (int i = 0; i < time; ++i)
	{
		phantom.goInDirection(angle);
		Circle new_pos = phantom.imitateMove();
		phantom.x = new_pos.x, phantom.y = new_pos.y;
		phantom.makeTurn();

		pos += M.speed * Point(M.angle);

		quality = Min(quality, sqrt((phantom.pos() - pos).norm2()) - phantom.R - M.R);
		if (quality < 0)
			return -1;		
	}

	if (isSpaceWrong(phantom.pos()) || obscure(phantom.pos(), obstacles) != NOT_FOUND)
		return -1;

	return quality;
}
void HateRockets(const vector <WarWizard *> & enemies)
{
	vector<Obstacle> obstacles = getObstacles(me->R);

	if (me->strategy != WarWizard::NEAREST_STAFF && me->strategy != WarWizard::ILL_STAFF)
	{
		for (WarWizard * enemy : enemies)
		{
			if (enemy->getDist2To(me) <= pow((game->getWizardCastRange() + TURNING_RANGE_FAR) / me->getSpeedCoeff(), 2) &&
				enemy->getDist2To(me) >= pow((game->getWizardCastRange() - TURNING_RANGE_FAR) / me->getSpeedCoeff(), 2))
			{
				// –ассматриваютс€ два варианта оптимальных положений: в одну сторону и в другую
				// ƒл€ каждого высчитываем корректность, то есть можно ли в эту сторону сдвинутьс€ на 2R
				double optimal_angle = normalize((me->pos() - enemy->pos()).angle() + PI / 2);
				Point optimal_pos = me->pos() + 2 * me->R * Point(optimal_angle);
				bool isCorrect = !isSpaceWrong(optimal_pos) && obscure(optimal_pos, obstacles) == NOT_FOUND;

				double optimal_angle2 = normalize((me->pos() - enemy->pos()).angle() - PI / 2);
				Point optimal_pos2 = me->pos() + 2 * me->R * Point(optimal_angle2);
				bool isCorrect2 = !isSpaceWrong(optimal_pos2) && obscure(optimal_pos2, obstacles) == NOT_FOUND;

				// ≈сли первый вариант некорректен или оба варианты корректны, но второй ближе - выбираем второй.
				if (!isCorrect ||
					(isCorrect2 && abs(normalize(optimal_angle2 - me->angle)) < abs(normalize(optimal_angle - me->angle))))
				{
					isCorrect = isCorrect2;
					optimal_angle = optimal_angle2;
					optimal_pos = optimal_pos2;
				}

				// ≈сли выбранный вариант корректен, можно рассмотреть поворот
				if (isCorrect)
				{
					double turn_angle = normalize(optimal_angle - me->angle);

					// ѕоворачиваемс€ только в последний момент
					if (Max(enemy->timeToMissile(me), enemy->tillShoot(me)) <= (ceil)(turn_angle / me->maxTurnAngle()))
					{
						me->move.setTurn(limit(turn_angle, me->maxTurnAngle()));

						drawArrow(me->x, me->y, me->x + 100 * cos(optimal_angle), me->y + 100 * sin(optimal_angle));
					}
				}
			}
		}
	}

	for (auto it : Rockets)
	{
		Missile & M = it.second;

		if (M.possible_targets.count(me) && M.type != PROJECTILE_FIREBALL)
		{
			////////cout << tick << ") POSSIBLE ROCKET AT " << M.pos << ", target is " << M.target->id << endl;

			double best = -1;
			double ans = -179;

			double right_angle = M.angle + PI / 2;
			for (double angle = M.angle - PI / 2; angle < right_angle; angle = angle + STEP_ANGLE)
			{
				double q = experimentHatingRockets(angle, M, obstacles);
				if (q > best)
				{
					best = q;
					ans = angle;
				}
			}

			double q = experimentHatingRockets(right_angle, M, obstacles);
			if (q > best)
			{
				best = q;
				ans = right_angle;
			}

			q = experimentHatingRockets(me->angle, M, obstacles);
			if (q > best)
			{
				best = q;
				ans = me->angle;
			}

			if (ans != -179)
			{
				me->goInDirection(ans);
				drawArrow(me->x, me->y, me->x + 100 * cos(ans), me->y + 100 * sin(ans), 0xFF0000);
			}
		}
	}
}