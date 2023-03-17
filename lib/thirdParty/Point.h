#ifndef POINT
#define POINT

#include <string>

class Point
{
private:
public:
	int x, y;
	int floor = 1;
	Point(int _x, int _y, int _fl = 1);

	static std::string tostr(Point p);

	bool operator==(const Point &p) const;
	bool operator!=(const Point &p) const;
};

class PassedPoint
{
public:
	Point point{0, 0, 0};
	int period = 0;

	PassedPoint(const Point p);
};

#endif