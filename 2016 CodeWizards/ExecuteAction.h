#include "Projectiles.h"

int UsesOfStaff = 0;

// проверяем, что ни один нейтрал не под ударом
bool NeutralsNotUnderStaff()
{
	for (Minion M : world->getMinions())
	{
		if (M.getFaction() == FACTION_NEUTRAL)
		{
			Obstacle ob(Point(M.getX(), M.getY()), M.getRadius(), MINION);
			if (me->canStaff(&ob))
				return false;
		}
	}
	return true;
}
// проверяем корректность точки - она не должна выходить за пределы карты, быть деревом или башней.
bool isCorrect(Point & P)
{
	if (P.x <= self->getRadius())
		P.x = self->getRadius();
	if (P.x >= 4000 - self->getRadius())
		P.x = 4000 - self->getRadius();
	if (P.y <= self->getRadius())
		P.y = self->getRadius();
	if (P.y >= 4000 - self->getRadius())
		P.y = 4000 - self->getRadius();

	for (auto T : trees)
	{
		if (Circle(T.second->pos(), T.second->R + game->getWizardRadius()).isIn(P))
			return false;
	}
	for (auto t : pieces)
	{
		if (t->isFriend && t->id != me->id && Circle(t->pos(), t->R + game->getWizardRadius()).isIn(P))
			return false;
	}
	return true;
}

// постреливаем в деревья, пока идём
void WoodcutterHelp(const vector<Obstacle> & obstacles)
{
	for (Obstacle obstacle : obstacles)
	{
		// TODO более грамотная проверка нужна.
		// на самом деле можно рубить любое дерево, которое попалось под руку.
		// также требуется проверка, не находимся ли мы в опасной зоне, где тратить ману/действия уже не надо.
		if (obstacle.allowed_sign != 0 && obstacle.type == TREE)
		{
			if (me->canStaff(&obstacle) && NeutralsNotUnderStaff())
			{
				me->move.setAction(ACTION_STAFF);
				////////////////cout << tick << ") Cut tree!" << endl;
			}
			else if (me->canAttack(&obstacle))
			{
				me->move.setMaxCastDistance(self->getDistanceTo(obstacle.x, obstacle.y));
				me->move.setMinCastDistance(self->getDistanceTo(obstacle.x, obstacle.y));
				me->move.setAction(ACTION_MAGIC_MISSILE);
				me->move.setCastAngle(me->getAngleTo(&obstacle));
				////////////////cout << tick << ") Shoot tree!" << endl;
			}
		}
	}
}
// общая функция похода куда-то
bool GetToPoint(Point target, int depth, bool WoodcutAllowed, double RadiusOfClearing = 0, double tree_reduce = 0)
{
	vector <Obstacle> obstacles = clear(getObstacles(self->getRadius()), me->pos(), target, RadiusOfClearing, tree_reduce);
	vector <Segment> segments = BuildPath(Segment(me->pos(), target), obstacles, depth, 0, WoodcutAllowed ? isSpaceLaneFunc[me->lane()] : isSpaceWrong);

	for (Segment segment : segments)
	{
		//debug.line(segment.A.x, segment.A.y, segment.B.x, segment.B.y, 0x0000FF);
	}

	if (segments.empty())
		return false;

	me->Go(segments[0].B);

	if (WoodcutAllowed)
		WoodcutterHelp(obstacles);

	return true;
}

// Заглушка: ищем ближайшее дерево и пилим его
void WoodcutterMode()
{
	vector<double> dist;
	vector <WarTree *> ans;
	for (auto T : trees)
	{
		WarTree * tree = T.second;
		ans.push_back(tree);

		int timeToStaff = me->timeToStaff(tree);
		int timeToMissile = me->timeToMissile(tree);
		dist.push_back(Min(timeToStaff, timeToMissile));

		int j = ans.size() - 1;
		while (j > 0 && dist[j] < dist[j - 1])
		{
			swap(dist[j], dist[j - 1]);
			swap(ans[j], ans[j - 1]);
			--j;
		}
	}

	// не можем найти дерево - переходим к следующему поблизости.
	for (WarTree * tree_to_cut : ans)
	{
		if (me->canStaff(tree_to_cut) && NeutralsNotUnderStaff())
		{
			me->move.setAction(ACTION_STAFF);
			////////////////cout << tick << ") Cut tree!" << endl;
		}
		else if (me->canAttack(tree_to_cut))
		{
			me->move.setMaxCastDistance(sqrt(me->getDist2To(tree_to_cut)));
			me->move.setMinCastDistance(sqrt(me->getDist2To(tree_to_cut)));
			me->move.setAction(ACTION_MAGIC_MISSILE);
			me->move.setCastAngle(me->getAngleTo(tree_to_cut));
			////////////////cout << tick << ") Shoot tree!" << endl;
		}

		if (GetToPoint(tree_to_cut->pos(), 10, true))
			return;
	}

	////////////cout << tick << ") ERROR: Woodcutter... is cut." << endl;
	me->Go(me->pos() - (ans[0]->pos() - me->pos()));
}
void WoodcutterMode(LaneType lane)
{
	vector<double> dist;
	vector <WarTree *> ans;
	for (auto T : trees)
	{
		WarTree * tree = T.second;
		if (getLane(tree->x, tree->y) == lane)
		{
			ans.push_back(tree);

			int timeToStaff = me->timeToStaff(tree);
			int timeToMissile = me->timeToMissile(tree);
			dist.push_back(Min(timeToStaff, timeToMissile));

			int j = ans.size() - 1;
			while (j > 0 && dist[j] < dist[j - 1])
			{
				swap(dist[j], dist[j - 1]);
				swap(ans[j], ans[j - 1]);
				--j;
			}
		}
	}

	// не можем найти дерево - переходим к следующему поблизости.
	for (WarTree * tree_to_cut : ans)
	{
		if (me->canStaff(tree_to_cut) && NeutralsNotUnderStaff())
		{
			me->move.setAction(ACTION_STAFF);
			////////////////cout << tick << ") Cut tree!" << endl;
		}
		else if (me->canAttack(tree_to_cut))
		{
			me->move.setMaxCastDistance(sqrt(me->getDist2To(tree_to_cut)));
			me->move.setMinCastDistance(sqrt(me->getDist2To(tree_to_cut)));
			me->move.setAction(ACTION_MAGIC_MISSILE);
			me->move.setCastAngle(me->getAngleTo(tree_to_cut));
			////////////////cout << tick << ") Shoot tree!" << endl;
		}

		if (GetToPoint(tree_to_cut->pos(), 10, true))
			return;
	}

	////////////cout << tick << ") ERROR: Woodcutter... is cut." << endl;
	me->Go(me->pos() - (ans[0]->pos() - me->pos()));
}
void GoIntoBattle(LaneType lane)
{	
	Point P = getWarSegment(lane, false).point();

	if (getArea(me->x, me->y) == MY_HOME)
	{
		if (lane == LANE_TOP)
			P = Point(200, 3000);
		else if (lane == LANE_MIDDLE)
			P = Point(1000, 3000);
		else
			P = Point(1000, 3800);
	}

	if (!GetToPoint(P, 15, true))	
	{
		//cout << tick << ") No variants of going into battle..." << endl;
		
		/*if (me->getSpeed().norm2() > 0)
		{
			me->goInDirection(me->getSpeed());
			return;
		}
		else
		{*/
			WoodcutterMode(lane);
			return;
		//}
	}		
}

// Оцениваем точки массива points по выгодности.
vector<double> estimation(const vector<Point> & points, WarPiece * target, AREA A, double Rocket_R, int time_till_shoot)
{
	int VARS = points.size();
	vector<double> est(VARS, 0);

	for (WarPiece * piece : pieces)
	{
		if (piece->isInBattle(A) && !piece->isFriend && piece != me->missile_target)
		{
			for (int i = 0; i < VARS; ++i)
			{
				// чем дальше от врага, тем лучше
				est[i] += piece->attack() * piece->getDist2To(points[i]) / (me->pos() - piece->pos()).norm2();
			}
		}
	}
	// штрафуем за попадание в радиус атаки башни!
	for (auto it : towers)
	{
		WarTower * t = it.second;
		for (int i = 0; i < VARS; ++i)
			if (!t->isFriend && t->getDist2To(points[i]) <= t->attackRange() * t->attackRange())			
				est[i] -= 1 - t->attackCooldownTicks / game->getGuardianTowerCooldownTicks();
	}

	// оцениваем, насколько мы "за деревьями"
	// TODO прятаться за деревьями!
	vector <Segment> S;
	for (int i = 0; i < VARS; ++i)
		S.push_back(Segment(target->tar_pos(me), points[i]));
	for (auto T : trees)
	{
		Circle tree(T.second->pos(), T.second->R + Rocket_R);
		for (int i = 0; i < VARS; ++i)
		{
			est[i] -= pow(game->getWizardActionCooldownTicks() - time_till_shoot, 2) * S[i].intersects(tree) * (tree.R - S[i].how_far(tree.pos()));
		}
	}

	// пытаемся избежать точек спавна четырёх крипов
	LaneType lanes[3] = { LANE_TOP, LANE_MIDDLE, LANE_BOTTOM };
	for (int i = 0; i < 3; ++i)
	{
		if (A == getArea(lanes[i]) || A == ENEMY_HOME)
		{
			for (int i = 0; i < VARS; ++i)
				est[i] -= 600.0 / sqrt((points[i] - spawns[i]).norm2()) / (750 - tick % 750);
		}
	}

	return est;
}

// ищем оптимальное положение в бою
double BASE_ANGLE = PI / 60;
double MAX_BASE_ANGLE = PI / 3;
double ITER_ANGLE = PI / 40;
void BattleDance(AREA A, Point tar_pos, WarPiece * target, double attackRange, double Rocket_R, int timeTillAttack)
{
	// круг, на котором мы должны находится
	double Super_R = attackRange + (game->getWizardBackwardSpeed() * me->getSpeedCoeff()) * timeTillAttack;
	//debug.circle(tar_pos.x, tar_pos.y, Super_R, 0x000000);

	// если мы в круге, нам надо искать на нём оптимальное положение
	if (me->getDist2To(tar_pos) <= Super_R * Super_R)
	{
		vector<Point> vars;

		for (double angle = -MAX_BASE_ANGLE; angle <= MAX_BASE_ANGLE; angle += ITER_ANGLE)
		{
			Point var = tar_pos + (me->pos() - tar_pos).make_module(Super_R).turn(angle);
			if (isCorrect(var))
			{
				vars.push_back(var);
			}
		}

		// получаем оценки
		vector<double> est = estimation(vars, target, A, Rocket_R, timeTillAttack);

		double best = -INFINITY;
		int ans = -1;
		for (int i = 0; i < est.size(); ++i)
		{
			if (est[i] > best)
			{
				best = est[i];
				ans = i;
			}
		}

		if (ans != -1 && ans < vars.size())
		{
			Point true_point = vars[ans];

			// пытаемся идти к точке
			if (!GetToPoint(true_point, 10, false))
			{
				////////////////cout << tick << ") Hard battledance!" << endl;
			}
		}
	}
	else
	{
		if (!GetToPoint(tar_pos, 15, false, attackRange, me->R - Rocket_R))
		{
			////////////cout << tick << ") VERY HARD to attack!" << endl;

			// TODO WHHHHAAAAAAT? commented ver.33
			/*if (me->getSpeed().norm2() > 0)
			{
				me->goInDirection(me->getSpeed());
			}
			else
			{*/
				WoodcutterMode();
			//}
		}
	}

	double R = Max(me->getRange(), Super_R);
	if (me->getDist2To(tar_pos) <= R * R)
	{
		me->move.setTurn(limit(me->getAngleTo(tar_pos), me->maxTurnAngle()));
	}
}

// избиваем палкой
void Beat()
{
	AREA A = getArea(me->x, me->y);

	for (WarPiece * piece : pieces)
	{
		if (piece->isInBattle(A) && 
			!piece->isFriend && me->isBetterStaffTarget(piece))
		{
			me->staff_target = piece;
		}
	}

	// если стратегия не подразумевает избиения, в цели будет записано NULL
	// тем не менее, рядом может быть враг. Его следует избить в перерыве между выстрелами. Но это в BeatIfCan
	if (me->staff_target != NULL)
	{
		// если мы не убегаем, подстраиваемся под атаку. Иначе только поворачиваемся к цели.
		BattleDance(A, me->staff_target->pos(), me->staff_target, game->getStaffRange() + me->staff_target->R, 0, me->till(ACTION_STAFF));
		//else
		//	me->move.setTurn(limit(me->getAngleTo(me->staff_target), me->maxTurnAngle()));

		if (me->canStaff(me->staff_target))
		{
			me->move.setAction(ACTION_STAFF);
			++UsesOfStaff;
		}
	}
}
void BeatIfCan()
{
	// ищем возможность между выстрелами побить соседа
	AREA A = getArea(me->x, me->y);

	if (me->till(ACTION_STAFF) + game->getWizardActionCooldownTicks() <= me->tillShoot(me->missile_target))
	{
		// время, за которое надо успеть
		int time = (me->tillShoot(me->missile_target) - game->getWizardActionCooldownTicks()) / 2;  // на 2 потому что ещё надо вернуться
		
		WarPiece * target = NULL;
		for (WarPiece * piece : pieces)
		{
			if (piece->isInBattle(A) && !piece->isFriend && me->timeToStaff(piece) <= time)
			{
				target = piece;
				time = me->timeToStaff(piece);
			}
		}

		if (target != NULL)
		{
			me->move.setTurn(limit(me->getAngleTo(target), me->maxTurnAngle()));
			if (me->canStaff(target))
			{
				me->move.setAction(ACTION_STAFF);
			}
		}
	}
}
void Shoot()
{
	AREA A = getArea(me->x, me->y);

	for (WarPiece * piece : pieces)
	{
		if (piece->isInBattle(A) && !piece->isFriend && me->isBetterMissileTarget(piece))
		{
			me->missile_target = piece;
		}
	}

	if (me->missile_target != NULL)
	{
		bool freeze = me->willFreeze(me->missile_target) && me->till(ACTION_FROST_BOLT) == 0;
		bool fireball = me->willFireball(me->missile_target) && me->till(ACTION_FIREBALL) == 0;
		double Rocket_R = freeze ? game->getFrostBoltRadius() : (fireball ? game->getFireballRadius() : game->getMagicMissileRadius());

		if (me->staff_target == NULL)
		{
			BattleDance(A, me->missile_target->tar_pos(me), me->missile_target, me->getDistToShoot(), Rocket_R, me->tillShoot(me->missile_target));
		}
		//else
		//	me->move.setTurn(limit(me->getAngleTo(me->missile_target), me->maxTurnAngle()));

		// проверяем, стоит ли на пути ДЕРЕВО!
		bool tree_on_my_way = false;
		Segment path(me->pos(), me->missile_target->tar_pos(me));
		for (auto T : trees)
		{
			WarTree * tree = T.second;
			if (path.intersects(Circle(tree->pos(), tree->R + Rocket_R)))
			{
				tree_on_my_way = true;
			}
		}

		if (!tree_on_my_way && me->canAttack(me->missile_target))
		{
			if (freeze)
			{
				me->move.setAction(ACTION_FROST_BOLT);
			}
			else if (fireball)
			{
				me->move.setAction(ACTION_FIREBALL);
			}
			else
			{
				me->move.setAction(ACTION_MAGIC_MISSILE);
			}

			me->move.setCastAngle(me->getAngleTo(me->missile_target));
			me->move.setMaxCastDistance(sqrt(me->getDist2To(me->missile_target->tar_pos(me))) + me->missile_target->saveRocketR());
			me->move.setMinCastDistance(sqrt(me->getDist2To(me->missile_target->tar_pos(me))) - me->missile_target->saveRocketR());
		}
		else
			BeatIfCan();
	}
}

// отступление!
int ESCAPE_DISTANCE = 1000;
double FEAR_DISTANCE = 1000;
double WALLS_FEAR_DISTANCE = 1000;
Point WhereToRun(AREA A)  // даёт направление
{
	double directions[4] = { 0, 0, 0, 0 };
	for (WarPiece * piece : pieces)
	{
		if (!piece->isFriend && piece->status != DEAD && piece->status != UNKNOWN)
		{
			Point dir = me->pos() - piece->pos();
			Point force = dir.make_module(Max(0.0, FEAR_DISTANCE - sqrt(dir.norm2())));//dir.make_module(1 / sqrt(dir.norm2()));

			if (force.x >= 0)
				directions[0] += force.x;
			else
				directions[1] -= force.x;

			if (force.y > 0)
				directions[2] += force.y;
			else
				directions[3] -= force.y;
		}
	}

	if (me->x < 400)
		directions[0] += WALLS_FEAR_DISTANCE - me->x;// 1 / me->x;
	if (me->x > 4000 - 400)
		directions[1] += WALLS_FEAR_DISTANCE - (4000 - me->x);
	if (me->y < 400)
		directions[2] += WALLS_FEAR_DISTANCE - me->y;
	if (me->y > 4000 - 400)
		directions[3] += WALLS_FEAR_DISTANCE - (4000 - me->y);

	// TODO Деревья? Препятствия?
	// Спавн крипов?

	Point retreat(directions[0] - directions[1], directions[2] - directions[3]);
	retreat.x += (retreat.x >= 0 ? 1 : -1) * Min(directions[2], directions[3]);
	retreat.y += (retreat.y > 0 ? 1 : -1) * Min(directions[0], directions[1]);

	return retreat;
}
void Run(AREA A)
{
	// если меня не видно, прячемся
	if (!isPointVisibleByEnemy(me->pos()))
	{
		me->move.setSpeed(0);
		me->move.setStrafeSpeed(0);
		return;
	}

	// получаем точку, куда надо бежать
	Point retreat = WhereToRun(A);
	retreat.make_module(ESCAPE_DISTANCE);
	retreat = retreat + me->pos();

	drawArrow(me->x, me->y, retreat.x, retreat.y, 0xFF0000);

	if (!GetToPoint(retreat, 15, false))
	{
		////////////////cout << tick << ") VERY HARD to retreat!" << endl;

		//TODO WHAAAAT?
		WoodcutterMode();
	}
}
void MoveFurther()
{
	// продвигаемся по линии
	WhereAtLine next = getWarSegment(me->lane(), false);
	Point Where = next.far >= 1 ? Point(3950, 50) : next.point();
	
	if (!GetToPoint(Where, 15, false))
	{
		////////////////cout << tick << ") VERY HARD to go further on line!" << endl;

		WoodcutterMode();
	}
}
