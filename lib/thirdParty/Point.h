#ifndef POINT
#define POINT

class Point
{
private:
public:
	int x, y;
	Point(int _x, int _y);
	//Point() {};

	bool operator==(const Point p) const;
	bool operator!=(const Point p) const;
};

class PassedPoint
{
public:
	Point point{ 0,0 };
	int period = 0;

	PassedPoint(const Point p);
};

#endif