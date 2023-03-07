#include "Route_v2.h"
#include "RouteManager.h"
#include "MachineManager.h"

Route_v2::Route_v2(Point startp, Direction dir)
{
	_pvec.push_back(startp);
	_dir = dir;
	AddPoint(MachineManager::_dir2p(startp, dir));
}

Route_v2::Route_v2(Point p, vector<Point> route)
{
	_pvec = route;
	Point lastp(route[route.size() - 1]);
	//TODO ここから下まだ完成してない
}

Route_v2::Route_v2(vector<Point>route, Direction dir) {
	_pvec = route;
	Point lastp(route[route.size() - 1]);
	AddPoint(MachineManager::_dir2p(lastp, dir));

}

void Route_v2::AddPoint(Point p)
{
	if (_pvec.size() != 0)
	{
		Point lastp = _pvec.back();

		//左
		if (lastp.x - p.x == 1)
		{
			_dir = Direction::West;
		}
		//上
		else if (lastp.y - p.y == -1)
		{
			_dir = Direction::North;
		}
		//右
		else if (lastp.x - p.x == -1)
		{
			_dir = Direction::East;
		}
		//下
		else if (lastp.y - p.y == 1)
		{
			_dir = Direction::South;
		}
		else {
			throw std::invalid_argument("Invalid angle!!!");
		}
		//座標を追加
		_pvec.push_back(p);
		RouteManager::PassedPosAdd(p);
	}
}

vector<Point> Route_v2::getAllPoint() const
{
	return _pvec;
}

Point Route_v2::getLatest() const
{
	return _pvec[_pvec.size() - 1];
}