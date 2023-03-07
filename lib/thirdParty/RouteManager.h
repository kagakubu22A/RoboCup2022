#ifndef ROUTES_MANAGEMENT
#define ROUTES_MANAGEMENT
#include "Point.h"
#include "Route_v2.h"
#include <vector>
using std::vector;

class RouteManager
{
public:
	static void PassedPosAdd(const Point p);
	static int WhenPointReached(const Point p);

	static void Move1Step();

	static Route_v2 Pos2Pos(Point start, Point end);

	static int Obstacle2Cost(const Obstacle obs);

private:
	RouteManager();

private:
	static vector<PassedPoint> _simPassedPosList;
	static vector<Route_v2> _simRouteList;
};
#endif