#pragma once

#include "BaseX.h"
#include "TileInfo.h"
#include "Point.h"
#include "MPU6050.h"
#include <vector>
#include "TCSManager.h"
#include "Adafruit_MLX90614.h"
#include "AngDir.h"
#include "Route_v2.h"
#include <string>

using std::string;
using std::vector;

// センサの存在チェック、定期的な読み込みをスキップする。デバッグ用フラグ。
#define SKIP_SENSORS_CHECK false

// デバッグ中か否か
#define DEBUGGING_MODE true

// 26は白
#define LED_PIN 26

// ジャイロセンサの過去の記録を保存する数
#define GYRO_SAVENUM 5

// 配列で使う
#define TOF_LEFT 0
#define TOF_FORWARD 1
#define TOF_RIGHT 2
#define TOF_BACKWARD 3
#define TOF_FORWARD_2 4

// 1タイル分の長さ(cm)
#define TILE_LENGTH_CM 30

// モーターのパワー(直進)
#define MOTOR_POWER_MOVING 90

// モーターのパワー(曲がり)
#define MOTOR_POWER_ROTATING 60

// これ以上の距離あったら壁がないとみなす距離(mm)
#define WALL_THRESHOLD_MM 180

// BaseXのポートと役割
#define BASE_X_TIRE_PORT_LEFT 1
#define BASE_X_TIRE_PORT_RIGHT 4
#define BASE_X_RESCUEKIT_PORT 3
#define BASE_X_PORT_4 4

// 符号
#define PLUS true
#define MINUS false

// モーターが30㎝進むために必要な倍率
// TODO 倍率じゃなくて長さにする
#define MOTOR_CONSTANT 1.60

// 90度回転するために必要な補正値
#define GYRO_CONSTANT 6.0

// フォントのサイズ
#define FONT_SIZE 1

// 余白
#define ALIGN 30
#define ALIGN_V 15
#define CURSOR_ALIGN 15

// 前についている2つのTOFセンサーの間隔(角度補正時に使う)
#define TOF_INTERVAL 9.5

// これ以上角度のずれを検知したら角度補正をする
#define ANGLE_CORRECTION_THRESHOLD 10.0

class MachineManager
{
private:
	MachineManager();

private:
	static Point _nowRobotPosition;
	static Direction NowFacingAbs;
	static BASE_X bx;
	static MPU6050 *mpu;
	static Adafruit_MLX90614 mlx1;
	static Adafruit_MLX90614 mlx2;
	static bool _inited;

	// 後退したタイルたちの配列、このタイルには踏み入らないように行動する
	static vector<Point> _retreatTiles;

	static int tofdisarr[5];

	static double floorTemp;

	static void ResetEncoder();
	static void _retTilesAdd(Point p);
	static void _motor_off();

	static void debDelay(int ms);

	static vector<double> _temps;

public:
	static vector<double> gyro_y;
	static string p2str(Point p);
	static volatile SemaphoreHandle_t semaphore;
	static Direction _a2dir(Angle ang);
	static Angle _dir2a(Direction d);
	static Point _dir2p(Point p, Direction dir);
	static Point _a2p(Angle ang);

	static Direction GetRobotDir() { return NowFacingAbs; }
	static Point GetRobotPos() { return _nowRobotPosition; }

	static bool ForceStop;
	static void Initialize();
	static void Move1Tile();

	static bool HasRetreated(Point p);

	static void DisplayRetry();

	static void DirCorrection();

	static void MoveForward(double cm);

	static void RotateRobot(double ang);

	static bool IsDanger(Point p);

	static void RegisterMPU(MPU6050 *m);

	static void DisplayPoint(Point p, bool isn = true);

	static void Route2Move(Route_v2 r);

	static void DispenseRescueKit();

	static void MLXRead();

	static string Dir2Str(Direction dir);
};
