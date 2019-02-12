#include "MyStrategy.h"
#include "Rush.h"

bool Rounds()
{
	int jocktime = 0;
	if (W.getMyPlayer().isJustMissedGoal())
		jocktime = -1;
	else if (W.getMyPlayer().isJustScoredGoal())
		jocktime = 1;

	bool subs = W.getHockeyists().size() >= 12;
	if (jocktime < 0 || (jocktime > 0 && subs))
	{
		for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
		{
			if (hcit->second.isTeammate() && hcit->second.getState() != RESTING)
			{
				variants.clear();

				if (subs)
					ExchangeHockeyists(hcit->second, INF);

				for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
					if (!IT->second.isTeammate() && IT->second.getState() != RESTING)
						variants.push_back(Variant(hcit->second, IT->second.getX(), IT->second.getY(), "striker", false));
				
				ChooseBestVariant(hcit->second);

				for (map <long long int, MyHockeyist>::iterator IT = hc.begin(); IT != hc.end(); ++IT)
				{
					if (!IT->second.isTeammate() && IT->second.getState() != RESTING && 
						CanStrikeHockeyist(hcit->second, IT->second))
					{
						hcit->second.answer.action = STRIKE;
					}
				}

				if (hcit->second.state == "substitute" && CanBeSubstituted(hcit->second))
				{
					Substitute(hcit->second);
				}
			}
		}
		return true;
	}
	else if (jocktime > 0)
	{
		for (map <long long int, MyHockeyist>::iterator hcit = hc.begin(); hcit != hc.end(); ++hcit)
		{
			if (hcit->second.isTeammate() && hcit->second.getState() != RESTING)
			{
				hcit->second.answer.action = STRIKE;
				hcit->second.answer.speed_up = 0.0;
				hcit->second.answer.turn = -PI;
			}
		}
		return true;
	}
	return false;
}
void NewTick()
{
	cerr << endl;
	cerr << endl;
	cerr << "TICK №" << tick << ";    -----------------------------------------------------------------" << endl;
	//cerr << puck.getX() << " " << puck.getY() << "     " << puck.getSpeedX() << " " << puck.getSpeedY() << endl;

	if (Rounds())
		return;

	if (W.getPuck().getOwnerPlayerId() == -1)
	{
		state = "RUSH";
		cerr << "STATE: " << state << endl;
		Rush();
	}
	else if (W.getPuck().getOwnerPlayerId() == W.getMyPlayer().getId())
	{
		state = "LEAD";
		cerr << "STATE: " << state << endl;
		Lead();
	}
	else
	{
		state = "CHASE";
		cerr << "STATE: " << state << endl;
		Chase();
	}
}

void MyStrategy::move(const Hockeyist& self, const World& world, const Game& game, Move& move) 
{
	W = world;
	G = game;
	if (tick == -1)
	{
		Starter();
	}
	if (W.getTick() != tick)
	{
		Update();

		NewTick();  // Задачу решаем сразу для всех хоккеистов раз в ход.
	}

	cerr << self.getId() << ", " << self.getTeammateIndex() << ",      " << hc[self.getId()].answer.action << endl;
	move.setSpeedUp(hc[self.getId()].answer.speed_up);  //Ответ для данного хоккеиста.
	move.setTurn(hc[self.getId()].answer.turn);
	move.setAction(hc[self.getId()].answer.action);
	move.setPassAngle(hc[self.getId()].answer.PassAngle);
	move.setPassPower(hc[self.getId()].answer.PassPower);
	move.setTeammateIndex(hc[self.getId()].answer.teammateindex);
}

MyStrategy::MyStrategy() { }
