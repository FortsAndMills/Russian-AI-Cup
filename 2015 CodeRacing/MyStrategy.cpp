#include "MyStrategy.h"
#include "Physics.h"
#include "FieldProcess.h"
#include "Brain.h"
#include "UseBonuses.h"

bool start = true;
void MyStrategy::move(const Car& _self, const World& _world, const Game& _game, Move& move)
{
	if (_self.isFinishedTrack())
		return;

	ID = _self.getId();
	world = &_world;
	game = &_game;

	int oldx = world->getTick() == 0 ? -1 : ((int)(self[ID]->getX() / game->getTrackTileSize()));
	int oldy = world->getTick() == 0 ? -1 : ((int)(self[ID]->getY() / game->getTrackTileSize()));
	delete self[ID];
	self[ID] = new Car(_self);
	
	if (start)
	{
		start = false;

		for (int i = 0; i < world->getCars().size(); ++i)
			if (world->getCars()[i].isTeammate())
				chosen_variant[world->getCars()[i].getId()] = -1;

		//cout << "FIELD KEY POINTS:" << endl;
		for (int i = 0; i < world->getWaypoints().size(); ++i)
		{
			//cout << world->getWaypoints()[i][0] << " " << world->getWaypoints()[i][1] << endl;
		}

		//cout << "INITIATING!" << endl;
		keys = world->getWaypoints();
		for (int x = 0; x < world->getWidth(); ++x)
		{
			field.push_back(vector <Tile>());
			bonusesOnTile.push_back(vector <vector <pair <bool, int> > >());
			
			for (int y = 0; y < world->getHeight(); ++y)
			{
				field[x].push_back(Tile(x, y));
				bonusesOnTile[x].push_back(vector < pair <bool, int> >());
			}
		}
		
		StartFieldEstimations();
	}

	if (true)
	{
		Answer ans = SearchWays(oldx, oldy);

		//if (world->getTick() >= 200 && world->getTick() <= 300)
			//ans = Answer(1, 1, 0);

		move.setBrake(ans.brake);
		move.setWheelTurn(ans.wheels());
		move.setEnginePower(ans.Engine);

		move.setUseNitro(UseNitro());
		move.setSpillOil(UseOil());
		move.setThrowProjectile(UseProjectile());
	}
	else
	{
		if (world->getTick() < 400)
		{
			move.setEnginePower(1);
		}
		else
		{
			PhysMe P;
			//cout << "MAIN---- " << P.tick << ": (" << P.x << " " << P.y << "), speed = (" << P.speed.x << ", " << P.speed.y << "); " << P.ang << ", " << P.spAng << "; " << P.engP << " and " << P.wheT << endl;

			if (world->getTick() == 400)
			{
				PhysMe P(Answer(1, 1, false));
				while (P.tick != world->getTick() + 40)
				{
					P.NextTick();
					//cout << "   " << P.tick << ": (" << P.x << " " << P.y << "), speed = (" << P.speed.x << ", " << P.speed.y << "); " << P.ang << ", " << P.spAng << "; " << P.engP << " and " << P.wheT << endl;
				}
			}

			move.setEnginePower(1);
			move.setWheelTurn(1);
		}
	}
}

MyStrategy::MyStrategy() { }
