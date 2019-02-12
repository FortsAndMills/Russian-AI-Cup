#pragma once

#include "Physics.h"

vector <Bonus> all_bonuses;
vector <OilSlick> all_slicks;
vector <vector < vector < pair<bool, int> > > > bonusesOnTile;

void initBonuses()
{
	for (int x = 0; x < world->getWidth(); ++x)
	{
		for (int y = 0; y < world->getHeight(); ++y)
		{
			bonusesOnTile[x][y].clear();
		}
	}

	all_bonuses.clear();
	all_bonuses = world->getBonuses();
	all_slicks.clear();
	all_slicks = world->getOilSlicks();
	for (int i = 0; i < all_bonuses.size(); ++i)
	{
		bonusesOnTile[all_bonuses[i].getX() / game->getTrackTileSize()][all_bonuses[i].getY() / game->getTrackTileSize()].push_back(pair<bool, int>(true, i));
	}
	for (int i = 0; i < all_slicks.size(); ++i)
	{
		int x = all_slicks[i].getX() / game->getTrackTileSize();
		int y = all_slicks[i].getY() / game->getTrackTileSize();

		if (x >= 0 && x < world->getWidth() && y >= 0 && y <= world->getHeight())
			bonusesOnTile[x][y].push_back(pair<bool, int>(false, i));
	}
}
double estimateBonus(Bonus * B)
{
	if (B->getType() == REPAIR_KIT)
		return (game->getCarReactivationTimeTicks() + 100) * (1 - self[ID]->getDurability()) / 2 + 25;
	else if (B->getType() == AMMO_CRATE)
		return 25 + pow(3 / 4.0, self[ID]->getProjectileCount()) * 40;
	else if (B->getType() == NITRO_BOOST)
		return 70;
	else if (B->getType() == OIL_CANISTER)
	{
		return 35;
	}

		return game->getPureScoreAmount();
}
double estimateSlick(OilSlick * OS)
{
	return -150.0 * (double)OS->getRemainingLifetime() / (double)game->getOilSlickLifetime();
}
double estimateBonuses(Tile * T)
{
	double ans = 0;
	for (int k = 0; k < bonusesOnTile[T->x][T->y].size(); ++k)
	{
		bool isBonus = bonusesOnTile[T->x][T->y][k].first;
		int i = bonusesOnTile[T->x][T->y][k].second;
		if (isBonus)
		{
			Bonus * B = &(all_bonuses[i]);

			if (T->whereIs(Vector(B->getX(), B->getY())).inside)
			{
				double est = estimateBonus(B);

				if (T->type != HORIZONTAL && T->type != VERTICAL)
				{
					Vector Me(B->getX(), B->getY());
					Vector pos(T->edges[0].x, T->edges[0].y);
					est *= (1 - max(0.0, (double)(Me - pos).module() * sqrt(2) / game->getTrackTileSize() - 1)) / 2.0;
				}

				ans += est;
			}
		}
		else
		{
			OilSlick * OS = &(all_slicks[i]);

			ans += estimateSlick(OS);
		}
	}

	return ans;
}

class MyBonus;
map <int, vector <MyBonus *> > bonuses;
class MyBonus
{
public:
	double estimation;
	double real_estimation;
	int path_index;

	bool isTaken;
	bool isPlannedToBeTaken;

	double x, y;

	bool isBonus;
	int id;

	MyBonus(Vector point)
	{
		x = point.x;
		y = point.y;
		isTaken = false;
		isBonus = true;
	}
	MyBonus(pair<bool, int> ind, int _path_index) :
		MyBonus(ind.first, ind.second, _path_index)
	{

	}
	MyBonus(bool _isBonus, int i, int _path_index)
	{
		isBonus = _isBonus;
		if (isBonus)
		{
			x = all_bonuses[i].getX();
			y = all_bonuses[i].getY();
			real_estimation = estimateBonus(&(all_bonuses[i]));
			id = all_bonuses[i].getId();
		}
		else
		{
			x = all_slicks[i].getX();
			y = all_slicks[i].getY();
			real_estimation = estimateSlick(&(all_slicks[i]));
			id = all_slicks[i].getId();
		}

		isTaken = false;
		isPlannedToBeTaken = false;
		path_index = _path_index;

		considerCollegue();
	}
	void considerCollegue()
	{
		estimation = real_estimation;

		int i = 0;
		while (i < world->getCars().size() && (!world->getCars()[i].isTeammate() || world->getCars()[i].getId() == ID))
			++i;

		if (i != world->getCars().size())
		{
			int c_id = world->getCars()[i].getId();

			int j = 0;
			while (j < bonuses[c_id].size() && (!bonuses[c_id][j]->isPlannedToBeTaken || bonuses[c_id][j]->id != id))
				++j;

			if (j < bonuses[c_id].size() && bonuses[c_id][j]->real_estimation >= real_estimation)
			{
				estimation = real_estimation - bonuses[c_id][j]->real_estimation;
				return;
			}
		}
	}

	void checkIfTaken(PhysMe * P)
	{
		if (isTaken)
			return;

		if (isBonus)
		{
			Vector Me(x, y);
			double ang1 = (P->points[0] - Me).angle();
			double ang2 = (P->points[1] - Me).angle();
			double ang3 = (P->points[3] - Me).angle();
			double ang4 = (P->points[4] - Me).angle();

			isTaken = (norm(ang2 - ang1) > 0 && norm(ang3 - ang2) > 0 && norm(ang4 - ang3) > 0 && norm(ang1 - ang4) > 0) ||
				(norm(ang2 - ang1) < 0 && norm(ang3 - ang2) < 0 && norm(ang4 - ang3) < 0 && norm(ang1 - ang4) < 0);
		}
		else
		{
			isTaken = Vector(x - P->x, y - P->y).module() < game->getOilSlickRadius();
		}
	}
};