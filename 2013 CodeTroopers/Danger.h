#include "Going.h"

bool IsSeen(int MyPosX, int MyPosY, int x, int y, const World& world)
{
	int i = 0;
	while (i < (int)troopers.size() && 
		((troopers[i].getId() != self.getId() && !world.isVisible(troopers[i].getVisionRange(), troopers[i].getX(), troopers[i].getY(), troopers[i].getStance(), x, y, STANDING)) ||
		(troopers[i].getId() == self.getId() && !world.isVisible(troopers[i].getVisionRange(), MyPosX, MyPosY, troopers[i].getStance(), x, y, STANDING))))
	{
		++i;
	}

	return i < (int)troopers.size();
}
bool IsFieldDangerous(int MyPosX, int MyPosY, int x, int y, const World& world)
{
	int i = 0;

	if (!IsSeen(MyPosX, MyPosY, x, y, world))
	{
		i = 0;
		while (i < (int)troopers.size() && 
			((troopers[i].getId() != self.getId() && !world.isVisible(DANGER_LIMITS, x, y, STANDING, troopers[i].getX(), troopers[i].getY(), troopers[i].getStance())) ||
			(troopers[i].getId() == self.getId() && !world.isVisible(DANGER_LIMITS, x, y, STANDING, MyPosX, MyPosY, troopers[i].getStance()))))
		{
			++i;
		}

		return i < (int)troopers.size();
	}
	
	return false;
}
bool IsFieldSeen(int MyPosX, int MyPosY, int x, int y, const World& world)
{
	int i = 0;
	while (i < (int)troopers.size() && 
		((troopers[i].getId() != self.getId() && !world.isVisible(troopers[i].getVisionRange(), troopers[i].getX(), troopers[i].getY(), troopers[i].getStance(), x, y, PRONE)) ||
		(troopers[i].getId() == self.getId() && !world.isVisible(troopers[i].getVisionRange(), MyPosX, MyPosY, troopers[i].getStance(), x, y, PRONE))))
	{
		++i;
	}

	return i != troopers.size();
}

set< pair<int, int> > CuriousFields;
void SuddenAttack(int X, int Y, const World& world)
{
	ds_update_time = my_time;
	int min_x = max(0, X - DANGER_LIMITS);
	int max_x = min(world.getWidth(), X + DANGER_LIMITS);
	int min_y = max(0, Y - DANGER_LIMITS);
	int max_y = min(world.getHeight(), Y + DANGER_LIMITS);

	for (int x = min_x; x < max_x; ++x)
	{
		for (int y = min_y; y < max_y; ++y)
		{
			if (DoExistWT(x, y, world) && IsFieldDangerous(self.getX(), self.getY(), x, y, world))
			{
				CuriousFields.insert(make_pair(x, y));
			}
		}
	}
}
const int RUN_LIMIT = 2;
void SuddenRun(int X, int Y, const World& world)
{
	ds_update_time = my_time;
	int min_x = max(0, X - RUN_LIMIT);
	int max_x = min(world.getWidth(), X + RUN_LIMIT);
	int min_y = max(0, Y - RUN_LIMIT);
	int max_y = min(world.getHeight(), Y + RUN_LIMIT);

	for (int x = min_x; x <= max_x; ++x)
	{
		for (int y = min_y; y <= max_y; ++y)
		{
			if (DoExistWT(x, y, world) &&
				abs(X - x) + abs(Y - y) <= RUN_LIMIT &&
				!IsFieldSeen(self.getX(), self.getY(), x, y, world))
			{
				CuriousFields.insert(make_pair(x, y));
			}
		}
	}
}
const int DISSAPEAR_LIMIT = 3;
void SuddenDissapear(int X, int Y, const World& world)
{
	ds_update_time = my_time;
	int min_x = max(0, X - DISSAPEAR_LIMIT);
	int max_x = min(world.getWidth(), X + DISSAPEAR_LIMIT);
	int min_y = max(0, Y - DISSAPEAR_LIMIT);
	int max_y = min(world.getHeight(), Y + DISSAPEAR_LIMIT);

	for (int x = min_x; x <= max_x; ++x)
	{
		for (int y = min_y; y <= max_y; ++y)
		{
			if (DoExistWT(x, y, world) &&
				abs(X - x) + abs(Y - y) <= DISSAPEAR_LIMIT &&
				!IsFieldSeen(self.getX(), self.getY(), x, y, world))
			{
				CuriousFields.insert(make_pair(x, y));
			}
		}
	}
}

const int DS_TIME = 5;
int DangerCheckInCurrentPos(int MyPosX, int MyPosY, const World& world)
{
	int min_x = MyPosX, max_x = MyPosX, min_y = MyPosY, max_y = MyPosY;  // Сначала находим границы, по которым будем искать опасности
	for (int i = 0; i < (int)troopers.size(); ++i)
	{
		if (troopers[i].getX() < min_x)
			min_x = troopers[i].getX();
		if (troopers[i].getX() > min_x)
			max_x = troopers[i].getX();
		if (troopers[i].getY() < min_y)
			min_y = troopers[i].getY();
		if (troopers[i].getY() > min_x)
			max_y = troopers[i].getY();
	}

	min_x = max(0, min_x - DANGER_LIMITS);
	max_x = min(world.getWidth(), max_x + DANGER_LIMITS);
	min_y = max(0, min_y - DANGER_LIMITS);
	max_y = min(world.getHeight(), max_y + DANGER_LIMITS);

	int count = 0;
	for (int x = min_x; x < max_x; ++x)
	{
		for (int y = min_y; y < max_y; ++y)
		{
			if (IsFieldDangerous(MyPosX, MyPosY, x, y, world))
			{
				++count;
			}
		}
	}
	return count;
}
int DangerSearchInCurrentPos(int MyPosX, int MyPosY, const World& world)
{
	int count = 0;
	for (set< pair<int, int> >::iterator it = CuriousFields.begin(); it != CuriousFields.end(); ++it)
	{
		if (IsFieldSeen(MyPosX, MyPosY, it->first, it->second, world))
		{
			++count;
		}
	}
	return count;
}
void DangerSearchUpdate(const World& world)
{
	if (times[my_time] - times[ds_update_time] > DS_TIME)
		CuriousFields.clear();

    set< pair<int, int> >::iterator it = CuriousFields.begin();
	while (it != CuriousFields.end())
	{
		if (IsFieldSeen(self.getX(), self.getY(), it->first, it->second, world))
		{
            ++it;
            set< pair<int, int> >::iterator _it = it;
            --it;
			CuriousFields.erase(*it);
            it = _it;
		}
        else
            ++it;
	}
}

void DangerSearch(const World& world)
{	
	for (int i = 0; i < (int)price.size(); ++i)
	{
		double Price = pow((double)DangerSearchInCurrentPos(new_x[i], new_y[i] , world), 3) * 200;
		
		if (Price != 0)
		{
			//cerr //<< "DangerSearch input: want to go to (" //<< new_x[i] //<< ", " //<< new_y[i] 
			//<< ") with price " //<<  Price //<< endl;
		}
        
		if (CommanderBonus(new_x[i], new_y[i], world) || abs(Price) >= 1000)
		{
			if (self.getActionPoints() - MoveCost(self) * SEARCH_SAFETY_LIMITS >= self.getShootCost())
			{
				price[i] += Price;
			}
			else
			{
				price[i] -= Price;
			}
		}
        else if (Price != 0)
		{
            //cerr //<< "This DS is banned because of CB!" //<< endl;
		}
	}
}
void DangerCheck(const World& world)
{
	int m = HowManyMovesCanDo();

	int cur_danger = DangerCheckInCurrentPos(self.getX(), self.getY(), world);

	for (int i = 0; i < (int)new_x.size(); ++i)
	{
		double k = cur_danger - DangerCheckInCurrentPos(new_x[i], new_y[i], world);
		double Price = pow(k, 3);
		
		if (k != 0)
		{
			if ((m <= SEARCH_SAFETY_LIMITS && CommanderBonus(new_x[i], new_y[i], world))
				|| abs(Price) >= 1000)
			{
				//cerr //<< "Danger input: go to (" //<< new_x[i] //<< ", " //<< new_y[i] 
				//<< ") gets " //<<  Price //<< endl;
				price[i] += Price;
			}
		}
	}
}

