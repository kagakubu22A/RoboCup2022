#include "Point.h"

Point::Point(int _x, int _y, int _fl)
{
	x = _x;
	y = _y;
	floor = _fl;
}

std::string Point::tostr(Point p){
	std::string res = "(";
	res += std::to_string(p.x);
	res += ", ";
	res += std::to_string(p.y);
	res += ", ";
	res += std::to_string(p.floor);
	res += ")";
	return res;
}

bool Point::operator==(const Point &p) const
{
	return x == p.x && y == p.y && floor == p.floor;
}

bool Point::operator!=(const Point &p) const { return x != p.x || y != p.y || floor != p.floor; }

PassedPoint::PassedPoint(const Point p)
{
	point = p;
}