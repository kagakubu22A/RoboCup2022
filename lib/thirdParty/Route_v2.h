#pragma once

#include <vector>
#include "Point.h"
#include "TileInfo.h"
using std::vector;

class Route_v2
{
private:
	vector<Point> _pvec;
	int curvenum = 0;
	Direction _dir;
	
public:
	int leftcost = 0;
	int costsum = 0;

	/// @brief 初回限定の動き
	/// @param startp
	/// @param dir
	Route_v2(Point startp, Direction dir);

	/// @brief 通常のムーブ
	/// @param p
	/// @param route
	Route_v2(Point p, vector<Point> route);

	/// <summary>
	/// 座標のリストと角度を用いた初期化
	/// </summary>
	/// <param name="route"></param>
	/// <param name="ang"></param>
	Route_v2(vector<Point> route, Direction dir);

	void AddPoint(Point p);
	vector<Point> getAllPoint() const;
	Point getLatest() const;

	int size(){return _pvec.size();}

	Point at(int i){return _pvec[i];}
};