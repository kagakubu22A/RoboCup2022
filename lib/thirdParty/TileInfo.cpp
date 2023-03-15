#include "TileInfo.h"
#include <stdexcept>
#include <M5Stack.h>

vector<TileInfo> TileManager::_tilevec;

Walls::Walls(const Wall west, const Wall north, const Wall east, const Wall south)
{
	wall[WALL_WEST] = west;
	wall[WALL_EAST] = east;
	wall[WALL_NORTH] = north;
	wall[WALL_SOUTH] = south;
}

TileInfo::TileInfo(Point p, Walls walls, vector<Obstacle> obss = vector<Obstacle>{Obstacle::Nothing}, FloorType sa = FloorType::None)
{
	_p = p;
	_obsvec = obss;
	_wls = walls;
	fp = sa;
}

Point TileInfo::GetPoint()
{
	return _p;
}

Wall TileInfo::GetWall(Direction dir)
{
	switch (dir)
	{
	case Direction::West:
		return _wls.wall[WALL_WEST];
	case Direction::North:
		return _wls.wall[WALL_NORTH];
	case Direction::East:
		return _wls.wall[WALL_EAST];
	case Direction::South:
		return _wls.wall[WALL_SOUTH];
	default:
		throw std::invalid_argument("TileInfo::GetWall");
	}
}