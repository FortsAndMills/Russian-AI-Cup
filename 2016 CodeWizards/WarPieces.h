#include "Pathfinder.h"

// все в этом радиусе считаются сражающимися рядом со мной
const int BATTLE_NEAR_ME = 1000;
const int BATTLE_NEAR_ANYONE = 650;
const int MISSED_TIME = 2000;  // за это время мы перестаём доверять данным о shadows-местонахождении

class WarPiece;
class Battle;
vector <WarPiece *> pieces;

enum STATUS { SIT_HOME,  // он сидит дома и не определился что будет делать
			  GO_INTO_BATTLE, // идёт к полю боя по безопасным маршрутам. По идеи, тут его не атаковать
	          BATTLE,         // сражается
	          BATTLE_FROM_SHADOW,  // сражается, но мне не видно
	          DEAD,            // мёртв, но возродится
			  UNKNOWN};         // непонятно, чё-то давно не было видно

const int NEVER = -179;
bool isPointVisible(Point point);
class WarPiece : public Obstacle
{
public:
	int id;

	double angle = 0;
	double life;

	// точка, в которую надо целится при выстреле. Она различается для разных позиций обстрела для, например, башен
	virtual Point tar_pos(WarPiece * shooter) { return pos(); }
	virtual double saveRocketR() { return 0; }  // расстояние, на которое надо пускать снаряд, чтобы он точно попал
	
	bool isFriend;
	Faction faction;

	bool isDistanced = true;  // дальнобойные юниты - волшебники, фетиши, здания
	bool isBuilding = false;  // башни и базы
	
	// Отслеживаем статусы:
	vector<Status> statuses;
	vector<int> burning;
	int empowered = 0;
	int hastened = 0;
	int shielded = 0;
	int frozen = 0;
	void processStatuses()
	{
		burning.clear();
		for (int i = 0; i < statuses.size(); ++i)
		{
			if (statuses[i].getType() == STATUS_BURNING)
				burning.push_back(statuses[i].getRemainingDurationTicks());
			if (statuses[i].getType() == STATUS_EMPOWERED)
				empowered = statuses[i].getRemainingDurationTicks();
			if (statuses[i].getType() == STATUS_FROZEN)
				frozen = statuses[i].getRemainingDurationTicks();
			if (statuses[i].getType() == STATUS_HASTENED)
				hastened = statuses[i].getRemainingDurationTicks();
			if (statuses[i].getType() == STATUS_SHIELDED)
				shielded = statuses[i].getRemainingDurationTicks();
		}
	}
	bool getStatusesUpdated(int ticks = 1)
	{
		for (int i = 0; i < burning.size(); ++i)
		{
			if (burning[i] > 0)
			{
				life -= Min(burning[i], ticks) * (double)game->getBurningSummaryDamage() / game->getBurningDurationTicks();
				burning[i] -= ticks;
			}
			else
			{
				burning.erase(burning.begin() + i, burning.begin() + i + 1);
				--i;
			}
		}
		if (hastened > 0)
			hastened = Max(0, hastened - ticks);
		if (empowered > 0)
			empowered = Max(0, empowered - ticks);
		if (shielded > 0)
			shielded = Max(0, shielded - ticks);
		if (frozen > 0)
			frozen = Max(0, frozen - ticks);

		return life < 1;
	}

	// Боевой статус! Где находится и что хранится в классе (мёртвый или нет, например)
	STATUS status;
	bool isNearMe = false;
	bool isValid()
	{
		return status != UNKNOWN && status != DEAD && (isVisible() || type == TOWER || type == BASE);
	}
	virtual bool isOnLane(LaneType L)
	{
		return getArea(x, y) == getArea(L) ||
			  (((getArea(x, y) == MY_HOME    && countTowers(L, true) == 0) ||
				(getArea(x, y) == ENEMY_HOME && countTowers(L, false) == 0)));
	}
	virtual bool isInArea(AREA A)
	{
		return getArea(x, y) == A || (A != MY_HOME && A != ENEMY_HOME && isOnLane(getLane(A)));
	}
	virtual bool isInBattle(AREA A, bool may_be_soon = false)
	{
		return (status == BATTLE || status == BATTLE_FROM_SHADOW || (status == GO_INTO_BATTLE && may_be_soon)) && 
			(isInArea(A) || isNearMe);
	}
	// это для башенок нужно, которые не на той линии находятся, но всё равно стреляют по нам
	virtual bool isImmortal() { return false; }
	virtual LaneType lane() { return getLane(x, y); };

	// текущие цели. Вабще, это используется только волшебниками.
	WarPiece * missile_target = NULL;
	WarPiece * staff_target = NULL;  // если NULL, это приоритетнее, и ракетами стрелять будем только по тем, по кому можем.

	WarPiece(OBSTACLE_TYPE type, int id, double x, double y, double R, double life, bool isFriend, STATUS status) : 
		Obstacle(Point(x, y), R, type)
	{
		this->id = id;
		this->isFriend = isFriend;
		this->status = status;
		this->life = life;
		this->faction = isFriend ? self->getFaction() : (self->getFaction() == FACTION_ACADEMY ? FACTION_RENEGADES : FACTION_ACADEMY);
	}
	virtual ~WarPiece() {}

	// отслеживание того, видим ли мы объект или нет.
	int last_update = NEVER;
	bool isVisible() { return last_update == tick; }
	virtual bool checkIfWasNotUpdated() = 0;
	
	// этот товарищ сделал выстрел из точки P и нужно обновить данные!
	virtual void madeShot(Point P)
	{
		x = P.x, y = P.y;
		status = BATTLE_FROM_SHADOW;
	}

	// время, через которое предполагаемо этот персонажик умрёт
	int estimated_death_time = NEVER;
	int timeToWarSegment();  // время до прибытия на поле боя. Определено в WarLine.h
	
	bool isEnemyNear()
	{
		for (WarPiece * piece : pieces)
		{
			if (piece->isFriend != isFriend && piece->isValid() && getDist2To(piece) <= game->getWizardVisionRange() * game->getWizardVisionRange())
				return true;
		}
		return false;
	}
	virtual void updateGoingIntoBattle()
	{
		if (status == GO_INTO_BATTLE)
		{
			if (isVisible())  // либо свой, либо мы его видим. В последнем случае он точно дошёл до битвы
			{
				// дошёл до битвы - переводим в битву
				if (!isFriend || isEnemyNear())
				{
					status = BATTLE;
					////////////cout << tick << ") " << "Piece " << id << " is in battle!" << endl;
				}
			}
			else
			{
				// продвигаем юнита по линии
				WhereAtLine Now(x, y);
				WhereAtLine where_must_be(lane(), Now.far - (WAS_DEAD + 1) * getAverageSpeed() / Now.L);
				
				// если он дошёл до видимой зоны, то он вот-вот появится, просто скрылся в тенях (наверное!)
				if (isPointVisible(where_must_be.point()))
				{
					status = BATTLE_FROM_SHADOW;
					//////////////cout << "Piece " << id << " must be in the battle, but he didn't appear! Shadows!" << endl;
				}
				else
				{
					x = where_must_be.x;
					y = where_must_be.y;
				}
			}
		}
	}
	virtual void updateShadows() = 0;

	virtual double attack() = 0;  // сколько в среднем сила атаки в тик
	virtual double defence() { return life; }
	virtual double xp() { return 0; }  // xp-а за гибель
	virtual double points(double dam) { return 0; }
	virtual double xp_for_damage(double dam) { return 0; }  // xp за нанесение урона
	virtual set <Point> possiblePositions() = 0;
	virtual double visionRange() = 0;
	virtual double maxTurnAngle() { return 0; }
	virtual double getAverageSpeed() = 0;
	
	// нанести урон dam - магическим способом или нет.
	virtual int damage(double dam, bool magical)
	{
		if (isImmortal())
			return 0;
		else if (shielded > 0)
			return ceil(dam * (1 - game->getShieldedDirectDamageAbsorptionFactor()));
		else
			return dam;
	}

	// видит ли точку
	inline bool isWatching(Point P)
	{
		return (P - pos()).norm2() < visionRange() * visionRange() + (isFriend ? -E : 10);
	}
	set<Point> couldEscape()  // куда с прошлого тика мог бы переметнуться юнит
	{
		set<Point> to_fill = possiblePositions();
		for (auto P: pieces)
		{
			if (P->isFriend && P->isValid())
			{
				// составляем список тех точек, которые видны данным союзником
				set<Point> to_del;
				for (auto fill: to_fill)
				{
					if (P->isWatching(fill))
					{
						to_del.insert(fill);
					}
				}

				// и удаляем их
				for (auto del: to_del)
					to_fill.erase(del);

				// если все удалены, можно не продолжать
				if (to_fill.empty())
					return to_fill;
			}
		}

		// вот сюда он мог сбежать из нашего взора
		return to_fill;
	}
	
	// расстояния и углы до объектов
	inline double getDist2To(Point P)
	{
		return (P - pos()).norm2();
	}
	inline double getDist2To(Circle * p)
	{
		return getDist2To(p->pos());
	}
	double getAngleTo(Point P)
	{
		// а тут немного копипасты из ИХ модулей
		Point vec = P - pos();

		double absoluteAngleTo = vec.angle();
		double relativeAngleTo = normalize(absoluteAngleTo - this->angle);

		return relativeAngleTo;
	}
	inline double getAngleTo(const Circle * p)
	{
		return getAngleTo(p->pos());
	}

	Move move;
	void clearMove()
	{
		move.setAction(ACTION_NONE);
		move.setCastAngle(0);
		move.setMaxCastDistance(0);
		move.setMessages(vector<Message>());
		move.setMinCastDistance(0);
		move.setSkillToLearn(SKILL_FIREBALL);
		move.setSpeed(0);
		move.setStatusTargetId(-1);
		move.setStrafeSpeed(0);
		move.setTurn(0);
	}
	Circle imitateMove()
	{
		Point new_pos = pos() + move.getSpeed() * Point(angle);
		new_pos = new_pos + move.getStrafeSpeed() * Point(angle + PI / 2);
		return Circle(new_pos, R);
	}
	void makeTurn()
	{
		angle = normalize(angle + move.getTurn());
	}
	
	virtual void NextTick(Battle * battle, int ticks) = 0;
};

// видим ли точку мы; противники
bool isPointVisible(Point point)
{
	for (auto P : pieces)
		if (P->isFriend && P->isValid())
			if (P->isWatching(point))
				return true;

	return false;
}
bool isPointVisibleByEnemy(Point point)
{
	for (auto P : pieces)
		if (!P->isFriend && P->isValid())
			if (P->isWatching(point))
				return true;

	return false;
}

// отодвигаем точку как можно меньше, чтобы она попала в пространство, которое нам не видно.
int MAX_DEPTH_PUSH_FROM_VISIBLE = 20;  // после такого количества итераций мы сдаёмся.
Point PushFromVisible(Point pos, int depth=0)
{
	if (depth >= MAX_DEPTH_PUSH_FROM_VISIBLE)
		return POINT_NOT_FOUND;

	for (auto P : pieces)
	{
		if (P->isFriend && P->isValid())
		{
			if (P->isWatching(pos))
			{
				// сдвигаем до края окружности и повторяем проверку.
				return PushFromVisible(P->pos() + (pos - P->pos()).make_module(P->visionRange()), depth + 1);
			}
		}
	}

	if (pos.x < 0)
		return PushFromVisible(Point(0, pos.y), depth + 1);
	if (pos.x > 4000)
		return PushFromVisible(Point(4000, pos.y), depth + 1);
	if (pos.y < 0)
		return PushFromVisible(Point(pos.x, 0), depth + 1);
	if (pos.y > 4000)
		return PushFromVisible(Point(pos.x, 4000), depth + 1);

	return pos;
}