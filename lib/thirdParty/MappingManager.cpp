#include "MappingManager.h"
#include "MachineManager.h"
#include <M5Stack.h>

vector<vector<TileInfo>> MappingManager::_passedTilevecs = vector<vector<TileInfo>>(1);
vector<vector<Point>> MappingManager::_unexploredPosvecs = vector<vector<Point>>(1);

// TODO structかなんか作ってまとめない?
vector<int> MappingManager::xMaxs = vector<int>(1, 0), MappingManager::yMaxs = vector<int>(1, 0), MappingManager::xmins = vector<int>(1, 0), MappingManager::ymins = vector<int>(1, 0);

//今何階にいるか
int MappingManager::nowFloor = 1;

/// @brief 指定した座標が既に登録されているか調べる
/// @param p 登録する座標
/// @return 既に登録されている場合はfalse、新たに登録した場合はtrueを返し、この場合にのみ配列に追加される
bool MappingManager::PointAdd(TileInfo tile)
{
	for (TileInfo ti : _passedTilevecs[nowFloor - 1])
	{
		if (ti.GetPoint() == tile.GetPoint())
		{
			return false;
		}
	}

	// 最大値、最小値の更新
	if (xMaxs[nowFloor - 1] < tile._p.x)
	{
		xMaxs[nowFloor - 1] = tile._p.x;
	}
	if (yMaxs[nowFloor - 1] < tile._p.y)
	{
		yMaxs[nowFloor - 1] = tile._p.y;
	}
	if (xmins[nowFloor - 1] > tile._p.x)
	{
		xmins[nowFloor - 1] = tile._p.x;
	}
	if (ymins[nowFloor - 1] > tile._p.y)
	{
		ymins[nowFloor - 1] = tile._p.y;
	}

	_passedTilevecs[nowFloor - 1].push_back(tile);
	Serial.printf("new tile added at: %s\n", MachineManager::p2str(tile._p).c_str());

	// いけるけど行っていないタイルを更新する
	for (int i = 0; i < 4; i++)
	{
		if (tile.GetWall((Direction)i) == Wall::WallNotExists)
		{
			Point tp = MachineManager::_dir2p(tile._p, (Direction)i);
			if (checkOverlap(_passedTilevecs[nowFloor - 1], tp) == -1)
			{
				_unexploredPosvecs[nowFloor - 1].push_back(tp);
				Serial.printf("new unexplored tile at %s\n", MachineManager::p2str(tp).c_str());
			}
		}
	}
	// 行けるけど行っていないタイルのうち、今回で訪れたタイルがあれば削除
	for (int i = 0; i < _unexploredPosvecs[nowFloor - 1].size(); i++)
	{
		if (_unexploredPosvecs[nowFloor - 1][i] == tile._p)
		{
			_unexploredPosvecs[nowFloor - 1].erase(_unexploredPosvecs[nowFloor - 1].begin() + i);
		}
	}
	return true;
}

/// @brief いけるけど行ってない座標の配列を返す
/// @return
vector<Point> MappingManager::FindUnesxplored()
{
	return _unexploredPosvecs[nowFloor - 1];
}

/// @brief ダブりがあるかどうかを返す
/// @param tls
/// @param checkedTile
/// @return ダブったらtrue,ダブんなかったらfalse
int MappingManager::checkOverlap(vector<TileInfo> tls, TileInfo checkedTile)
{
	for (int i = 0; i < tls.size(); i++)
	{
		if (tls[i].GetPoint() == checkedTile.GetPoint())
			return i;
	}

	return -1;
}

int MappingManager::checkOverlap(vector<Point> ps, Point checkedPos)
{
	for (int i = 0; i < ps.size(); i++)
	{
		if (ps[i] == checkedPos)
		{
			return i;
		}
	}
	return -1;
}

int MappingManager::checkOverlap(vector<TileInfo> tl, Point checkedPos)
{
	for (int i = 0; i < tl.size(); i++)
	{
		if (tl[i]._p == checkedPos)
		{
			return i;
		}
	}
	return -1;
}

/// @brief デバッグ用関数、任意の向きに壁があるタイルを作れる
/// @param west
/// @param north
/// @param east
/// @param south
/// @return
Walls MappingManager::_wallgenerate(bool west, bool north, bool east, bool south)
{
	return Walls{west ? Wall::WallExists : Wall::WallNotExists, north ? Wall::WallExists : Wall::WallNotExists, east ? Wall::WallExists : Wall::WallNotExists, south ? Wall::WallExists : Wall::WallNotExists};
}

void MappingManager::DisplayMap()
{
	/*Walls allwall = Walls{Wall::WallExists, Wall::WallExists, Wall::WallExists, Wall::WallExists};

	PointAdd(TileInfo{Point{-1, 0}, _wallgenerate(true, false, true, true), vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{-1, 1}, _wallgenerate(true, false, false, false), vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{-1, 2}, _wallgenerate(false, false, true, true), vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{0, 0}, _wallgenerate(true, false, true, true), vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	//  PointAdd(TileInfo{Point{0, 0}, allwall, vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{0, 1}, Walls{Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists}, vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{0, 2}, allwall, vector<Obstacle>{Obstacle::Nothing}, FloorType::KeepOut});
	PointAdd(TileInfo{Point{1, 0}, allwall, vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{1, 1}, allwall, vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{1, 2}, allwall, vector<Obstacle>{Obstacle::Nothing}, FloorType::None});
	PointAdd(TileInfo{Point{-1, -1}, allwall, vector<Obstacle>{Obstacle::Nothing}, FloorType::None});*/
	const int tilesize = 10;

	// x,y方向に何マスあるか
	int xno = abs(xMaxs[nowFloor - 1] - xmins[nowFloor - 1]) + 1, yno = abs(yMaxs[nowFloor - 1] - ymins[nowFloor - 1]) + 1;

	// マスをいくつかの記号の集合体として表現したとき、何個必要か
	int serialxsize = tilesize + (xno - 1) * (tilesize - 1), serialysize = tilesize + (yno - 1) * (tilesize - 1);

	vector<vector<unsigned char>> wallvec(serialysize, vector<unsigned char>(serialxsize, 0x80));

	for (int y = 0; y < yno; y++)
	{
		for (int x = 0; x < xno; x++)
		{
			//[y][x]の順で座標を突っ込まないといけない点に注意
			TileInfo ti;
			int nowFocusingTile_x = x + xmins[nowFloor - 1];
			int nowFocusingTile_y = y + ymins[nowFloor - 1];
			if (GetTileFromPosition(Point{x + xmins[nowFloor - 1], y + ymins[nowFloor - 1]}, ti))
			{
				Serial.printf("Point (%d, %d) found.\n", x + xmins[nowFloor - 1], y + ymins[nowFloor - 1]);

				// まずはじめに進入禁止だったら塗りつぶす
				if (ti.fp == FloorType::KeepOut)
				{
					for (int filly = 0; filly < tilesize; filly++)
					{
						for (int fillx = 0; fillx < tilesize; fillx++)
						{
							wallvec[(serialysize - 1) - ((tilesize - 1) * (y + 1)) + filly][(tilesize - 1) * x + fillx] = 0x81;
						}
					}
				}

				// 座標を描く
				int nposx = ((tilesize - 1) * x) + 1;
				int nposy = ((serialysize - 1) - ((tilesize - 1) * (y + 1))) + 1;

				// 自分のいる座標だったら矢印を描く
				if (Point{x + xmins[nowFloor - 1], y + ymins[nowFloor - 1]} == MachineManager::GetRobotPos())
				{
					wallvec[nposy + 4][nposx + 4] = (int)MachineManager::GetRobotDir() + DIRECTION_WEST;
				}

				if (nposx >= POSITION_WRITING_END_FLAG || nposy >= POSITION_WRITING_END_FLAG)
				{
					Serial.println("maze is too big to draw!!!");
					return;
				}

				// 座標お絵かき始まりフラグ
				wallvec[nposy][nposx] = POSITION_WRITING_BEGIN_FLAG;
				nposx++;
				// 座標マイナスなら符号を描く
				if (x + xmins[nowFloor - 1] < 0)
				{
					wallvec[nposy][nposx] = POSITION_SIGNAL_FLAG;
					nposx++;
				}

				wallvec[nposy][nposx] = abs(nowFocusingTile_x);
				nposx++;

				// カンマ
				wallvec[nposy][nposx] = POSITION_SPLIT_X_Y_FLAG;
				nposx++;

				// 座標マイナスなら符号を描く
				if (x + xmins[nowFloor - 1] < 0)
				{
					wallvec[nposy][nposx] = POSITION_SIGNAL_FLAG;
					nposx++;
				}

				wallvec[nposy][nposx] = abs(nowFocusingTile_y);
				nposx++;

				// 終了フラグ
				wallvec[nposy][nposx] = POSITION_WRITING_END_FLAG;

				// 壁を描く
				if (ti._wls.wall[WALL_WEST] == Wall::WallExists)
				{
					for (int i = 0; i < tilesize; i++)
					{
						wallvec[(serialysize - 1) - ((tilesize - 1) * y + i)][(tilesize - 1) * x] = 0x81;
					}
				}
				if (ti._wls.wall[WALL_NORTH] == Wall::WallExists)
				{
					for (int i = 0; i < tilesize; i++)
					{
						wallvec[(serialysize - 1) - ((tilesize - 1) * (y + 1))][(tilesize - 1) * x + i] = 0x81;
					}
				}
				if (ti._wls.wall[WALL_EAST] == Wall::WallExists)
				{
					for (int i = 0; i < tilesize; i++)
					{
						wallvec[(serialysize - 1) - ((tilesize - 1) * y + i)][(tilesize - 1) * (x + 1)] = 0x81;
					}
				}
				if (ti._wls.wall[WALL_SOUTH] == Wall::WallExists)
				{
					for (int i = 0; i < tilesize; i++)
					{
						wallvec[(serialysize - 1) - ((tilesize - 1) * (y))][(tilesize - 1) * x + i] = 0x81;
					}
				}
			}
		}
	}

	Serial.printf("size is (%d, %d)\n", wallvec[0].size(), wallvec.size());

	// 迷路出力
	Serial.println("\n");

	// 座標を出力しているときは、普通と異なる挙動(座標もその他壁有り無し等もcharで表現しているためかぶってしまうと困る)をするので特別に
	bool poswriting = false;

	// 翻訳、0x00~0xFFの記号を文字に置き換えシリアルに送信する
	for (int y = 0; y < wallvec.size(); y++)
	{
		for (int x = 0; x < wallvec[y].size(); x++)
		{
			if (poswriting)
			{
				switch (wallvec[y][x])
				{
				case POSITION_WRITING_END_FLAG:
					Serial.print(" )");
					poswriting = false;

					// 辻褄合わせ
					x += 2;
					break;
				case POSITION_SIGNAL_FLAG:
					Serial.print("- ");
					break;
				case POSITION_SPLIT_X_Y_FLAG:
					Serial.print(", ");
					break;
				default:
					Serial.printf("%-3d ", wallvec[y][x]);
					break;
				}
			}
			else
			{
				switch (wallvec[y][x])
				{
				case 0x81:
					Serial.print("██");
					break;
				case POSITION_WRITING_BEGIN_FLAG:
					poswriting = true;
					Serial.print("( ");
					break;
				case 0x80:
					Serial.print("  ");
					break;
				case DIRECTION_WEST:
					Serial.print("←");
					break;
				case DIRECTION_NORTH:
					Serial.print("↑");
					break;
				case DIRECTION_EAST:
					Serial.print("→");
					break;
				case DIRECTION_SOUTH:
					Serial.print("↓");
					break;
				}
			}
		}
		Serial.print("\n");
	}
}
/// @brief 未到達の配列の中から最後に追加されたものを返す。返したものはもう探索したとみなして削除する
/// @return
Point MappingManager::GetUnreachedAndDelete()
{
	int at = _unexploredPosvecs[nowFloor - 1].size() - 1;
	Point p = _unexploredPosvecs[nowFloor - 1][at];
	Serial.println("candidates to reach:");
	for (int i = 0; i < _unexploredPosvecs[nowFloor - 1].size(); i++)
	{
		Serial.printf("%s\n", MachineManager::p2str(_unexploredPosvecs[nowFloor - 1][i]).c_str());
	}
	_unexploredPosvecs[nowFloor - 1].erase(_unexploredPosvecs[nowFloor - 1].begin() + at);
	M5.Lcd.printf("Deleted at :");
	Serial.printf("deleted at : (%d, %d)\n", p.x, p.y);
	return p;
}

/// @brief その座標にあるタイルを返す
/// @param point 座標
/// @param tl 返却されるタイル
/// @return あったらtrue、なかったらfalse
bool MappingManager::GetTileFromPosition(Point point, TileInfo &tl)
{
	for (TileInfo ti : _passedTilevecs[nowFloor - 1])
	{
		if (ti.GetPoint() == point)
		{
			tl = ti;
			return true;
		}
	}
	return false;
}

int MappingManager::IsReached(Point p)
{
	for (int i = 0; i < _passedTilevecs[nowFloor - 1].size(); i++)
	{
		if (_passedTilevecs[nowFloor - 1][i]._p == p)
			return i;
	}
	return -1;
}

vector<TileInfo> MappingManager::GetPassedTileVec()
{
	return _passedTilevecs[nowFloor - 1];
}
// TODO 1階から始まることを想定しています
/// @brief
/// @param floor
void MappingManager::FloorChanged(int floor)
{
	while (floor >= _unexploredPosvecs.size())
	{
		_unexploredPosvecs.push_back(vector<Point>());
		_passedTilevecs.push_back(vector<TileInfo>());
		xMaxs.push_back(0);
		// TODO xmax,yminなど追加、xmax[0]を0にする
	}
	nowFloor = floor;
}