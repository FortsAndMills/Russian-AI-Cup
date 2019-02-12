#include "WarLine.h"

// удалить из базы юнит с таким id
void killPiece(int id)
{
	int NumOfPieces = pieces.size();
	for (int i = 0; i < NumOfPieces; ++i)
	{
		if (pieces[i]->id == id)
		{
			// волшебники не удаляются
			if (pieces[i]->type == MINION)
				minions.erase(id);
			else
				towers.erase(id);

			pieces.erase(pieces.begin() + i, pieces.begin() + i + 1);
			return;
		}
	}
}

const int NUM_OF_STRATEGIES = 10;
class WarWizard : public WarPiece
{
public:
	enum MissileTargetStrategy
	{
		RUN,  // отступление
		BUILDING,  // по зданию
		ILL_WIZARD,  // по самому больному, но волшебники в приоритете
		NEAREST_WIZARD,  // по самому близкому, но волшебники в приоритете
		ILL_STAFF,  // бьём палкой больных
		NEAREST_STAFF,  // бьём палкой ближайшего
		ILL,  // по самому больному
		NEAREST,  // оборона - по самому близкому
		ILL_ARTILLERY,  // по самому больному, но дальнобойные в приоритете
		NEAREST_ARTILLERY    // по самому близкому, но дальнобойные в приоритете
	}
											strategy = ILL;

	/* Неудачный тест
	double life_w = 1 / 10000.0;
	double dist_w = 1 / 360000.0;
	double wiz_w = 0.75;
	double fet_w = 0.25;
	double tow_w = 1;
	double W(WarPiece * piece)
	{
		return -life_w * (piece->life * piece->life) +
			-dist_w * getDist2To(piece) +
			wiz_w * (piece->type == WIZARD) +
			fet_w * (piece->type == MINION && isDistanced) +
			tow_w * (piece->type == BASE || piece->type == TOWER);
	}
	*/

	// лучше ли piece как цель, чем текущая установка
	bool isBetterMissileTarget(WarPiece * piece)
	{
		if (!piece->isValid())
			return false;
		if (piece->isImmortal())
			return false;
		if (piece->estimated_death_time != NEVER &&
			tick + tillShoot(piece) + ceil(sqrt(getDist2To(piece)) / game->getMagicMissileSpeed()) > piece->estimated_death_time)
			return false;

		if (strategy == ILL_STAFF || strategy == NEAREST_STAFF || strategy == RUN)
			return piece == staff_target;

		if (missile_target == NULL)  // всегда так
			return true;

		if ((strategy == ILL_WIZARD && piece->type == WIZARD && missile_target->type != WIZARD) ||
			(strategy == NEAREST_WIZARD && piece->type == WIZARD && missile_target->type != WIZARD) ||
			(strategy == ILL_ARTILLERY && piece->isDistanced && !missile_target->isDistanced) ||
			(strategy == NEAREST_ARTILLERY && piece->isDistanced && !missile_target->isDistanced) ||
			(strategy == BUILDING && piece->isBuilding && !missile_target->isBuilding))
			return true;

		if ((strategy == ILL_WIZARD && piece->type != WIZARD && missile_target->type == WIZARD) ||
			(strategy == NEAREST_WIZARD && piece->type != WIZARD && missile_target->type == WIZARD) ||
			(strategy == ILL_ARTILLERY && !piece->isDistanced && missile_target->isDistanced) ||
			(strategy == NEAREST_ARTILLERY && !piece->isDistanced && missile_target->isDistanced) ||
			(strategy == BUILDING && !piece->isBuilding))
			return false;

		if ((strategy == ILL && piece->life == missile_target->life && getDist2To(piece) < getDist2To(missile_target)) ||
			(strategy == ILL_WIZARD && piece->life == missile_target->life && getDist2To(piece) < getDist2To(missile_target)) ||
			(strategy == ILL_ARTILLERY && piece->life == missile_target->life && getDist2To(piece) < getDist2To(missile_target)) ||
			(strategy == BUILDING && getDist2To(piece) < getDist2To(missile_target)))
			return true;

		return (strategy == ILL && piece->life < missile_target->life) ||
			(strategy == NEAREST && getDist2To(piece) < getDist2To(missile_target)) ||
			(strategy == ILL_WIZARD && piece->life < missile_target->life) ||
			(strategy == NEAREST_WIZARD && getDist2To(piece) < getDist2To(missile_target)) ||
			(strategy == ILL_ARTILLERY && piece->life < missile_target->life) ||
			(strategy == NEAREST_ARTILLERY && getDist2To(piece) < getDist2To(missile_target));
	}
	bool isBetterStaffTarget(WarPiece * piece)
	{
		if (!piece->isValid())
			return false;
		if (piece->isImmortal())
			return false;
		if (piece->estimated_death_time != NEVER &&
			tick + till(ACTION_STAFF) > piece->estimated_death_time)
			return false;

		if (strategy != ILL_STAFF && strategy != NEAREST_STAFF && strategy != RUN)  // всегда так
			return false;
		if (staff_target == NULL)
			return true;

		if ((strategy == ILL_STAFF && piece->life == staff_target->life && getDist2To(piece) < getDist2To(staff_target)))
			return true;

		return (strategy == ILL_STAFF && piece->life < staff_target->life) ||
			(strategy == NEAREST_STAFF && getDist2To(piece) < getDist2To(staff_target)) ||
			(strategy == RUN           && getDist2To(piece) < getDist2To(staff_target));
	}

	// из списка выбрать цель по стратегии
	WarPiece * findTargetToShoot(const vector<WarPiece*> & war)
	{
		missile_target = NULL;
		for (WarPiece * piece : war)
		{
			if (piece->isFriend != isFriend && isBetterMissileTarget(piece))
			{
				missile_target = piece;
			}
		}
		return missile_target;
	}
	WarPiece * findTargetToBeat(const vector<WarPiece*> & war)
	{
		staff_target = NULL;
		for (WarPiece * piece : war)
		{
			if (piece->isFriend != isFriend && isBetterStaffTarget(piece))
			{
				staff_target = piece;
			}
		}
		return staff_target;
	}

	map<MissileTargetStrategy, string> StrategyName = {
		{ ILL_STAFF, "Ill staff" },
		{ NEAREST_STAFF, "Near staff" },
		{ ILL, "Ill" },
		{ NEAREST, "Near" },
		{ ILL_WIZARD, "Ill wizard" },
		{ NEAREST_WIZARD, "Near wizard" },
		{ ILL_ARTILLERY, "Ill artillery" },
		{ NEAREST_ARTILLERY, "Near artillery" },
		{ BUILDING, "TOWERS!" },
		{ RUN, "RUN!" } };
	// наоборот, определить стратегию по таргету. Для этого задаём очередности перебора
	const MissileTargetStrategy enemy_strategies[NUM_OF_STRATEGIES] =
	{
		WarWizard::ILL_WIZARD,
		WarWizard::NEAREST_WIZARD,
		WarWizard::ILL_ARTILLERY,
		WarWizard::NEAREST_ARTILLERY,
		WarWizard::ILL,
		WarWizard::NEAREST,
		WarWizard::ILL_STAFF,
		WarWizard::NEAREST_STAFF,
		WarWizard::BUILDING,
		WarWizard::RUN
	};
	const MissileTargetStrategy strategies[NUM_OF_STRATEGIES] = {
		RUN,
		BUILDING,
		ILL_WIZARD,
		NEAREST_WIZARD,
		ILL_ARTILLERY,
		NEAREST_ARTILLERY,
		ILL,
		NEAREST,
		ILL_STAFF,
		NEAREST_STAFF
	};
	void updateStrategy(WarPiece * target)
	{
		MissileTargetStrategy prev_strategy = strategy;
		vector <WarPiece*> war;

		for (WarPiece* piece : pieces)
			if (piece->isInBattle(getArea(x, y)))
				war.push_back(piece);

		// до финала, наверное, имело смысло сделать, что если исходная подходит, то не менять!

		bool success = false;
		for (int i = 0; i < NUM_OF_STRATEGIES; ++i)
		{
			strategy = isFriend ? strategies[i] : enemy_strategies[i];
			if (findTargetToShoot(war) == target)
			{
				if (strategy != prev_strategy)
				{
					//////////////cout << tick << ") " << shooter->id << " has changed strategy to " << StrategyName[i] << endl;
				}

				success = true;
				break;
			}
		}

		if (!success)
			strategy = prev_strategy;
		missile_target = target;
	}

	// оптимальная точка, кажется, не зависит от speedCoeff.
	// Хотя вблизи можно стрелять куда угодно, но там тогда неважно, куда именно.
	Point tar_pos(WarPiece * shooter)
	{
		return pos() + R * (!isFriend) * (game->getWizardForwardSpeed() - (double)game->getWizardBackwardSpeed()) /
		(game->getWizardBackwardSpeed() + game->getWizardForwardSpeed()) * Point(angle);
	}
	double saveRocketR() { return R; }
	double getDistToShoot(WarPiece * target)
	{
		if (target->type != WIZARD || !isFriend)
			return getRange();

		double angle_to_turn = abs(PI / 2 - abs(target->getAngleTo(this)));

		double spd = dynamic_cast<WarWizard *>(target)->speedForGoingInAngle(angle_to_turn);
		return Min(getRange(), game->getWizardCastRange() * game->getWizardForwardSpeed() / spd);
	}
	double getDistToShoot() { return getDistToShoot(missile_target); }

	// храним кулдауны до действий
	vector<int> cooldownTicks;
	int actionCooldownTicks;
	int mana;

	WarWizard(const Wizard * wizard) :
		WarPiece(WIZARD, wizard->getId(), wizard->getX(), wizard->getY(), wizard->getRadius(), wizard->getLife(), true, SIT_HOME)
	{
		cooldownTicks = vector<int>(6, 0);
		actionCooldownTicks = 0;
		maxLife = wizard->getMaxLife();
		mana = wizard->getMana();
		maxMana = wizard->getMaxMana();
	}
	WarWizard(int id) : WarPiece(WIZARD, id, 3800, 200, game->getWizardRadius(), self->getMaxLife(), false, SIT_HOME)
	{
		cooldownTicks = vector<int>(6, 0);
		actionCooldownTicks = 0;
		maxLife = self->getMaxLife();
		maxMana = self->getMaxMana();
	}

	int maxLife = 0;
	int maxMana = 0;

	int last_death = 0;  // время предыдущей смерти
	int time_to_ressurection = 0;  // время до возрождения c момента смерти
	void update(const Wizard * wizard)
	{
		x = wizard->getX();
		y = wizard->getY();
		angle = wizard->getAngle();
		life = wizard->getLife();
		skills = wizard->getSkills();
		maxLife = wizard->getMaxLife();
		mana = wizard->getMana();
		maxMana = wizard->getMaxMana();
		sum_xp = wizard->getXp();
		statuses = wizard->getStatuses();
		processStatuses();

		actionCooldownTicks = wizard->getRemainingActionCooldownTicks();
		cooldownTicks = wizard->getRemainingCooldownTicksByAction();

		if (status == DEAD)  // значит, произошло возрождение.
		{
			if (!isFriend)
			{
				//////////////cout << tick << ") " << "ERROR! Zombie wizard!" << endl;
				status = GO_INTO_BATTLE;
			}
			else
			{
				last_death = tick - time_to_ressurection;
				status = SIT_HOME;
			}
		}
		else if (status == BATTLE_FROM_SHADOW)  // если он прятался в тени
		{
			status = BATTLE;
			//////////////cout << tick << ") " << "Wizard " << id << " appears from shadow!" << endl;
		}
		else if (last_update != tick - 1 && tick != 0)
		{
			//cout << tick << ") " << "Piece " << id << " returns!" << endl;
			status = GO_INTO_BATTLE;
		}

		last_update = tick;
		time_of_actual_data = tick;
	}
	bool checkIfWasNotUpdated()
	{
		if (isVisible())
			return false;

		if (getStatusesUpdated())
			return true;

		if (status == DEAD)
		{
			if (!isFriend && last_death + time_to_ressurection <= tick)
			{
				status = SIT_HOME;
				x = 3800;
				y = 200;
				life = maxLife;
				//////////////cout << tick << ") " << "Wizard " << id << " ressurected!" << endl;
			}
			else
				return false;
		}

		if (isFriend)  // не обновившийся визард-друг = он умер
		{
			status = DEAD;
			//////////////cout << tick << ") " << "Wizard " << id << " died!" << endl;

			// формула из правил
			time_to_ressurection = Max(game->getWizardMaxResurrectionDelayTicks() - (tick - last_death - time_to_ressurection),
									   game->getWizardMinResurrectionDelayTicks());
			last_death = tick;
			return false;
		}

		if (last_update == tick - 1)  // если только-только исчез
		{
			if (couldEscape().empty())
			{
				// не мог - значит, умер.
				status = DEAD;
				//////////////cout << tick << ") " << "Wizard " << id << " died!" << endl;

				time_to_ressurection = Max(game->getWizardMaxResurrectionDelayTicks() - (tick - last_death - time_to_ressurection),
					game->getWizardMinResurrectionDelayTicks());
				last_death = tick;
			}
			else
			{
				//////////////cout << tick << ") " << "Wizard " << id << " is out of vision!" << endl;

				/*if (status == RETREAT)
				{
					status = UNKNOWN;
					disappear_tick = tick;
					will_come_in = (wizard->getMaxLife() / 2 - wizard->getLife()) / game->getWizardBaseLifeRegeneration();
				}*/
				status = BATTLE_FROM_SHADOW;
			}
		}

		if (WAS_DEAD && last_update != NEVER)
			status = BATTLE_FROM_SHADOW;

		if (tick - time_of_actual_data >= MISSED_TIME)
		{
			//////cout << tick << ") Hope wizard " << id << " is far away!" << endl;
			status = UNKNOWN;
		}

		return false;
	}

	void madeShot(Point P)
	{
		x = P.x, y = P.y;
		status = BATTLE_FROM_SHADOW;
		time_of_actual_data = tick;
	}


	int time_of_actual_data = NEVER;
	double actuality()
	{
		return time_of_actual_data == NEVER ? 0 : 1 - ((double)tick - time_of_actual_data) / MISSED_TIME;
	}

	void updateSitHome()
	{
		AREA area = getArea(x, y);
		if (status == SIT_HOME)
		{
			//cout << x << " " << y << endl;
			if ((area != MY_HOME && isFriend) || (!isFriend && area != ENEMY_HOME))
			{
				status = GO_INTO_BATTLE;

				//cout << tick << ") " << "Wizard " << id << " goes to the battle! will be in " << timeToWarSegment() << " time!" << endl;
			}
			else if (isEnemyNear())
			{
				status = BATTLE;
				//cout << tick << ") Wizard " << id << " fights for home!" << endl;
			}
		}
	}
	void updateShadows()
	{
		if (status == BATTLE_FROM_SHADOW)
		{
			// есть две проблемы: забивание в угол возле дома врага и устаревание инфы

			life = Min((double)maxLife, life + regeneration());
			for (int i = 0; i < cooldownTicks.size(); ++i)
				if (cooldownTicks[i] > 0)
					--cooldownTicks[i];
			if (actionCooldownTicks > 0)
				--actionCooldownTicks;
			mana = Min((double)maxMana, mana + manaRegeneration());
			life = Min((double)maxLife, life + regeneration());

			Point newPos = PushFromVisible(pos());
			if (newPos == pos())
			{
				WarPiece * nearest = NULL;
				double near = INFINITY;
				for (auto P : pieces)
				{
					if (P->isFriend && P->isValid())
					{
						double dist2 = getDist2To(P);
						if (dist2 < near)
						{
							near = dist2;
							nearest = P;
						}
					}
				}

				Point target = nearest->pos() + (pos() - nearest->pos()).make_module(nearest->visionRange());

				if ((target - pos()).norm2() > getAverageSpeed() * getAverageSpeed())
					newPos = pos() + (target - pos()).make_module(getAverageSpeed());
				else
					newPos = target;
			}
			else if (newPos == POINT_NOT_FOUND)
			{
				newPos = getNearestUnvisible(lane(), getWarSegment(lane(), true).far, 1.2);
				if (isSpaceWrong(newPos))
					status = UNKNOWN;
			}

			x = newPos.x, y = newPos.y;
		}
	}

	// ужас
	int sum_xp = 0;
	vector<SkillType> skills;
	int _passiveRange = 0;
	int _auraRange = 0;
	bool _mm_cooldown = true;
	int _passiveMMDamage = 0;
	int _auraMMDamage = 0;
	bool canFroze = false;
	int _passiveStaffDamage = 0;
	int _auraStaffDamage = 0;
	bool canFireball = false;
	int _passiveSpeed = 0;
	int _auraSpeed = 0;
	bool canHaste = false;
	int _passiveShield = 0;
	int _auraShield = 0;
	bool canShield = false;
	void nullSkills()
	{
		_passiveRange = 0;
		_auraRange = 0;
		_mm_cooldown = true;
		_passiveMMDamage = 0;
		_auraMMDamage = 0;
		canFroze = false;
		_passiveStaffDamage = 0;
		_auraStaffDamage = 0;
		canFireball = false;
		_passiveSpeed = 0;
		_auraSpeed = 0;
		canHaste = false;
		_passiveShield = 0;
		_auraShield = 0;
		canShield = false;
	}
	void processSkills()
	{
		for (int i = 0; i < skills.size(); ++i)
		{
			if (skills[i] == SKILL_RANGE_BONUS_PASSIVE_1)
				_passiveRange += 1;
			else if (skills[i] == SKILL_RANGE_BONUS_AURA_1)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraRange = Max(ww->_auraRange, 1);
					}
				}
			}
			else if (skills[i] == SKILL_RANGE_BONUS_PASSIVE_2)
				_passiveRange += 1;
			else if (skills[i] == SKILL_RANGE_BONUS_AURA_2)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraRange = Max(ww->_auraRange, 2);
					}
				}
			}
			else if (skills[i] == SKILL_ADVANCED_MAGIC_MISSILE)
				_mm_cooldown = false;
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_1)
				_passiveMMDamage += 1;
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_BONUS_AURA_1)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraMMDamage = Max(ww->_auraMMDamage, 1);
					}
				}
			}
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_BONUS_PASSIVE_2)
				_passiveMMDamage += 1;
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_BONUS_AURA_2)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraMMDamage = Max(ww->_auraMMDamage, 2);
					}
				}
			}
			else if (skills[i] == SKILL_FROST_BOLT)
				canFroze = true;
			else if (skills[i] == SKILL_STAFF_DAMAGE_BONUS_PASSIVE_1)
				_passiveStaffDamage += 1;
			else if (skills[i] == SKILL_STAFF_DAMAGE_BONUS_AURA_1)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraStaffDamage = Max(ww->_auraStaffDamage, 1);
					}
				}
			}
			else if (skills[i] == SKILL_STAFF_DAMAGE_BONUS_PASSIVE_2)
				_passiveStaffDamage += 1;
			else if (skills[i] == SKILL_STAFF_DAMAGE_BONUS_AURA_2)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraStaffDamage = Max(ww->_auraStaffDamage, 2);
					}
				}
			}
			else if (skills[i] == SKILL_FIREBALL)
				canFireball = true;
			else if (skills[i] == SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_1)
				_passiveSpeed += 1;
			else if (skills[i] == SKILL_MOVEMENT_BONUS_FACTOR_AURA_1)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraSpeed = Max(ww->_auraSpeed, 1);
					}
				}
			}
			else if (skills[i] == SKILL_MOVEMENT_BONUS_FACTOR_PASSIVE_2)
				_passiveSpeed += 1;
			else if (skills[i] == SKILL_MOVEMENT_BONUS_FACTOR_AURA_2)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraSpeed = Max(ww->_auraSpeed, 2);
					}
				}
			}
			else if (skills[i] == SKILL_HASTE)
				canHaste = true;
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_1)
				_passiveShield += 1;
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_1)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraShield = Max(ww->_auraShield, 1);
					}
				}
			}
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_ABSORPTION_PASSIVE_2)
				_passiveShield += 1;
			else if (skills[i] == SKILL_MAGICAL_DAMAGE_ABSORPTION_AURA_2)
			{
				for (auto it : wizards)
				{
					WarWizard * ww = it.second;
					if (ww->isFriend == isFriend && getDist2To(ww) <= game->getAuraSkillRange() * game->getAuraSkillRange())
					{
						ww->_auraShield = Max(ww->_auraShield, 2);
					}
				}
			}
			else if (skills[i] == SKILL_SHIELD)
				canShield = true;
		}
	}

	double manaRegeneration()
	{
		return game->getWizardBaseManaRegeneration() + skills.size() * game->getWizardManaRegenerationGrowthPerLevel();
	}
	double regeneration()
	{
		return game->getWizardBaseLifeRegeneration() + skills.size() * game->getWizardLifeRegenerationGrowthPerLevel();
	}
	double getRange()
	{
		return game->getWizardCastRange() + (_passiveRange + _auraRange) * game->getRangeBonusPerSkillLevel();
	}
	int magicMissileCooldown()
	{
		return _mm_cooldown ? game->getMagicMissileCooldownTicks() : 0;
	}
	double missileDamage()
	{
		return game->getMagicMissileDirectDamage() * (1 + (empowered > 0) * game->getEmpoweredDamageFactor())
			+ (_passiveMMDamage + _auraMMDamage) * game->getMagicalDamageBonusPerSkillLevel();
	}
	double staffDamage()
	{
		return game->getStaffDamage() + (_passiveStaffDamage + _auraStaffDamage) * game->getStaffDamageBonusPerSkillLevel();
	}
	int damage(double dam, bool magical)
	{
		if (magical)
			return Max(0, (int)(ceil)(dam * (1 - game->getShieldedDirectDamageAbsorptionFactor()) - (_passiveShield + _auraShield) * game->getMagicalDamageAbsorptionPerSkillLevel()));
		else
			return WarPiece::damage(dam, magical);
	}

	double attack()
	{
		return actuality() *
			  (missileDamage() / (2.0 * game->getWizardActionCooldownTicks()) +
			   staffDamage() / (2.0 * game->getWizardActionCooldownTicks()) +
			   canFroze * 2 * game->getFrostBoltDirectDamage() / game->getFrostBoltCooldownTicks() +
			   canFireball * (game->getFireballExplosionMaxDamage() + game->getBurningSummaryDamage()) / game->getFireballCooldownTicks());
	}
	double defence()
	{
		return actuality() * sqrt(life) * sqrt(maxLife);  // это мне приснилось
	}
	double xp() { return maxLife; }
	double xp_for_damage(double dam) { return dam * game->getWizardDamageScoreFactor(); }
	double visionRange()
	{
		return game->getWizardVisionRange();
	}
	double getSpeedCoeff()
	{
		double ans = 1 + (_passiveSpeed + _auraSpeed) * game->getMovementBonusFactorPerSkillLevel() +
			(hastened > 0) * game->getHastenedMovementBonusFactor();
		return ans;
	}
	double getTurnCoeff()
	{
		double ans = 1 + (_passiveSpeed + _auraSpeed) * game->getMovementBonusFactorPerSkillLevel()
			+ (hastened > 0) * game->getHastenedRotationBonusFactor();
		return ans;
	}
	double getAverageSpeed() { return game->getWizardForwardSpeed() * getSpeedCoeff(); }
	double maxTurnAngle() { return game->getWizardMaxTurnAngle() * getTurnCoeff(); }
	set <Point> possiblePositions()
	{
		// просто перебираются варианты
		set<Point> ans;

		Point Me = Point(x, y);
		ans.insert(Me);

		Point straight(cos(angle), sin(angle));
		double Max_straight = game->getWizardForwardSpeed() * getSpeedCoeff();
		ans.insert(Me + Max_straight * straight);

		Point backward(-cos(angle), -sin(angle));
		double Max_backward = game->getWizardBackwardSpeed() * getSpeedCoeff();
		ans.insert(Me + Max_backward * backward);

		Point strafe(cos(angle + PI / 2), sin(angle + PI / 2));
		double Max_strafe = game->getWizardStrafeSpeed() * getSpeedCoeff();
		ans.insert(Me + Max_strafe * strafe);
		ans.insert(Me - Max_strafe * strafe);

		double k = sqrt(Max_strafe * Max_strafe + Max_straight * Max_straight);
		ans.insert(Me + Max_straight / k * straight + Max_strafe / k * strafe);
		ans.insert(Me + Max_straight / k * straight - Max_strafe / k * strafe);

		k = sqrt(Max_strafe * Max_strafe + Max_backward * Max_backward);
		ans.insert(Me + Max_backward / k * backward + Max_strafe / k * strafe);
		ans.insert(Me + Max_backward / k * backward - Max_strafe / k * strafe);

		return ans;
	}

	int manacost(ActionType action)
	{
		if (action == ACTION_MAGIC_MISSILE)
			return game->getMagicMissileManacost();
		if (action == ACTION_FROST_BOLT)
			return game->getFrostBoltManacost();
		if (action == ACTION_FIREBALL)
			return game->getFireballManacost();
		if (action == ACTION_HASTE)
			return game->getHasteManacost();
		if (action == ACTION_SHIELD)
			return game->getShieldManacost();

		return 0;
	}
	int till(ActionType action)
	{
		int mana_time = Max(frozen, (int)ceil((manacost(action) - mana) / manaRegeneration()));
		return Max(mana_time, Max(cooldownTicks[action], actionCooldownTicks));
	}
	int tillShoot(WarPiece * target)
	{
		int tillFreeze = willFreeze(target) ? till(ACTION_FROST_BOLT) : 10000;
		int tillFireball = willFireball(target) ? till(ACTION_FIREBALL) : 10000;
		return Min(till(ACTION_MAGIC_MISSILE), Min(tillFreeze, tillFireball));
	}
	int timeToStaff(Obstacle * ob)
	{
		return Max(
			ceil(Max(abs(abs(getAngleTo(ob)) - game->getStaffSector() / 2), 0.0) / (game->getWizardMaxTurnAngle() * getSpeedCoeff())),
			ceil(Max(0.0, sqrt(getDist2To(ob)) - game->getStaffRange()) / (getAverageSpeed() * getSpeedCoeff())));
	}
	int timeToMissile(Obstacle * ob)  // подразумевается дерево
	{
		return Max(
			ceil(Max(abs(abs(getAngleTo(ob)) - game->getStaffSector() / 2), 0.0) / (game->getWizardMaxTurnAngle() * getSpeedCoeff())),
			ceil(Max(0.0, sqrt(getDist2To(ob)) - getRange()) / (getAverageSpeed() * getSpeedCoeff())));
	}
	int timeToMissile(WarPiece * ob)
	{
		return Max(
			ceil(Max(abs(abs(getAngleTo(ob->tar_pos(this))) - game->getStaffSector() / 2), 0.0) / (game->getWizardMaxTurnAngle() * getSpeedCoeff())),
			ceil(Max(0.0, sqrt(getDist2To(ob->tar_pos(this))) - getDistToShoot(ob)) / (getAverageSpeed() * getSpeedCoeff())));
	}
	bool canStaff(Obstacle * ob)
	{
		return actionCooldownTicks == 0 && cooldownTicks[ACTION_STAFF] == 0 &&
			abs(getAngleTo(ob)) <= game->getStaffSector() / 2 &&
			getDist2To(ob) <= (game->getStaffRange() + ob->R) * (game->getStaffRange() + ob->R);
	}
	bool canAttack(Obstacle * ob)  // подразумевается дерево
	{
		return actionCooldownTicks == 0 && cooldownTicks[ACTION_MAGIC_MISSILE] == 0 &&
			abs(getAngleTo(ob)) <= game->getStaffSector() / 2 &&
			getDist2To(ob) <= getRange() * getRange();
	}
	bool canAttack(WarPiece * ob)
	{
		return tillShoot(ob) == 0 &&
			abs(getAngleTo(ob->tar_pos(this))) <= game->getStaffSector() / 2 &&
			(ob->tar_pos(this) - pos()).norm2() <= getRange() * getRange();
	}
	bool willFreeze(WarPiece * target)
	{
		if (!canFroze)
			return false;
		if (target->frozen > game->getWizardActionCooldownTicks())
			return false;

		if (!isFriend)
			return true;

		if (target->life <= missileDamage())
			return false;
		if (target->type == TOWER || target->type == BASE)
			return target->life <= game->getFrostBoltDirectDamage();
		if (target->type == WIZARD)
			return true;
		if (target->isDistanced && sqrt(getDist2To(target)) <= game->getFetishBlowdartAttackRange() + mana + maxLife - life)
			return true;

		// приснилось
		return sqrt(getDist2To(target)) <= 3 * (maxLife - life) + game->getOrcWoodcutterAttackRange() + 3 * mana;
	}
	bool willFireball(WarPiece * target)
	{
		if (!canFireball)
			return false;

		if (getDist2To(target->tar_pos(this)) <= (game->getFireballExplosionMinDamageRange() + R) * (game->getFireballExplosionMinDamageRange() + R))
			return false;

		return true;
	}

	const int HOW_FAR_GO_IN_DIRECTION = 10;
	double speedForGoingInAngle(double angle_to_turn)
	{
		double speed_limit2 = game->getWizardForwardSpeed() * getSpeedCoeff();
		speed_limit2 *= speed_limit2;
		if (abs(angle_to_turn) >= PI / 2)
		{
			speed_limit2 = game->getWizardBackwardSpeed() * getSpeedCoeff();
			speed_limit2 *= speed_limit2;
		}

		double strafe_limit2 = game->getWizardStrafeSpeed() * getSpeedCoeff();
		strafe_limit2 *= strafe_limit2;

		return sqrt(strafe_limit2 * speed_limit2 / (strafe_limit2 + (speed_limit2 - strafe_limit2) * pow(sin(angle_to_turn), 2)));
	}
	void goInDirection(Point direction)
	{
		Go(pos() + direction.make_module(HOW_FAR_GO_IN_DIRECTION));
	}
	void Go(Point target, int ticks = 1)  // двигаться в направлении таргета. Заполняет свой Move move.
	{
		double angle_to_turn = getAngleTo(target);
		move.setTurn(limit(angle_to_turn, maxTurnAngle() * ticks));

		double C = speedForGoingInAngle(angle_to_turn);

		move.setStrafeSpeed(sin(angle_to_turn) * C * ticks);
		move.setSpeed(cos(angle_to_turn) * C * ticks);
	}

	void NextTick(Battle * battle, int ticks);
};

set <int> IDS;  // список встретившихся миньонов
int shadow_ids = -1;  // id для таинственных миньонов врага, которые только идут, но их не видно.
class WarMinion : public WarPiece
{
	MinionType m_type;
	LaneType my_lane;

	int attackCooldownTicks;

public:
	WarMinion(const Minion * minion) : WarPiece(MINION, minion->getId(), minion->getX(), minion->getY(), minion->getRadius(),
		                                     minion->getLife(), minion->getFaction() == self->getFaction(), GO_INTO_BATTLE)
	{
		if (IDS.count(id))
		{
			//////////////cout << tick << ") " << "ERROR! that Minion already died!" << endl;
		}

		this->m_type = minion->getType();
		this->my_lane = getLane(x, y);
		last_update = tick;
		if (m_type == MINION_ORC_WOODCUTTER) isDistanced = false;

		if (isFriend)
		{
			//////////////cout << tick << ") " << "Minion " << id << " appeared: will be in battle in " << timeToWarSegment() << " time!" << endl;
		}
		else
		{
			if (!IDS.count(id))
			{
				int i = 0;
				int NumOfShadowminions = shadow_minions.size();
				while (i < NumOfShadowminions &&
					(shadow_minions[i]->m_type != m_type || shadow_minions[i]->my_lane != my_lane))
					++i;

				if (i < NumOfShadowminions)
				{
					//////////////cout << tick << ") " << "ShadowMinion " << shadow_minions[i]->id << " is found! His id is " << id << endl;
					killPiece(shadow_minions[i]->id);
					shadow_minions.erase(shadow_minions.begin() + i, shadow_minions.begin() + i + 1);
				}
				else
				{
					//////////////cout << tick << ") " << "ERROR: shadow Minion not found" << endl;
				}
			}

			status = BATTLE;
		}

		IDS.insert(id);
	}
	WarMinion(int x, int y, MinionType m_type) : WarPiece(MINION, shadow_ids, x, y, game->getMinionRadius(), 100, false, GO_INTO_BATTLE)
	{
		pieces.push_back(this);
		shadow_ids -= 1;

		this->m_type = m_type;
		this->my_lane = getLane(x, y);
		if (m_type == MINION_ORC_WOODCUTTER) isDistanced = false;

		//////////////cout << tick << ") " << "Shadow Minion " << id << " appeared: will be in battle in " << timeToWarSegment() << " time!" << endl;
	}

	void update(const Minion * minion)
	{
		x = minion->getX();
		y = minion->getY();
		angle = minion->getAngle();
		attackCooldownTicks = minion->getCooldownTicks();
		life = minion->getLife();
		last_update = tick;
		processStatuses();

		if (status == BATTLE_FROM_SHADOW)  // если он прятался в тени
		{
			status = BATTLE;
			//////////////cout << tick << ") " << "Minion " << id << " appears from shadow!" << endl;
		}
	}
	bool checkIfWasNotUpdated()
	{
		if (isVisible())
			return false;

		if (isFriend)
		{
			//////////////cout << tick << ") " << "  Minion " << id << " died :(" << endl;
			return true;
		}

		if (getStatusesUpdated())
			return true;

		if (last_update == tick - 1)
		{
			if (couldEscape().empty())
			{
				//////////////cout << tick << ") " << "  Minion " << id << " died :)" << endl;
				return true;
			}

			//////////////cout << tick << ") " << "  Minion " << id << " is out of vision!" << endl;
			status = BATTLE_FROM_SHADOW;
		}

		if (WAS_DEAD && last_update != NEVER)
		{
			//////////////cout << tick << ") " << "  hope Minion " << id << " died :/" << endl;
			return true;
		}

		if (last_update != NEVER && tick - last_update > MISSED_TIME)
		{
			//////////////cout << tick << ") " << "  didn't see minion " << id << " for long time. Hope he died :/" << endl;
			return true;
		}

		return false;
	}
	void updateShadows()
	{
		if (status == BATTLE_FROM_SHADOW)
		{
			if (attackCooldownTicks > 0)
				--attackCooldownTicks;

			Point newPos = PushFromVisible(pos());
			if (newPos == pos())
			{
				Point target = (isFriend ? 1 : -1) * WhereAtLine(x, y).getVector();
				newPos = pos() + getAverageSpeed() * target;

				// не заморачиваясь, будем считать, что он остаётся на месте.
				if (isPointVisible(newPos))
					newPos = pos();
			}
			else if (newPos == POINT_NOT_FOUND)
			{
				newPos = getNearestUnvisible(lane(), getWarSegment(lane(), true).far, 1.2);
			}

			x = newPos.x, y = newPos.y;
		}
	}

	Point tar_pos(WarPiece * shooter) { return pos() + R * Point(angle); }
	LaneType lane() { return my_lane; }
	double xp() { return game->getMinionLife() * game->getMinionEliminationScoreFactor(); }
	double getAverageSpeed() { return game->getMinionSpeed(); }
	double attack()
	{
		if (m_type == MINION_FETISH_BLOWDART)
			return game->getDartDirectDamage() / (double)game->getFetishBlowdartActionCooldownTicks();
		return game->getOrcWoodcutterDamage() / (double)game->getOrcWoodcutterActionCooldownTicks();
	}
	double visionRange()
	{
		return game->getMinionVisionRange();
	}
	double maxTurnAngle() { return game->getMinionMaxTurnAngle(); }
	set <Point> possiblePositions()
	{
		set<Point> ans;

		Point Me = Point(x, y);
		ans.insert(Me);

		Point straight(cos(angle), sin(angle));
		ans.insert(Me + game->getMinionSpeed() * straight);
		return ans;
	}

	bool canAttack(WarPiece * target)
	{
		if (m_type == MINION_ORC_WOODCUTTER)
		{
			return attackCooldownTicks == 0 && abs(getAngleTo(target)) < game->getOrcWoodcutterAttackSector() &&
				getDist2To(target) < (game->getOrcWoodcutterAttackRange() + target->R) * (game->getOrcWoodcutterAttackRange() + target->R);
		}

		return attackCooldownTicks == 0 && abs(getAngleTo(target)) < game->getFetishBlowdartAttackSector() &&
			getDist2To(target) < game->getFetishBlowdartAttackRange() * game->getFetishBlowdartAttackRange();
	}

	void goInDirection(Point direction)
	{
		double angle_to_turn = normalize(direction.angle() - angle);
		move.setTurn(limit(angle_to_turn, maxTurnAngle()));
		if (abs(angle_to_turn) <= PI / 2)
			move.setSpeed(game->getMinionSpeed());
		else
			move.setSpeed(0);
	}
	void Go(Point target, int ticks = 1)
	{
		double angle_to_turn = getAngleTo(target);
		move.setTurn(limit(angle_to_turn, maxTurnAngle() * ticks));
		if (abs(angle_to_turn) <= PI / 2)
			move.setSpeed(game->getMinionSpeed() * ticks);
		else
			move.setSpeed(0);
	}
	void NextTick(Battle * battle, int ticks);
};

class WarTower : public WarPiece
{
public:
	int attackCooldownTicks;

	AREA protect;
	bool isSecondOnLine = true;
	bool isImmortal()
	{
		if (type == BASE)
			return !anyLanePushed(!isFriend);
		return isSecondOnLine && countTowers(lane(), isFriend) == 2;
	}
	bool isInBattle(AREA A, bool may_be_soon = false)
	{
		if (type == BASE)
			return true;
		return (status == BATTLE || status == BATTLE_FROM_SHADOW) &&
			(isInArea(A) || protect == A || isNearMe);
	}

	Point tar_pos(WarPiece * shooter)
	{
		return pos() + (shooter->pos() - pos()).make_module(R);
	}
	double saveRocketR() { return 0; }

	WarTower(const Building * tower, bool isSecondOnLine) : WarPiece(tower->getType() == BUILDING_GUARDIAN_TOWER ? TOWER : BASE,
		tower->getId(), tower->getX(), tower->getY(), tower->getRadius(), tower->getMaxLife(), tower->getFaction() == self->getFaction(), BATTLE)
	{
		this->attackCooldownTicks = tower->getRemainingActionCooldownTicks();
		this->isSecondOnLine = isSecondOnLine;
		isBuilding = true;
		last_update = tick;
		protect = MY_HOME;
	}
	WarTower(int id, WarTower * opposite) : WarPiece(opposite->type, id, 4000 - opposite->x, 4000 - opposite->y, opposite->R, opposite->life, false, BATTLE_FROM_SHADOW)
	{
		this->attackCooldownTicks = opposite->attackCooldownTicks;
		this->isSecondOnLine = opposite->isSecondOnLine;
		isBuilding = true;
		last_update = tick;
		protect = ENEMY_HOME;
	}

	void update(const Building * tower)
	{
		last_update = tick;
		life = tower->getLife();
		attackCooldownTicks = tower->getRemainingActionCooldownTicks();
		status = BATTLE;
		processStatuses();

		if (abs(tower->getX() - x) > E || abs(tower->getY() - y) > E)
		{
			//////////////cout << "ERRRRORR!!! TOWERZ!!! " << id << endl;
		}
	}
	bool checkIfWasNotUpdated()
	{
		if (isVisible())
			return false;

		if (isFriend)
		{
			//////////////cout << tick << ") " << "TOWER " << id << " DIES!!! :(" << endl;
			return true;
		}

		if (getStatusesUpdated())
		{
			//////////cout << tick << ") Tower " << id << " is... burned?" << endl;
			return true;
		}

		if (isPointVisible(pos()))
		{
			//////////cout << tick << ") " << "TOWER " << id << " DIES!!! :)" << endl;
			return true;
		}
		else if (status != BATTLE_FROM_SHADOW)
		{
			//////////cout << tick << ") " << "Tower " << id << " is out of vision..." << endl;
			status = BATTLE_FROM_SHADOW;
		}

		return false;
	}
	void updateShadows()
	{
		if (status == BATTLE_FROM_SHADOW && attackCooldownTicks > 0)
			attackCooldownTicks--;
	}

	double getAverageSpeed() { return 0; }
	double attack()
	{
		if (type == TOWER)
			return game->getGuardianTowerDamage() / (double)game->getGuardianTowerCooldownTicks();
		return game->getFactionBaseDamage() / (double)game->getFactionBaseCooldownTicks();
	}
	double defence() { return 250 + sqrt(life); }
	double points(double dam) { return (type == BASE) * dam; }
	double xp() { return (type == BASE ? 100000 : game->getGuardianTowerLife()) * game->getBuildingEliminationScoreFactor(); }
	double xp_for_damage(double dam) { return dam * game->getBuildingDamageScoreFactor(); }
	double visionRange()
	{
		return type == TOWER ? game->getGuardianTowerVisionRange() : game->getFactionBaseVisionRange();
	}
	double getDamage() { return type == TOWER ? game->getGuardianTowerDamage() : game->getFactionBaseDamage(); }
	double attackRange()
	{
		return type != BASE ? game->getGuardianTowerAttackRange() : game->getFactionBaseAttackRange();
	}
	set <Point> possiblePositions()
	{
		set<Point> ans;
		Point Me = Point(x, y);
		ans.insert(Me);
		return ans;
	}

	void NextTick(Battle * battle, int ticks);
};

int countTowers(LaneType lane, bool friends)
{
	int left = 0;
	for (auto it : towers)
	{
		WarTower * tower = it.second;
		if (tower->type == TOWER && tower->isFriend == friends && tower->lane() == lane)
			++left;
	}
	return left;
}
double TowerLife(LaneType lane, bool friends)
{
	double ans = 0;
	for (auto it : towers)
	{
		WarTower * tower = it.second;
		if (tower->type == TOWER && tower->isFriend == friends && tower->lane() == lane)
			ans += tower->life;
	}
	return ans;
}
double nearestTower(LaneType lane)
{
	double ans = 0;
	for (auto it : towers)
	{
		WarTower * tower = it.second;
		if (!tower->isFriend && (tower->type == BASE || tower->lane() == lane))
			ans = Min(ans, WhereAtLine(tower->x, tower->y).far);
	}
	return ans;
}

class WarTree : public Obstacle
{
public:
	double life;
	int id;

	WarTree(const Tree & tree) : Obstacle(Point(tree.getX(), tree.getY()), tree.getRadius(), TREE)
	{
		id = tree.getId();
		life = tree.getLife();
		time_of_update = tick;
	}

	int time_of_update = 0;
	void update(const Tree & tree)
	{
		time_of_update = tick;
		life = tree.getLife();
	}
	bool checkIfWasNotUpdated()
	{
		return time_of_update != tick && isPointVisible(pos());
	}
};

// из данного вектора obstacles выбираем те, которые не покрывают точки А и B, а также заменяем
// все препятствия в радиусе RadiusOfClearing на такие же с меньшими на tree_reduce радиус.
vector<Obstacle> clear(const vector<Obstacle> & obstacles, Point A, Point B, double RadiusOfClearing, double tree_reduce = 0)
{
	vector<Obstacle> ans(obstacles.size());
	for (Obstacle c : obstacles)
	{
		if (!c.isIn(A) && !c.isIn(B))
		{
			if (!Circle(B, RadiusOfClearing).isInside(c))
				ans.push_back(c);
			else if (c.type == TREE)
				ans.push_back(Obstacle(c.pos(), c.R - tree_reduce, TREE));
		}
	}
	return ans;
}
vector<Obstacle> getObstacles(double additional_radius)
{
	vector<Obstacle> ans;

	for (Wizard w : world->getWizards())
		if (w.getId() != self->getId())
			ans.push_back(Obstacle(Point(w.getX(), w.getY()), w.getRadius() + additional_radius, WIZARD));
	for (Building b : world->getBuildings())
		ans.push_back(Obstacle(Point(b.getX(), b.getY()), b.getRadius() + additional_radius, TOWER));
	for (Minion m : world->getMinions())
		ans.push_back(Obstacle(Point(m.getX(), m.getY()), m.getRadius() + additional_radius, MINION));

	for (auto t : trees)
		ans.push_back(Obstacle(t.second->pos(), t.second->R + additional_radius, TREE));
	return ans;
}

