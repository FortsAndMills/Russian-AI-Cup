#pragma once

#include "FieldProcess.h"

bool UseNitro()
{
	if (IsDrivingBack[ID])
		return false;
	if (world->getTick() <= game->getInitialFreezeDurationTicks())
		return false;
	if (self[ID]->getRemainingNitroTicks() != 0)
		return false;

	Path * P = path_variants[ID][chosen_variant[ID]];

	if (fabs(norm(self[ID]->getAngle() - target_angle)) < PI / 90 && fabs(self[ID]->getAngularSpeed()) < 0.1)
	{
		set <pair<int, int> > danger;
		vector <OilSlick> os = world->getOilSlicks();
		for (int i = 0; i < os.size(); ++i)
			danger.insert(pair<int, int>(os[i].getX() / game->getTrackTileSize(), os[i].getY() / game->getTrackTileSize()));

		if (danger.count(pair<int, int>(P->path[0]->x, P->path[0]->y)))
			return false;

		int est = 0;
		enum TYPES {STR, DIA, NO} T = NO;

		int i = 1;
		while (i < P->path.size() - 1 && i < 5)
		{
			TYPES w = NO;
			if (P->types[i] == STRAIGHT)
				w = STR;
			if (((P->types[i] == TURN_LEFT || P->types[i] == TURN_RIGHT) &&
				(((P->types[i + 1] == TURN_LEFT || P->types[i + 1] == TURN_RIGHT) && P->types[i] != P->types[i + 1]))))
				w = DIA;

			if (w == NO || danger.count(pair<int, int>(P->path[i]->x, P->path[i]->y)))
				return false;

			if (i == 1)
				T = w;
			else if (T != w)
			{
				T = w;
				est++;
			}

			++i;
		}

		if ((P->types[i] == TURN_LEFT || P->types[i] == TURN_RIGHT) && P->types[i] == P->types[i + 1])
			return false;

		return est < self[ID]->getNitroChargeCount();
	}

	return false;
}

bool SaveOil()
{
	for (int i = 0; i < world->getCars().size(); ++i)
	{
		if (world->getCars()[i].isTeammate() && world->getCars()[i].getId() != ID)
		{
			Path * path = path_variants[world->getCars()[i].getId()][chosen_variant[world->getCars()[i].getId()]];
			int x = self[ID]->getX() / game->getTrackTileSize();
			int y = self[ID]->getY() / game->getTrackTileSize();

			int i = 0;
			while (i < path->path.size() && (path->path[i]->x != x || path->path[i]->y != y))
				++i;

			return i < path->path.size();
		}
	}
	return false;
}
bool UseOil()
{
	if (world->getTick() <= game->getInitialFreezeDurationTicks())
		return false;
	if (SaveOil())
		return false;
	if (IsDrivingBack[ID])
		return false;

	double R = self[ID]->getWidth() / 2 + game->getOilSlickInitialRange() + 2 * game->getOilSlickRadius();

	vector <Car> cars = world->getCars();
	bool yes = false;
	for (int i = 0; i < cars.size(); ++i)
	{
		if (cars[i].getId() != ID)
		{
			Vector tar = Vector(cars[i].getX() - self[ID]->getX(), cars[i].getY() - self[ID]->getY());
			if (tar.module() < R)
			{
				double Ang = norm(tar.angle() - self[ID]->getAngle() - PI);
				if (fabs(Ang) <= PI / 6 * (1.2 - tar.module() / R))
				{
					if (cars[i].isTeammate())
						return false;

					yes = true;
				}
			}
		}
	}

	if (yes)
		return true;

	int X = self[ID]->getX() / game->getTrackTileSize();
	int Y = self[ID]->getY() / game->getTrackTileSize();
	if (self[ID]->getOilCanisterCount() > 1)
	{
		if (field[X][Y].type != VERTICAL && field[X][Y].type != HORIZONTAL)
		{
			Vector pos(self[ID]->getX(), self[ID]->getY());
			Vector ang(self[ID]->getAngle() + PI);

			Vector center = pos + ang * (self[ID]->getWidth() / 2 + game->getOilSlickInitialRange());

			double r = game->getOilSlickRadius() * 1;
			if (isInTrack(center + Vector(r, 0)) &&
				isInTrack(center + Vector(-r, 0)) &&
				isInTrack(center + Vector(0, r)) &&
				isInTrack(center + Vector(0, -r)))
			{
				return true;
			}			
		}
	}

	return false;
}

bool UseProjectile()
{
	if (world->getTick() <= game->getInitialFreezeDurationTicks())
		return false;

	double R = 750;

	bool yes = false;

	vector <Car> cars = world->getCars();
	for (int i = 0; i < cars.size(); ++i)
	{
		if (cars[i].getId() != ID && cars[i].getDurability() > 0 && !cars[i].isFinishedTrack())
		{
			if (self[ID]->getType() == BUGGY)
			{
				Vector car_pos(cars[i].getX(), cars[i].getY());
				Vector car_vel(cars[i].getSpeedX(), cars[i].getSpeedY());

				Vector start_pos(self[ID]->getX(), self[ID]->getY());

				Vector vel[3];
				vel[0] = Vector(self[ID]->getAngle() - game->getSideWasherAngle()) * game->getWasherInitialSpeed();
				vel[1] = Vector(self[ID]->getAngle()) * game->getWasherInitialSpeed();
				vel[2] = Vector(self[ID]->getAngle() + game->getSideWasherAngle()) * game->getWasherInitialSpeed();

				for (int j = 0; j < 3; ++j)
				{
					double t1 = (car_pos.x - start_pos.x) / (vel[j].x - car_vel.x);
					double t2 = (car_pos.y - start_pos.y) / (vel[j].y - car_vel.y);

					if (t1 > 0 && t2 > 0 && (cars[i].isTeammate() || max(t1, t2) <= 30 + 10 * self[ID]->getProjectileCount()) &&
						fabs(t2 - t1) <= 3 * self[ID]->getProjectileCount())
					{
						if (cars[i].isTeammate())
							return false;

						yes = true;
					}
				}
			}
			else
			{
				Vector tar = Vector(cars[i].getX() - self[ID]->getX(), cars[i].getY() - self[ID]->getY());
				if (tar.module() < R)
				{
					double Ang = norm(tar.angle() - self[ID]->getAngle());
					if (fabs(Ang) <= PI / 8 &&
						min(0.0, tar.module() - self[ID]->getHeight()) * fabs(sin(Ang)) <= 2 * game->getTireRadius())
					{
						if (cars[i].isTeammate())
							return false;

						yes = true;
					}
				}
			}
		}
	}

	return yes;
}
/*
bool UseProjectile()
{
	if (world->getTick() <= game->getInitialFreezeDurationTicks())
		return false;

	double R = 500 + 500 * self[ID]->getProjectileCount();
	if (self[ID]->getType() == JEEP)
		R = 700;

	vector <Car> cars = world->getCars();
	for (int i = 0; i < cars.size(); ++i)
	{
		if (!cars[i].isTeammate() && cars[i].getDurability() > 0 && !cars[i].isFinishedTrack())
		{
			Vector tar = Vector(cars[i].getX() - self[ID]->getX(), cars[i].getY() - self[ID]->getY());
			if (tar.module() < R)
			{
				double Ang = norm(tar.angle() - self[ID]->getAngle());
				if ((self[ID]->getType() == BUGGY) && fabs(Ang) <= 2 * game->getSideWasherAngle() * 1500 / tar.module())
					return true;
				if ((self[ID]->getType() == JEEP) && fabs(Ang) <= PI / 8 && 
					min(0.0, tar.module() - self[ID]->getHeight()) * fabs(sin(Ang)) <= 2 * game->getTireRadius())
					return true;
			}
		}
	}

	return false;
}*/