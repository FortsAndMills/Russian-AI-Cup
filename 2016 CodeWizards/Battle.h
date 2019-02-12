#include "ExecuteAction.h"

// путь дл€ моделировани€ отступлени€
vector<Segment> retreat_path;
int where_on_retreat_path = 0;

class Battle
{
public:
	vector<WarWizard> _wizards;
	vector<WarMinion> _minions;
	vector<WarTower> _towers;


	vector <WarPiece *> war;
	vector <Missile> missiles;

	map<int, int> when_die;

	int excep = -1;  // кого не учитывать при столкновени€х - костылЄк дл€ отступлени€
	double xp = 0;  // сколько набрали экспы
	double enemy_xp = 0;
	double points = 0;  // это не очки, а бонусы за базу.
	WarWizard * phantom;

	int time;

	Battle(AREA A)
	{
		time = tick;
		where_on_retreat_path = 0;

		for (auto it = wizards.begin(); it != wizards.end(); ++it)
		{
			if (it->second->isInBattle(A))
			{
				_wizards.push_back(WarWizard(*(it->second)));
				if (it->second->strategy == WarWizard::RUN ||
					it->second->strategy == WarWizard::NEAREST_STAFF ||
					it->second->strategy == WarWizard::ILL_STAFF)
					excep = it->second->id;
			}
		}
		for (auto it = minions.begin(); it != minions.end(); ++it)
		{
			if (it->second->isInBattle(A))
			{
				_minions.push_back(WarMinion(*(it->second)));
			}
		}
		for (auto it = towers.begin(); it != towers.end(); ++it)
		{
			if (it->second->isInBattle(A))
			{
				_towers.push_back(WarTower(*(it->second)));
			}
		}

		for (auto it = _wizards.begin(); it != _wizards.end(); ++it)
		{
			if ((*it).missile_target != NULL)
				(*it).missile_target = getById((*it).missile_target->id);
			if ((*it).staff_target != NULL)
				(*it).staff_target = getById((*it).staff_target->id);
		}

		for (int i = 0; i < _wizards.size(); ++i)
		{
			war.push_back(&(_wizards[i]));
			if (_wizards[i].id == me->id)
				phantom = &(_wizards[i]);
		}
		for (int i = 0; i < _minions.size(); ++i)
		{
			war.push_back(&(_minions[i]));
		}
		for (int i = 0; i < _towers.size(); ++i)
		{
			war.push_back(&(_towers[i]));
		}
	
		for (auto it : Rockets)
		{
			if (it.second.target != NULL)
			{
				WarPiece * target = getById(it.second.target->id);
				WarPiece * author = it.second.author == NULL ? NULL : getById(it.second.author->id);
				if (target != NULL)
					missiles.push_back(Missile(author, target, it.second));
			}
		}
	}
	WarPiece * getById(int id)
	{
		for (WarPiece * piece : war)
			if (piece->id == id)
				return piece;
		return NULL;
	}

	int winner()
	{
		bool win = false;
		bool lose = false;
		for (WarPiece * piece : war)
		{
			if (piece->isFriend)
				win = true;
			else
				lose = true;
		}
		return win - lose;
	}
	bool isDead()
	{
		for (int i = 0; i < _wizards.size(); ++i)
		{
			if (_wizards[i].id == me->id)
			{
				return _wizards[i].life < 1;
			}
		}
		return false;
	}
	double myHealth()
	{
		for (int i = 0; i < _wizards.size(); ++i)
		{
			if (_wizards[i].id == me->id)
			{
				return Max(0.0, _wizards[i].life);
			}
		}
		return 0;
	}

	bool finished = false;
	int exceeded = NEVER;
	// провер€ет, не противоречит ли круг текущей диспозиции.
	bool isCollision(Circle C, int my_i)
	{
		for (int i = 0; i < (int)war.size(); ++i)
		{
			if (i != excep && my_i != excep && i != my_i && 
				((C.pos() - war[i]->pos()).norm2() < (war[i]->R + C.R) * (war[i]->R + C.R) - E))
				return true;
		}
		return false;
	}
	void NextTick(int ticks = 1)
	{
		time += ticks;
		if (time > world->getTickCount() || finished)
			return;

		// все юниты делают то, что хот€т
		int WarSize = war.size();
		for (int i = 0; i < WarSize; ++i)
		{
			war[i]->NextTick(this, ticks);
		}

		// перемещаем их
		for (int i = 0; i < WarSize; ++i)
		{
			Circle new_pos = war[i]->imitateMove();

			if (!isCollision(new_pos, i) && !isSpaceWrong(new_pos.pos()))
				war[i]->x = new_pos.x, war[i]->y = new_pos.y;

			war[i]->makeTurn();
		}

		// ракеты лет€т, некоторые взрываютс€
		for (int i = 0; i < (int)missiles.size(); ++i)
		{
			int dam = missiles[i].fly(ticks);
			if (dam != 0)
			{
				if (missiles[i].author != NULL && missiles[i].author->id == me->id)
				{
					xp += missiles[i].target->xp_for_damage(dam);
				}
				if (missiles[i].author != NULL && missiles[i].author->type == WIZARD && !missiles[i].author->isFriend)
				{
					enemy_xp += missiles[i].target->xp_for_damage(dam);
				}
				if (missiles[i].author != NULL)
				{
					points += (missiles[i].author->isFriend ? 1 : -1) * missiles[i].target->points(dam);
				}

				missiles.erase(missiles.begin() + i, missiles.begin() + i + 1);
				--i;
			}
		}

		// погибшие удал€ютс€.
		for (int i = 0; i < (int)war.size(); ++i)
		{
			if (war[i]->life < 1 - E)
			{
				for (int j = 0; j < (int)war.size(); ++j)  // кто стрел€л в этого - мен€ет цель.
				{
					if (war[j]->missile_target == war[i])
						war[j]->missile_target = NULL;
					if (war[j]->staff_target == war[i])
						war[j]->staff_target = NULL;
				}
				for (int j = 0; j < missiles.size(); ++j)  // ракеты, лет€щие в него, промахиваютс€.
				{
					if (missiles[j].target == war[i])
					{
						missiles.erase(missiles.begin() + j, missiles.begin() + j + 1);
						--j;
					}
				}

				// плюшки мне:
				if (war[i]->id == me->id)
				{
					enemy_xp += war[i]->xp();
					phantom = NULL;
				}
				else if (phantom != NULL && !war[i]->isFriend && war[i]->getDist2To(phantom) <= game->getScoreGainRange() * game->getScoreGainRange())
					xp += war[i]->xp();
				else if (war[i]->isFriend)
					enemy_xp += war[i]->xp();

				if (war[i]->type == BASE)
					finished = true;

				//////cout << "        tick " << time << ", " << war[i]->id << " dies." << endl;
				when_die[war[i]->id] = time;
				war.erase(war.begin() + i, war.begin() + i + 1);
				--i;
			}
		}
	}
};

void WarWizard::NextTick(Battle * battle, int ticks)
{
	life = Min((double)maxLife, life + regeneration() * ticks);
	mana = Min((double)maxMana, mana + manaRegeneration() * ticks);
	getStatusesUpdated(ticks);

	for (int i = 0; i < cooldownTicks.size(); ++i)
		cooldownTicks[i] = Max(cooldownTicks[i] - ticks, 0);
	actionCooldownTicks = Max(actionCooldownTicks - ticks, 0);

	if (frozen > 0)
	{
		move.setTurn(0);
		move.setSpeed(0);
		move.setStrafeSpeed(0);
		return;
	}

	// как только в мен€ можно встрел€ть, ќЌ» будут стрел€ть!
	if (!isFriend && strategy != ILL_WIZARD)
	{
		for (WarPiece * piece : battle->war)
		{
			if (piece->isFriend && piece->type == WIZARD && getDist2To(piece) <= getRange() * getRange())
				strategy = NEAREST_WIZARD;
		}
	}

	if (strategy == RUN)
	{
		// бегаем по ломанной. Ёто приблизительно, но похоже, на то что будет.
		//  оллизии при этом не учитываютс€
		if (where_on_retreat_path < retreat_path.size())
		{
			Go(retreat_path[where_on_retreat_path].B, ticks);

			if (getDist2To(retreat_path[where_on_retreat_path].B) < getAverageSpeed() * getAverageSpeed())
				++where_on_retreat_path;
		}
	}
	else if (findTargetToBeat(battle->war) == NULL)
	{
		findTargetToShoot(battle->war);		

		if (missile_target == NULL)
		{
			//////cout << "ERROR the battle is over?" << endl;
			if (battle->exceeded != NEVER)
				battle->exceeded = battle->time;

			move.setTurn(0);
			move.setSpeed(0);
			move.setStrafeSpeed(0);
			return;
		}

		if (canStaff(missile_target))
		{
			cooldownTicks[ACTION_STAFF] = game->getStaffCooldownTicks();
			actionCooldownTicks = game->getWizardActionCooldownTicks();
			
			double dam = missile_target->damage(staffDamage(), false);
			missile_target->life -= dam;
			if (id == me->id)
				battle->xp += missile_target->xp_for_damage(dam);
			else if (!isFriend)
				battle->enemy_xp += missile_target->xp_for_damage(dam);
			battle->points += (isFriend ? 1 : -1) * missile_target->points(dam);

			//////cout << "        tick " << battle->time << ", " << id << " staffs " << missile_target->id << endl;
		}
		else if (canAttack(missile_target))
		{
			if (willFreeze(missile_target) && till(ACTION_FROST_BOLT) == 0)
			{
				cooldownTicks[ACTION_FROST_BOLT] = game->getFrostBoltCooldownTicks();
				actionCooldownTicks = game->getWizardActionCooldownTicks();
				mana -= game->getFrostBoltManacost();
				battle->missiles.push_back(Missile(this, missile_target, PROJECTILE_FROST_BOLT, pos()));
			}
			else if (willFireball(missile_target) && till(ACTION_FIREBALL) == 0)
			{
				cooldownTicks[ACTION_FIREBALL] = game->getFireballCooldownTicks();
				actionCooldownTicks = game->getWizardActionCooldownTicks();
				mana -= game->getFireballManacost();
				battle->missiles.push_back(Missile(this, missile_target, PROJECTILE_FIREBALL, pos()));
			}
			else
			{
				cooldownTicks[ACTION_MAGIC_MISSILE] = magicMissileCooldown();
				actionCooldownTicks = game->getWizardActionCooldownTicks();
				mana -= game->getMagicMissileManacost();
				battle->missiles.push_back(Missile(this, missile_target, PROJECTILE_MAGIC_MISSILE, pos()));
			}
			//////cout << "        tick " << battle->time << ", " << id << " shoots at " << missile_target->id << endl;
		}

		if (getDist2To(missile_target->tar_pos(me)) <= getDistToShoot() * getDistToShoot())
		{			
			double Super_R = getDistToShoot() + (game->getWizardBackwardSpeed() * getSpeedCoeff()) * tillShoot(missile_target);
			Point true_point = missile_target->tar_pos(me) + (pos() - missile_target->tar_pos(me)).make_module(Super_R);
			Go(true_point, ticks);
			
			move.setTurn(limit(getAngleTo(missile_target->tar_pos(me)), maxTurnAngle() * ticks));
		}
		else
			Go(missile_target->tar_pos(me), ticks);
	}
	else
	{
		missile_target = staff_target;

		move.setTurn(0);
		move.setSpeed(0);
		move.setStrafeSpeed(0);

		if (canStaff(staff_target))
		{
			cooldownTicks[ACTION_STAFF] = game->getStaffCooldownTicks();
			actionCooldownTicks = game->getWizardActionCooldownTicks();
			double dam = staff_target->damage(staffDamage(), false);
			staff_target->life -= dam;
			if (id == me->id)
				battle->xp += staff_target->xp_for_damage(dam);
			else if (!isFriend)
				battle->enemy_xp += staff_target->xp_for_damage(dam);
			battle->points += (isFriend ? 1 : -1) * staff_target->points(dam);
			
			//////cout << "        tick " << battle->time << ", " << id << " staffs " << missile_target->id << endl;
		}
		if (canAttack(missile_target))
		{
			if (willFreeze(missile_target) && till(ACTION_FROST_BOLT) == 0)
			{
				cooldownTicks[ACTION_FROST_BOLT] = game->getFrostBoltCooldownTicks();
				actionCooldownTicks = game->getWizardActionCooldownTicks();
				mana -= game->getFrostBoltManacost();
				battle->missiles.push_back(Missile(this, missile_target, PROJECTILE_FROST_BOLT, pos()));
			}
			else if (willFireball(missile_target) && till(ACTION_FIREBALL) == 0)
			{
				cooldownTicks[ACTION_FIREBALL] = game->getFireballCooldownTicks();
				actionCooldownTicks = game->getWizardActionCooldownTicks();
				mana -= game->getFireballManacost();
				battle->missiles.push_back(Missile(this, missile_target, PROJECTILE_FIREBALL, pos()));
			}
			else
			{
				cooldownTicks[ACTION_MAGIC_MISSILE] = magicMissileCooldown();
				actionCooldownTicks = game->getWizardActionCooldownTicks();
				mana -= game->getMagicMissileManacost();
				battle->missiles.push_back(Missile(this, missile_target, PROJECTILE_MAGIC_MISSILE, pos()));
			}

			//////cout << "        tick " << battle->time << ", " << id << " shoots at " << missile_target->id << endl;
		}

		if (getDist2To(missile_target) <= (game->getStaffRange() + staff_target->R) * (game->getStaffRange() + staff_target->R))
		{
			double Super_R = game->getStaffRange() + (game->getWizardBackwardSpeed() * getSpeedCoeff()) * till(ACTION_STAFF);
			Point true_point = staff_target->pos() + (pos() - staff_target->pos()).make_module(Super_R);
			Go(true_point, ticks);

			move.setTurn(limit(getAngleTo(staff_target), maxTurnAngle() * ticks));
		}
		else
			Go(staff_target->pos(), ticks);
	}
}

void WarMinion::NextTick(Battle * battle, int ticks)
{
	getStatusesUpdated();
	attackCooldownTicks = Max(0, attackCooldownTicks - ticks);

	if (frozen > 0)
	{
		move.setTurn(0);
		move.setSpeed(0);
		move.setStrafeSpeed(0);
		return;
	}

	WarPiece * target = NULL;
	double dist2 = 100000000.0;

	// ищем ближайшую цель
	for (WarPiece * piece : battle->war)
	{
		if (piece->isFriend != isFriend && dist2 > getDist2To(piece) && getDist2To(piece) <= visionRange() * visionRange())
		{
			dist2 = getDist2To(piece);
			target = piece;
		}
	}

	// не находим - просто движемс€ по линии
	if (target == NULL)
	{
		Point target = (isFriend ? 1 : -1) * WhereAtLine(x, y).getVector();
		goInDirection(target);
	}
	else if (m_type == MINION_ORC_WOODCUTTER)
	{
		if (canAttack(target))
		{
			attackCooldownTicks = game->getOrcWoodcutterActionCooldownTicks();
			target->life -= target->damage(game->getOrcWoodcutterDamage(), false);

			battle->points += (isFriend ? 1 : -1) * target->points(game->getOrcWoodcutterDamage());

			//////cout << "        tick " << battle->time << ", " << id << " orcs at " << target->id << endl;
		}

		if (getDist2To(target) <= (game->getOrcWoodcutterAttackRange() + target->R) * (game->getOrcWoodcutterAttackRange() + target->R))
			move.setTurn(limit(getAngleTo(target), maxTurnAngle() * ticks));
		else
			Go(target->pos(), ticks);
	}
	else
	{
		if (canAttack(target))
		{
			attackCooldownTicks = game->getFetishBlowdartActionCooldownTicks();
			battle->missiles.push_back(Missile(this, target, PROJECTILE_DART, pos()));

			//////cout << "        tick " << battle->time << ", " << id << " fetishes at " << target->id << endl;
		}
		
		if (getDist2To(target) <= game->getFetishBlowdartAttackRange() * game->getFetishBlowdartAttackRange())
			move.setTurn(limit(getAngleTo(target), maxTurnAngle() * ticks));
		else
			Go(target->pos(), ticks);
	}
}

void WarTower::NextTick(Battle * battle, int ticks)
{
	getStatusesUpdated();
	attackCooldownTicks = Max(0, attackCooldownTicks - ticks);
	if (attackCooldownTicks > 0)
		return;

	WarPiece * target = NULL;
	double health = 0;

	for (int i = battle->war.size() - 1; i >= 0; --i)
	{
		WarPiece * piece = battle->war[i];

		// пока выбор цели башней оптимистичен дл€ врага и пессимистичен дл€ нас
		// TODO хмм, а не предполагаем ли мы, что башн€ поначалу будет помогать нам в избиении вражеских волшебников?
		if (piece->isFriend != isFriend && getDist2To(piece) <= attackRange() * attackRange() &&
			((health < getDamage() && health <= piece->life) || 
			(health >= getDamage() && piece->life >= getDamage() &&
				((isFriend && piece->life > health) || (!isFriend && (piece->life < health || target->id == me->id) && target->id != me->id)))))
		{
			health = piece->life;
			target = piece;
		}
	}

	if (target != NULL)
	{
		attackCooldownTicks = type == TOWER ? game->getGuardianTowerCooldownTicks() : game->getFactionBaseCooldownTicks();
		target->life -= target->damage(getDamage(), false);
		//////cout << "        tick " << battle->time << ", " << id << " towers " << target->id << endl;
	}
}
