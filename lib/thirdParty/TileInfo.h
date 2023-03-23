#ifndef TILESINFO
#define TILESINFO
#include <vector>
#include "Point.h"
#include "AngDir.h"
using std::vector;

#define WALL_WEST 0
#define WALL_NORTH 1
#define WALL_EAST 2
#define WALL_SOUTH 3

enum class FloorType : char
{
	None,		// 普通の床
	StartPoint, // スタート地点
	CheckPoint, // チェックポイント
	KeepOut,	// 立ち入り禁止(黒)
};

enum class VictimType : char
{
	None,			// 被災者なし
	NeedRescue,		// 被災者あり(助け必要)
	NotNeedRescue,	// 被災者あり(助け必要なし)
	AlreadyRescued, // 救出済み
};

enum class Obstacle : char
{
	Nothing,	 // 何もない
	Slope,		 // 坂
	Bump,		 // バンプ
	Step,		 // 階段
	SmtgHarmful, // 何かしら妨害するもの
};

enum class Wall : char
{
	WallExists,
	WallNotExists,
	Unknown,
};

class Walls
{
private:
public:
	Wall wall[4];

	Walls(){};
	Walls(const Wall west, const Wall north, const Wall east, const Wall south);
};

class TileInfo
{
private:
public:
	Walls _wls;

	FloorType fp;

	Point _p{0, 0};
	Obstacle obs;
	TileInfo(Point p, Walls walls, Obstacle obss, FloorType sa);
	TileInfo(){};
	Point GetPoint();
	Wall GetWall(Direction dir);
};

class TileManager
{
private:
	TileManager();

private:
	static vector<TileInfo> _tilevec;

public:
	static Wall CheckWall(Point point, Angle ang);
	static bool GetTileFromPosition(Point point, TileInfo &tl);
	static void AddTile(TileInfo tile);
	static bool IsTileAchieved(Point p);
	static vector<TileInfo> GetTiles();

	static void Initialize(Walls walls);
};
#endif