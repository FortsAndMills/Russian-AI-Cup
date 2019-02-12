#include "TroopersCheck.h"

const double NEED_TO_HELP_GET_K = 1;
const double BEST_BONUS = 10;
double DoesNeedBonus(const Trooper& t, BonusInfo& b)
{
	if ((b.type == FIELD_RATION && t.isHoldingFieldRation()) ||  // ќцениваем нужность бонуса.
		(b.type == GRENADE && t.isHoldingGrenade()) ||
		(b.type == MEDIKIT && t.isHoldingMedikit()))
    {
        return 0;  // ≈сли он уже есть, то нам как бы всЄ равно.
    }
	else if (b.type == FIELD_RATION)
    {
		return t.getStandingDamage() * ((double)(game.getFieldRationBonusActionPoints() - game.getFieldRationEatCost()) / (double)t.getShootCost());
    }
    else if (b.type == GRENADE)
    {
		return game.getGrenadeDirectDamage() - t.getShootingRange() * 3;
    }
    else
    {
		int best = 0;
		for (int i = 0; i < (int)troopers.size(); ++i)
		{
			int k;
			if (troopers[i].getId() == self.getId())
				k = max(game.getMedikitHealSelfBonusHitpoints(), 
				(troopers[i].getMaximalHitpoints() - troopers[i].getHitpoints()));
			else
				k = max(game.getMedikitBonusHitpoints(), 
				(troopers[i].getMaximalHitpoints() - troopers[i].getHitpoints()));

			if (k > best)
				best = k;
		}
        return best;
    }
}
double BonusDistance(BonusInfo& b)
{
	double ans = 0;
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		ans += GetMarkDistance(b.x, b.y, troopers[i].getX(), troopers[i].getY());
	}
	ans /= troopers.size();
	if (ans == 0)
		ans = 1;
	return ans;
}

void DeleteFromArchieve(vector<Bonus>& bonuses, const World& world)
{
	for (int i = 0; i < (int)BA.size(); ++i)
	{  // если видим бонус, а в массиве бонусов его нет.
		int j = 0;
		while (j < (int)troopers.size() &&
			!world.isVisible(troopers[j].getVisionRange(),
			troopers[j].getX(), troopers[j].getY(), troopers[j].getStance(),
			BA[i].x, BA[i].y, PRONE))
		{
			++j;
		}

		if (j < (int)troopers.size())
		{
			int k = 0;
			while (k < (int)bonuses.size() && 
				(BA[i].x != bonuses[k].getX() || BA[i].y != bonuses[k].getY()))
				++k;

			if (k == (int)bonuses.size())
			{
				BA.erase(BA.begin() + i, BA.begin() + i + 1);
				--i;
			}
		}
	}
}
void UpdateBonusArchieve(const World &world)
{
	vector <Bonus> bonuses = world.getBonuses();
	DeleteFromArchieve(bonuses, world);
	for (int i = 0; i < (int)bonuses.size(); ++i)
	{
		int j = 0;
		while (j < (int)BA.size() && 
			(BA[j].x != bonuses[i].getX() || BA[j].y != bonuses[i].getY()))
		{
			++j;
		}

		if (j == (int)BA.size())
		{
			BA.push_back(BonusInfo(bonuses[i]));
			BA.push_back(BonusInfo(bonuses[i], 1, world));
			BA.push_back(BonusInfo(bonuses[i], 2, world));
			BA.push_back(BonusInfo(bonuses[i], 3, world));
		}
	}
	DeleteFromArchieve(bonuses, world);
}

vector <double> Price;
vector < vector<int> > best_i;

int best_bonus_index;
double BestBonusPrice;
double best_dist;

void PriceBonuses(const World& world)
{
	BonusesCoord.clear();
	BonusTargets.clear();
	Price.clear();
	best_i.clear();

	best_bonus_index = -1;
	BestBonusPrice = 0;
	best_dist = 0;
	for (int i = 0; i < (int)BA.size(); ++i)  // ƒл€ каждого бонуса определ€ем
    {
		Price.push_back(0);
		best_i.push_back(vector<int>());
		for (int j = 0; j < (int)troopers.size(); ++j)
		{
			double cur = DoesNeedBonus(troopers[j], BA[i]);
			if (cur > Price[i])
			{
				Price[i] = cur;
				best_i[i].clear();
				best_i[i].push_back(j);
			}
			else if (cur == Price[i] && Price[i] != 0)
			{
				best_i[i].push_back(j);
			}
		}	

		double dist = BonusDistance(BA[i]);
		Price[i] /= dist * dist;
		if (Price[i] > BestBonusPrice || 
			(Price[i] == BestBonusPrice && dist < best_dist))
		{
			BestBonusPrice = Price[i];
			best_dist = dist;
			best_bonus_index = i;
		}

		BonusesCoord[BA[i].type].insert(make_pair(BA[i].x, BA[i].y));
	}
}

void BonusesCheck(const World& world)
{
	if (STATE == "peace")
		UpdateLocks(world);
	
	UpdateBonusArchieve(world);
	PriceBonuses(world);
	
	if (best_bonus_index < 0)
		return;

	Price[best_bonus_index] *= BEST_BONUS;
	for (int i = 0; i < (int)BA.size(); ++i)
	{
		if (Price[i] != 0)
		{
			HasCrusade = false;
			bool Do_I_Need = false;

			for (int k = 0; k < (int)best_i[i].size(); ++k)
			{
				if (best_i[i][k] == self_index)  // Ќужен ли он нашему труперу. ≈сли нужен, то говорим, что хорошо бы пойти за этим бонусом.
				{
					//cerr //<< "Need bonus " //<< BA[i].type //<< " on " 
					       //<< BA[i].x //<< ", " //<< BA[i].y 
					       //<< ", price is " //<< Price[i] //<< endl;

					BA[i].MoveTo(Price[i], world, i == best_bonus_index, true);
					Do_I_Need = true;
					break;
				}
			}

			if (!Do_I_Need)
			{
				Price[i] *= NEED_TO_HELP_GET_K;
				if (self.getX() != BA[i].x || self.getY() != BA[i].y)
				{
					//cerr //<< "Need to give someone another bonus " //<< BA[i].type //<< " on " 
							//<< BA[i].x //<< ", " //<< BA[i].y 
							//<< ", price is " //<< Price[i] //<< endl;

					BA[i].MoveTo(Price[i], world, i == best_bonus_index, false);
				}
				else
				{
					//cerr //<< "I am at the bonus, but should be near it" //<< endl;
					do_not_move -= Price[i];
					for (int j = 0; j < (int)price.size(); ++j)
					{
						price[j] += Price[i];
					}
				}
			}
		}
    }

	ClearCurDis();
	TargetDistance(BonusTargets, self.getX(), self.getY(), 0, world);
	for (int i = 0; i < (int)BonusTargets.size(); ++i)
	{
		if (BonusTargets[i].var_x.size() == 0)
			WantToGoHere(BonusTargets[i].x, BonusTargets[i].y, BonusTargets[i].price, 0, world);
		else
		{
			for (int j = 0; j < (int)BonusTargets[i].var_x.size(); ++j)
			{
				if (BonusTargets[i].var_x[j] == self.getX() &&
					BonusTargets[i].var_y[j] == self.getY())
				{
					//cerr //<< "Because of wanting to help get the bonus, want to stay in the circle!" //<< endl;
					do_not_move += BonusTargets[i].price;
					for (int k = 0; k < (int)price.size(); ++k)
					{
						double distance_to_bonus = (BonusTargets[i].x - new_x[k]) * (BonusTargets[i].x - new_x[k]) +
							(BonusTargets[i].y - new_y[k]) * (BonusTargets[i].y - new_y[k]);

						if (distance_to_bonus <= NEAR_EACH_OTHER * NEAR_EACH_OTHER
							&& distance_to_bonus != 0)
						{
							price[k] += BonusTargets[i].price;
						}
						else
						{
							//cerr //<< "5Move to (" //<< new_x[k] //<< ", " //<< new_y[k] //<< "): -("
								//<< BonusTargets[i].price //<< ")" //<< endl;
							price[k] -= BonusTargets[i].price;
						}
					}

					BonusTargets[i].var_x.erase(BonusTargets[i].var_x.begin() + j, BonusTargets[i].var_x.begin() + j + 1);
					BonusTargets[i].var_y.erase(BonusTargets[i].var_y.begin() + j, BonusTargets[i].var_y.begin() + j + 1);
					--j;
				}
			}

			if (BonusTargets[i].var_x.size() > 0)
				WantToGoHere(BonusTargets[i].var_x, BonusTargets[i].var_y, BonusTargets[i].price, 0, world);
		}
	}
}