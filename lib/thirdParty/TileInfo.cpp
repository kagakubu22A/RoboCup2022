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

Wall TileManager::CheckWall(Point point, Angle ang)
{
	switch (ang)
	{
	case Angle::Left:
		break;
	case Angle::Forward:
		break;
	case Angle::Right:
		break;
	case Angle::Backward:
		break;
	default:
		break;
	}

	// TODO
	return Wall::Unknown;
}

bool TileManager::GetTileFromPosition(Point point, TileInfo &tl)
{
	for (TileInfo ti : _tilevec)
	{
		if (ti.GetPoint() == point)
		{
			tl = ti;
			return true;
		}
	}
	return false;
}

void TileManager::AddTile(TileInfo tile)
{
	_tilevec.push_back(tile);
}

bool TileManager::IsTileAchieved(Point p)
{
	for (TileInfo ti : _tilevec)
	{
		if (ti.GetPoint() == p)
			return true;
	}
	return false;
}

void TileManager::Initialize(Walls walls)
{
	_tilevec.push_back(TileInfo{Point{0, 0}, walls, vector<Obstacle>{Obstacle::Nothing}, FloorType::StartPoint});
	M5.Lcd.println();
	/*_tilevec.push_back(TileInfo(Point(0, 0), Obstacle::Nothing, vector<Wall>{Wall::WallNotExists, Wall::WallNotExists, Wall::WallExists, Wall::WallExists}));
	_tilevec.push_back(TileInfo(Point(-1, 0), Obstacle::Nothing, vector<Wall>{Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists, Wall::WallExists}));
	_tilevec.push_back(TileInfo(Point(-2, 0), Obstacle::Nothing, vector<Wall>{Wall::WallExists, Wall::WallNotExists, Wall::WallNotExists, Wall::WallExists}));

	_tilevec.push_back(TileInfo(Point(0, 1), Obstacle::Nothing, vector<Wall>{Wall::WallNotExists, Wall::WallNotExists, Wall::WallExists, Wall::WallNotExists}));
	_tilevec.push_back(TileInfo(Point(-1, 1), Obstacle::Nothing, vector<Wall>{Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists}));
	_tilevec.push_back(TileInfo(Point(-2, 1), Obstacle::Nothing, vector<Wall>{Wall::WallExists, Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists}));

	_tilevec.push_back(TileInfo(Point(0, 2), Obstacle::Nothing, vector<Wall>{Wall::WallNotExists, Wall::WallExists, Wall::WallExists, Wall::WallNotExists}));
	_tilevec.push_back(TileInfo(Point(-1, 2), Obstacle::Nothing, vector<Wall>{Wall::WallNotExists, Wall::WallExists, Wall::WallNotExists, Wall::WallNotExists}));
	_tilevec.push_back(TileInfo(Point(-2, 2), Obstacle::Nothing, vector<Wall>{Wall::WallExists, Wall::WallExists, Wall::WallNotExists, Wall::WallNotExists}));*/
}

vector<TileInfo> TileManager::GetTiles()
{
	return _tilevec;
}