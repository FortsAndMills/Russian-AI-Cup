#include "Battle.h"

void checkForMinionAppear()
{
	if (tick > 0)
	{
		// если должны появится миньоны, то создаём четыре shadow-миньона врагу на каждой линии
		for (int t = tick - WAS_DEAD; t <= tick; ++t)
		{
			if (t % game->getFactionMinionAppearanceIntervalTicks() == 0)
			{
				for (int i = 0; i < 3; ++i)
				{
					shadow_minions.push_back(new WarMinion(spawns[i].x, spawns[i].y, MINION_ORC_WOODCUTTER));
					shadow_minions.push_back(new WarMinion(spawns[i].x, spawns[i].y, MINION_ORC_WOODCUTTER));
					shadow_minions.push_back(new WarMinion(spawns[i].x, spawns[i].y, MINION_ORC_WOODCUTTER));
					shadow_minions.push_back(new WarMinion(spawns[i].x, spawns[i].y, MINION_FETISH_BLOWDART));
				}
			}
		}
	}
}

void updatePieces()  // апдейт всех данных
{
	me->clearMove();

	vector <Tree> tr = world->getTrees();
	for (int i = 0; i < (int)tr.size(); ++i)
	{
		int ID = tr[i].getId();
		if (trees.count(ID))
			trees[ID]->update(tr[i]);
		else
			trees[ID] = new WarTree(tr[i]);
	}

	vector <Wizard> w = world->getWizards();
	for (int i = 0; i < (int)w.size(); ++i)
	{
		int ID = w[i].getId();
		wizards[ID]->update(&(w[i]));
	}

	vector <Minion> m = world->getMinions();
	for (int i = 0; i < (int)m.size(); ++i)
	{
		if (m[i].getFaction() == FACTION_ACADEMY ||
			m[i].getFaction() == FACTION_RENEGADES)
		{
			int ID = m[i].getId();
			if (minions.count(ID))
			{
				minions[ID]->update(&(m[i]));
			}
			else
			{
				minions[ID] = new WarMinion(&(m[i]));
				pieces.push_back(minions[ID]);
			}
		}
	}

	vector <Building> t = world->getBuildings();
	for (int i = 0; i < (int)t.size(); ++i)
	{
		int ID = t[i].getId();

		if (towers.count(ID))
			towers[ID]->update(&(t[i]));
		else
		{
			towers[ID] = new WarTower(&(t[i]), false);
			////cout << tick << ") Strange! Tower " << ID << " is not dead!" << endl;
		}
	}
}

void dealWithDisappeared(bool friendship)  // кто из непроапдейтенных заявляет, что умер - убираем
{
	set<int> to_remove;
	for (auto P: pieces)
		if (P->isFriend == friendship && P->checkIfWasNotUpdated())
			to_remove.insert(P->id);
	for (auto id_to_kill : to_remove)
		killPiece(id_to_kill);
	
	if (friendship)
	{
		to_remove.clear();
		for (auto T : trees)
			if (T.second->checkIfWasNotUpdated())
				to_remove.insert(T.second->id);

		for (auto id_to_kill : to_remove)
			trees.erase(id_to_kill);
	}
}

void dealWithMissiles()
{
	set<int> ids;  // получаем набор id-шек всех имеющихся ракет.
	for (auto it : Rockets)
		ids.insert(it.first);

	vector<Projectile> missiles = world->getProjectiles();
	for (int i = 0; i < missiles.size(); ++i)
	{
		int ID = missiles[i].getId();
		ids.erase(ID);

		if (Rockets.count(ID))
			Rockets[ID].update(missiles[i]);
		else
			Rockets[ID] = Missile(missiles[i]);
	}

	// взорвались!
	for (int ID : ids)
		Rockets.erase(ID);
}
void dealWhoIsNearMe()
{
	for (WarPiece * p : pieces)
	{
		p->isNearMe = me->getDist2To(p) <= BATTLE_NEAR_ME * BATTLE_NEAR_ME;
	}

	bool updated = false;
	do
	{
		updated = false;
		for (WarPiece * p : pieces)
		{
			if (!p->isNearMe)
			{
				for (WarPiece * near : pieces)
				{
					if (near->isNearMe && near->getDist2To(p) <= BATTLE_NEAR_ANYONE * BATTLE_NEAR_ANYONE)
					{
						p->isNearMe = true;
						updated = true;
					}
				}
			}
		}
	} while (updated);
}

void getNews()
{
	checkForMinionAppear();

	updatePieces();
	dealWithDisappeared(true);  // сначала своих, чтобы исчезнувшие умерли и не портили области видимости.
	dealWithDisappeared(false);

	for (auto W : wizards)
		W.second->nullSkills();
	for (auto W : wizards)
		W.second->processSkills();

	dealWhoIsNearMe();

	dealWithMissiles();

	for (auto P : pieces)
		P->estimated_death_time = NEVER;

	for (auto W : wizards)
		W.second->updateSitHome();
	for (auto P : pieces)
		P->updateGoingIntoBattle();
	for (auto P : pieces)
		P->updateShadows();
}