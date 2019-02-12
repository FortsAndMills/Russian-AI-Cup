#include "WorldAroundUs.h"

const Point POINT_NOT_FOUND(-1, -1);
const Point NO_POINT_NEEDED(179, 179);
const int NOT_FOUND = -1;

// ���� ������ ����������� �� ������, ������� ��������� P
int obscure(Point P, const vector<Obstacle> & obstacles)
{
	int obstacles_size = obstacles.size();
	for (int i = 0; i < obstacles_size; ++i)
		if (obstacles[i].isIn(P))
			return i;
	return NOT_FOUND;
}

// �������� �� ��, ��� ����� �����������.
bool isSpaceWrong(Point P)
{
	return P.x <= self->getRadius() || P.x >= 4000 - self->getRadius() ||
		P.y <= self->getRadius() || P.y >= 4000 - self->getRadius();
}
template <LaneType Lane>
bool isSpaceLane(Point P)
{
	AREA a = getArea(P.x, P.y);
	return isSpaceWrong(P) || (a != MY_HOME && a != ENEMY_HOME && getLane(P.x, P.y) != Lane);
}
bool (*isSpaceLaneFunc[3])(Point) = { isSpaceLane<LANE_TOP>, isSpaceLane<LANE_MIDDLE>, isSpaceLane<LANE_BOTTOM> };

Point getSegmentBreak(Segment segment, int i, vector<Obstacle> & obstacles, double sign, set<int> & problems, bool is_space_wrong(Point) = isSpaceWrong)
{
	Point P;
	int side = 0;

	sign *= 1.1;
	
	do
	{
		// ���� �� ������� ��� ����������� �� � ��� �������, ������ ��� � ������� �� ��� ��� ��������, �� ���-�� �� ���.
		// ����� ������ ��� ����� � ������������ ���� ���������
		// ������� ������� ��������.
		if ((obstacles[i].allowed_sign > 0 && sign < 0) ||
			(obstacles[i].allowed_sign < 0 && sign > 0))
		{
			problems.clear();
			return POINT_NOT_FOUND;
		}

		// ���� �� ��������� � �����, � ������� ��� ����, ������ �� �����������
		// ������ ��� ���������� � ������� ���������, ����� ���� �� ������ - �����������.
		// ��� ��������, ��� � "��� �������" ���������� ������� �� ���������.
		if (problems.count(i) != 0)
		{
			//debug.fillCircle(obstacles[i].x, obstacles[i].y, obstacles[i].R, 0x000000);
			problems.clear();
			return POINT_NOT_FOUND;
		}
		problems.insert(i);

		//debug.fillCircle(obstacles[i].x, obstacles[i].y, obstacles[i].R, 0xFF0000);

		if (obstacles[i].isIn(segment.A) || obstacles[i].isIn(segment.B))
		{
			problems.clear();
			return POINT_NOT_FOUND;
		}

		Line a(segment.A, obstacles[i], sign), b(segment.B, obstacles[i], -sign);
		if (a.parallel(b))
		{
			// ���� �� �������, ���� ����������� - �����������.
			// ����� �� ����� ���� ��� �� �����������!
			problems.erase(i);

			// ���� ��� �������� �����������, �� ����� ���� ��� ������� ���������������.
			if (problems.size() == 0)
				return NO_POINT_NEEDED;

			// ����� �� �� ����� ����� ��� ������ � ��������� ��� ����.
			// � ��������, ���� ������ ��������, ��� �� ��������� �������� ���� ����������� ���� �� � ��� �������.
			problems.clear();
			return POINT_NOT_FOUND;
		}
		else
		{
			P = a.intersects(b);
			//debug.fillCircle(P.x, P.y, 5, 0x79FF00);

			// ���� �� ��������� �� ������ ������� �������, ������, �� �����������.
			// ���� ������������ ���� ��������� => ��������.
			int new_side = segment.side(P);
			if (side == 0) side = new_side;
			if (side != new_side)
			{
				problems.clear();
				return POINT_NOT_FOUND;
			}

			if (obstacles[i].isIn(P))
			{
				problems.clear();
				return POINT_NOT_FOUND;
			}

			// ���� ����, ������� ��������� ������ �����.
			// ���� ����� �������, ������� ����� ������ ��� ���� � � ��� �� sign
			i = obscure(P, obstacles);

			if (is_space_wrong(P))
			{
				problems.clear();
				return POINT_NOT_FOUND;
			}
		}
	} while (i != -1);

	return P;
}

// TODO ������� �������� ����������� ��� ������ ���, ����� ������ ������� ��� ����

void optimize(vector<Segment> & segment, const vector<Obstacle> & obstacles)
{
	for (int i = 0; i < (int)segment.size() - 1; ++i)
	{
		// ������� ���������� ��� ������� � ����
		Segment TestSegment(segment[i].A, segment[i + 1].B);

		// ����, �� ������������ �� � ���-������
		int j = 0;
		while (j < (int)obstacles.size() && !TestSegment.intersects(obstacles[j]))
			++j;

		// ���� ���, ���������� � ���� �������.
		if (j == (int)obstacles.size())
		{
			segment[i + 1] = TestSegment;
			segment.erase(segment.begin() + i, segment.begin() + i + 1);
			--i;
		}
	}
}
vector<Segment> BuildPath(Segment segment, vector<Obstacle> & obstacles, int Max_depth, int depth = 0, bool is_space_wrong(Point) = isSpaceWrong)
{
	if (depth > Max_depth)
	{
		return vector<Segment>();
	}

	// ������ ���� �����������, ������� ������ ����������� �� ����.
	vector<int> indexes;
	vector<double> far;
	int obstacles_size = obstacles.size();
	for (int ob = 0; ob < obstacles_size; ++ob)
	{
		if (segment.intersects(obstacles[ob]))
		{
			indexes.push_back(ob);
			far.push_back(segment.how_far(obstacles[ob]));

			int j = far.size() - 1;
			while (j > 0 && far[j] < far[j - 1])
			{
				swap(far[j], far[j - 1]);
				swap(indexes[j], indexes[j - 1]);
				--j;
			}
		}
	}

	for (int k = 0; k < (int)indexes.size(); ++k)
	{
		int i = indexes[k];
		if (far[k] > segment.len())  // ������ �� "����������� ����������". ����� ������ � ������������ ������� ��-�� �������.
			break;  // �� ���� ������ ��������, ��� ����������� ������ ���, � ���� ��������.

		set<int> problems1;
		Point P1 = getSegmentBreak(segment, i, obstacles, +1, problems1, is_space_wrong);
		if (P1 == NO_POINT_NEEDED)  // ���� ����������� - �����������, ��� ��� �� ���������� - ��������� � ����������.
			continue;

		set<int> problems2;
		Point P2 = getSegmentBreak(segment, i, obstacles, -1, problems2, is_space_wrong);
		if (P2 == NO_POINT_NEEDED)
			continue;

		if (P1 != POINT_NOT_FOUND)
		{
			//debug.line(segment.A.x, segment.A.y, P1.x, P1.y);
			//debug.line(P1.x, P1.y, segment.B.x, segment.B.y);
		}
		if (P2 != POINT_NOT_FOUND)
		{
			//debug.line(segment.A.x, segment.A.y, P2.x, P2.y);
			//debug.line(P2.x, P2.y, segment.B.x, segment.B.y);
		}

		vector<Segment> first_ans, second_ans;
		if (P1 != POINT_NOT_FOUND)  // ��������� ������� � ������ �������
		{
			for (int only1 : problems1)  // �������� ��������� �������.
				obstacles[only1].allowed_sign += 1;

			first_ans = BuildPath(Segment(segment.A, P1), obstacles, Max_depth, depth + 1, is_space_wrong);
			if (!first_ans.empty())  // ������ ����� �������� �������.
			{
				vector<Segment> second = BuildPath(Segment(P1, segment.B), obstacles, Max_depth, depth + 1, is_space_wrong);
				if (!second.empty())
				{
					first_ans.insert(first_ans.end(), second.begin(), second.end());  // ���������� � ����.
				}
				else
				{
					first_ans.clear();  // �������
				}
			}

			for (int only1 : problems1)  // ������� ������� � ��������� �������
				obstacles[only1].allowed_sign -= 1;
		}

		if (P2 != POINT_NOT_FOUND)  // ����������
		{
			for (int only_1 : problems2)
				obstacles[only_1].allowed_sign -= 1;

			second_ans = BuildPath(Segment(segment.A, P2), obstacles, Max_depth, depth + 1, is_space_wrong);
			if (!second_ans.empty())
			{
				vector<Segment> second = BuildPath(Segment(P2, segment.B), obstacles, Max_depth, depth + 1, is_space_wrong);
				if (!second.empty())
				{
					second_ans.insert(second_ans.end(), second.begin(), second.end());
				}
				else
				{
					second_ans.clear();
				}
			}

			for (int only_1 : problems2)
				obstacles[only_1].allowed_sign += 1;
		}


		double len1 = len(first_ans);  // ���������� �����. ����� ���� ��������, ��� ������ � �������� ���.
		double len2 = len(second_ans);

		if ((len2 == 0 || len1 < len2) && len1 != 0)
		{
			for (int only1 : problems1)
				obstacles[only1].allowed_sign += 1;
			optimize(first_ans, obstacles);
			return first_ans;
		}
		else if (len2 != 0)
		{
			for (int only_1 : problems2)
				obstacles[only_1].allowed_sign -= 1;
			optimize(second_ans, obstacles);
			return second_ans;
		}

		return vector<Segment>();
	}

	vector<Segment> ans;
	ans.push_back(segment);
	return ans;
}
