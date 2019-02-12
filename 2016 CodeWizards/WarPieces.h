#include "Pathfinder.h"

// ��� � ���� ������� ��������� ������������ ����� �� ����
const int BATTLE_NEAR_ME = 1000;
const int BATTLE_NEAR_ANYONE = 650;
const int MISSED_TIME = 2000;  // �� ��� ����� �� �������� �������� ������ � shadows-���������������

class WarPiece;
class Battle;
vector <WarPiece *> pieces;

enum STATUS { SIT_HOME,  // �� ����� ���� � �� ����������� ��� ����� ������
			  GO_INTO_BATTLE, // ��� � ���� ��� �� ���������� ���������. �� ����, ��� ��� �� ���������
	          BATTLE,         // ���������
	          BATTLE_FROM_SHADOW,  // ���������, �� ��� �� �����
	          DEAD,            // ����, �� ����������
			  UNKNOWN};         // ���������, ��-�� ����� �� ���� �����

const int NEVER = -179;
bool isPointVisible(Point point);
class WarPiece : public Obstacle
{
public:
	int id;

	double angle = 0;
	double life;

	// �����, � ������� ���� ������� ��� ��������. ��� ����������� ��� ������ ������� �������� ���, ��������, �����
	virtual Point tar_pos(WarPiece * shooter) { return pos(); }
	virtual double saveRocketR() { return 0; }  // ����������, �� ������� ���� ������� ������, ����� �� ����� �����
	
	bool isFriend;
	Faction faction;

	bool isDistanced = true;  // ������������ ����� - ����������, ������, ������
	bool isBuilding = false;  // ����� � ����
	
	// ����������� �������:
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

	// ������ ������! ��� ��������� � ��� �������� � ������ (������ ��� ���, ��������)
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
	// ��� ��� ������� �����, ������� �� �� ��� ����� ���������, �� �� ����� �������� �� ���
	virtual bool isImmortal() { return false; }
	virtual LaneType lane() { return getLane(x, y); };

	// ������� ����. �����, ��� ������������ ������ ������������.
	WarPiece * missile_target = NULL;
	WarPiece * staff_target = NULL;  // ���� NULL, ��� ������������, � �������� �������� ����� ������ �� ���, �� ���� �����.

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

	// ������������ ����, ����� �� �� ������ ��� ���.
	int last_update = NEVER;
	bool isVisible() { return last_update == tick; }
	virtual bool checkIfWasNotUpdated() = 0;
	
	// ���� ������� ������ ������� �� ����� P � ����� �������� ������!
	virtual void madeShot(Point P)
	{
		x = P.x, y = P.y;
		status = BATTLE_FROM_SHADOW;
	}

	// �����, ����� ������� ������������� ���� ���������� ����
	int estimated_death_time = NEVER;
	int timeToWarSegment();  // ����� �� �������� �� ���� ���. ���������� � WarLine.h
	
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
			if (isVisible())  // ���� ����, ���� �� ��� �����. � ��������� ������ �� ����� ����� �� �����
			{
				// ����� �� ����� - ��������� � �����
				if (!isFriend || isEnemyNear())
				{
					status = BATTLE;
					////////////cout << tick << ") " << "Piece " << id << " is in battle!" << endl;
				}
			}
			else
			{
				// ���������� ����� �� �����
				WhereAtLine Now(x, y);
				WhereAtLine where_must_be(lane(), Now.far - (WAS_DEAD + 1) * getAverageSpeed() / Now.L);
				
				// ���� �� ����� �� ������� ����, �� �� ���-��� ��������, ������ ������� � ����� (��������!)
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

	virtual double attack() = 0;  // ������� � ������� ���� ����� � ���
	virtual double defence() { return life; }
	virtual double xp() { return 0; }  // xp-� �� ������
	virtual double points(double dam) { return 0; }
	virtual double xp_for_damage(double dam) { return 0; }  // xp �� ��������� �����
	virtual set <Point> possiblePositions() = 0;
	virtual double visionRange() = 0;
	virtual double maxTurnAngle() { return 0; }
	virtual double getAverageSpeed() = 0;
	
	// ������� ���� dam - ���������� �������� ��� ���.
	virtual int damage(double dam, bool magical)
	{
		if (isImmortal())
			return 0;
		else if (shielded > 0)
			return ceil(dam * (1 - game->getShieldedDirectDamageAbsorptionFactor()));
		else
			return dam;
	}

	// ����� �� �����
	inline bool isWatching(Point P)
	{
		return (P - pos()).norm2() < visionRange() * visionRange() + (isFriend ? -E : 10);
	}
	set<Point> couldEscape()  // ���� � �������� ���� ��� �� ������������� ����
	{
		set<Point> to_fill = possiblePositions();
		for (auto P: pieces)
		{
			if (P->isFriend && P->isValid())
			{
				// ���������� ������ ��� �����, ������� ����� ������ ���������
				set<Point> to_del;
				for (auto fill: to_fill)
				{
					if (P->isWatching(fill))
					{
						to_del.insert(fill);
					}
				}

				// � ������� ��
				for (auto del: to_del)
					to_fill.erase(del);

				// ���� ��� �������, ����� �� ����������
				if (to_fill.empty())
					return to_fill;
			}
		}

		// ��� ���� �� ��� ������� �� ������ �����
		return to_fill;
	}
	
	// ���������� � ���� �� ��������
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
		// � ��� ������� ��������� �� �� �������
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

// ����� �� ����� ��; ����������
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

// ���������� ����� ��� ����� ������, ����� ��� ������ � ������������, ������� ��� �� �����.
int MAX_DEPTH_PUSH_FROM_VISIBLE = 20;  // ����� ������ ���������� �������� �� ������.
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
				// �������� �� ���� ���������� � ��������� ��������.
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