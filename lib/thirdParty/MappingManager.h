#ifndef __MAPPING_MANAGER_H__
#define __MAPPING_MANAGER_H__

#define POSITION_WRITING_BEGIN_FLAG 0xFB
#define POSITION_WRITING_END_FLAG 0xFC
#define POSITION_SIGNAL_FLAG 0xFF
#define POSITION_SPLIT_X_Y_FLAG 0xFE

#define DIRECTION_WEST 0x90
#define DIRECTION_NORTH 0x91
#define DIRECTION_EAST 0x92
#define DIRECTION_SOUTH 0x93

#define VICTIM_

#include <vector>
#include "Point.h"
#include "TileInfo.h"
using std::vector;

class MappingManager{
    private:
    MappingManager();
    
    private:

    static vector<TileInfo> _passedTileVec;
    static vector<Point> _unexploredPosvec;

    static int xMax;
    static int yMax;
    static int xmin;
    static int ymin;

    static int checkOverlap(vector<TileInfo> tls, TileInfo checkedTile);
    static int checkOverlap(vector<Point> ps, Point checkedPos);
    static int checkOverlap(vector<TileInfo> tl, Point checkedPos);

    static Walls _wallgenerate(bool west,bool north,bool east,bool south);

    public:
    static bool PointAdd(TileInfo tile);

    static int IsReached(Point p);

    static void DisplayMap();

    static vector<Point> FindUnesxplored();

    static vector<TileInfo> GetPassedTileVec();

    static Point GetUnreachedAndDelete();

    static bool GetTileFromPosition(Point point, TileInfo &tl);

    static void DeleteAsBlack(Point p);
};

#endif