#pragma once

#include "Tile.h"

const int PHYS_ITERATIONS = 10;

class PhysMe
{
public:
	int tick;

	double engP;
	double wheT;

	Vector speed;
	double spAng;

	double x, y, ang;

	int nitro_ticks;

	PhysMe()
	{
		restart();
	}
	PhysMe(Answer _ans)
	{
		ans = _ans;
		restart();
	}
	void restart()
	{
		tick = world->getTick();

		engP = self[ID]->getEnginePower();
		wheT = self[ID]->getWheelTurn();

		speed = Vector(self[ID]->getSpeedX(), self[ID]->getSpeedY());
		spAng = self[ID]->getAngularSpeed();

		x = self[ID]->getX();
		y = self[ID]->getY();
		ang = self[ID]->getAngle();

		nitro_ticks = self[ID]->getRemainingNitroTicks();

		RecountPoints();
	}
	
	vector <Vector> points;
	void RecountPoints()
	{
		points.clear();

		Vector center(x, y);

		double L = (Vector(self[ID]->getWidth(), self[ID]->getHeight()) / 2).module();
		double a = acos(self[ID]->getWidth() / 2 / L);

		points.push_back(Vector(a + ang) * L + center);
		points.push_back(Vector(-a + ang) * L + center);
		points.push_back((points[0] + points[1]) / 2);
		points.push_back(Vector(PI + a + ang) * L + center);
		points.push_back(Vector(PI - a + ang) * L + center);
	}
	
	Answer ans;
	void NextTick()
	{
		tick++;

		// изменяем установки
		double delta = fabs(engP - ans.Engine);
		if (delta <= game->getCarEnginePowerChangePerTick())
			engP = ans.Engine;
		else if (engP > ans.Engine)
			engP -= game->getCarEnginePowerChangePerTick();
		else
			engP += game->getCarEnginePowerChangePerTick();

		delta = fabs(wheT - ans.wheels());
		if (delta <= game->getCarWheelTurnChangePerTick())
			wheT = ans.wheels();
		else if (wheT > ans.wheels())
			wheT -= game->getCarWheelTurnChangePerTick();
		else
			wheT += game->getCarWheelTurnChangePerTick();

		// константы
		double uF = 1.0 / PHYS_ITERATIONS;
		double CarPower = game->getBuggyEngineForwardPower();
		if (self[ID]->getType() == JEEP)
			CarPower = game->getJeepEngineForwardPower();
		if (engP < 0 && self[ID]->getType() == BUGGY)
			CarPower = game->getBuggyEngineRearPower();
		else if (engP < 0)
			CarPower = game->getJeepEngineRearPower();
		double m = self[ID]->getMass();

		double medSpAng = wheT * game->getCarAngularSpeedFactor() * (speed ^ Vector(ang));
		spAng += medSpAng;

		for (int p = 0; p < PHYS_ITERATIONS; ++p)
		{
			// изменяем скорость
			x += speed.x * uF;
			y += speed.y * uF;

			// применяем силу двигателя
			if (!ans.brake)
			{
				double EngPower = engP;
				if (nitro_ticks > 0)
					EngPower = game->getNitroEnginePowerFactor();

				speed += Vector(ang) * EngPower * CarPower / m * uF;
			}

			// мешается воздух
			//speed -= Vector(speed.angle()) * game->getCarMovementAirFrictionFactor() * speed.module() * uF;
			speed *= pow(1 - game->getCarMovementAirFrictionFactor(), uF);

			// мешается трение
			double lengthwise = (speed ^ Vector(ang));
			double crosswise = (speed ^ Vector(ang + PI / 2));

			double LFactor = game->getCarLengthwiseMovementFrictionFactor();
			if (ans.brake)
			{
				LFactor = game->getCarCrosswiseMovementFrictionFactor();
			}
			if (lengthwise >= 0)
			{
				lengthwise -= LFactor * uF;
				if (lengthwise < 0)
					lengthwise = 0;
			}
			else
			{
				lengthwise += LFactor * uF;
				if (lengthwise > 0)
					lengthwise = 0;
			}
			
			if (crosswise >= 0)
			{
				crosswise -= game->getCarCrosswiseMovementFrictionFactor() * uF;
				if (crosswise < 0)
					crosswise = 0;
			}
			else
			{
				crosswise += game->getCarCrosswiseMovementFrictionFactor() * uF;
				if (crosswise > 0)
					crosswise = 0;
			}

			speed = Vector(ang) * lengthwise + Vector(PI / 2 + ang) * crosswise;

			// угловое ускорение
			ang += medSpAng * uF;
			normalize(ang);

			// поворот колёс			
			spAng -= medSpAng;

			// трение поворота о воздух
			spAng *= pow(1 - game->getCarRotationAirFrictionFactor(), uF);

			// трение поворота
			if (spAng > 0)
			{
				spAng -= game->getCarRotationFrictionFactor() * uF;
				if (spAng < 0)
					spAng = 0;
			}
			else
			{
				spAng += game->getCarRotationFrictionFactor() * uF;
				if (spAng > 0)
					spAng = 0;
			}

			spAng += medSpAng;
		}

		if (nitro_ticks > 0)
			--nitro_ticks;

		RecountPoints();
	}
};



