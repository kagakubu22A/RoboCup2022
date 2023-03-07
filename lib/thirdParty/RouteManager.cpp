#include "RouteManager.h"
#include "Point.h"
#include <stdexcept>
#include <iostream>
#include <M5Stack.h>
#include "MachineManager.h"
#include "MappingManager.h"

vector<PassedPoint> RouteManager::_simPassedPosList;
vector<Route_v2> RouteManager::_simRouteList;

void RouteManager::PassedPosAdd(const Point p)
{
	if (WhenPointReached(p) < 0)
	{
		PassedPoint p2{p};
		_simPassedPosList.push_back(p2);
	}
}

int RouteManager::WhenPointReached(const Point p)
{

	for (PassedPoint in : _simPassedPosList)
	{
		if (in.point == p)
			return in.period;
	}
	return -1;
}

void RouteManager::Move1Step()
{
	vector<Route_v2> tmpv(_simRouteList);
	vector<Route_v2> newroutes;

	for (int i = 0; i < tmpv.size(); i++)
	{
		// 合計コスト1追加
		// TODO ちゃんと動くかね?
		tmpv[i].costsum++;

		// コストがなければ先に行く
		if (tmpv[i].leftcost > 0)
		{
			tmpv[i].leftcost--;
			continue;
		}
		TileInfo tl;

		//
		if (!TileManager::GetTileFromPosition(tmpv[i].getLatest(), tl))
		{
			throw std::runtime_error("Unknown Error occured in RouteManager::Move1Step()");
		}

		// 最大4方向に伸ばせるので4回繰り返す
		for (int k = 0; k < 4; k++)
		{
			// TODO Angle を Direction に変え、pointrotateを_dir2pに変えたが大丈夫か
			if (tl.GetWall((Direction)k) == Wall::WallNotExists && WhenPointReached(MachineManager::_dir2p(tmpv[i].getLatest(), (Direction)k)) < 0)
			{
				Route_v2 r(tmpv[i].getAllPoint(), (Direction)k);
				r.costsum = tmpv[i].costsum;
				newroutes.push_back(r);
				//_passedPosList.push_back(r.getLatest());
				PassedPosAdd(r.getLatest());
			}
		}
	}

	for (int i = 0; i < _simPassedPosList.size(); i++)
	{
		_simPassedPosList[i].period++;
	}

	// 候補ルートのリストを更新
	_simRouteList = newroutes;
	for (int i = 0; i < _simRouteList.size(); i++)
	{
		for (int j = 0; j < _simRouteList[i].size(); j++)
		{
			Serial.println(MachineManager::p2str(_simRouteList[i].at(j)).c_str());
		}
		Serial.println();
	}
}

Route_v2 RouteManager::Pos2Pos(Point start, Point end)
{
	Serial.printf("Goes to %s, from %s\n", MachineManager::p2str(end).c_str(), MachineManager::p2str(start).c_str());
	TileInfo tl;

	if (!MappingManager::GetTileFromPosition(start, tl))
	{
		Serial.println("Error: start point is invalid!\n in RouteManager::Pos2Pos(Point start, Point end)");
		throw std::invalid_argument("Error: start point is invalid in RouteManager::Pos2Pos(Point start, Point end)");
	}
	_simPassedPosList.push_back(PassedPoint(start));
	_simPassedPosList[0].period++;

	// 最初は壁のないところに一回伸ばす
	for (int i = 0; i < 4; i++)
	{
		if (tl.GetWall((Direction)i) == Wall::WallNotExists)
		{
			Route_v2 r(start, (Direction)i);
			_simRouteList.push_back(r);
		}
	}

	for (int i = 0; i < _simPassedPosList.size(); i++)
	{
		_simPassedPosList[i].period++;
	}

	while (RouteManager::WhenPointReached(end) == -1)
	{
		Move1Step();
	}

	for (int i = 0; i < _simRouteList.size(); i++)
	{
		if (_simRouteList[i].getLatest() == end)
		{
			Route_v2 r(_simRouteList[i]);
			_simRouteList.clear();
			_simPassedPosList.clear();
#if DEBUGGING_MODE
			Serial.println("Found route is below.");
			for (int k = 0; k < r.size(); k++)
			{
				Serial.println(MachineManager::p2str(r.at(k)).c_str());
			}
#endif
			return r;
		}
	}

	Serial.println("No route found");
	throw std::runtime_error("No route found");
}

int RouteManager::Obstacle2Cost(const Obstacle obs)
{
	switch (obs)
	{
	case Obstacle::Slope:
		return 100;
	default:
		M5.Lcd.println("indeclared obstacle!! : Are all costs of obstacles set?");
		throw std::invalid_argument("indeclared obstacle!! : All costs of obstacles are set?");
	}
}
