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

// �Z���T�̑��݃`�F�b�N�A����I�ȓǂݍ��݂��X�L�b�v����B�f�o�b�O�p�t���O�B
#define SKIP_SENSORS_CHECK false

// �f�o�b�O�����ۂ�
#define DEBUGGING_MODE true

// 26�͔�
#define LED_PIN 26

// �W���C���Z���T�̉ߋ��̋L�^��ۑ����鐔
#define GYRO_SAVENUM 5

// �z��Ŏg��
#define TOF_LEFT 0
#define TOF_FORWARD 1
#define TOF_RIGHT 2
#define TOF_BACKWARD 3
#define TOF_FORWARD_2 4

// 1�^�C�����̒���(cm)
#define TILE_LENGTH_CM 30

// ���[�^�[�̃p���[(���i)
#define MOTOR_POWER_MOVING 90

// ���[�^�[�̃p���[(�Ȃ���)
#define MOTOR_POWER_ROTATING 60

// ����ȏ�̋�����������ǂ��Ȃ��Ƃ݂Ȃ�����(mm)
#define WALL_THRESHOLD_MM 180

// BaseX�̃|�[�g�Ɩ���
#define BASE_X_TIRE_PORT_LEFT 1
#define BASE_X_TIRE_PORT_RIGHT 4
#define BASE_X_RESCUEKIT_PORT 3
#define BASE_X_PORT_4 4

// ����
#define PLUS true
#define MINUS false

// ���[�^�[��30�p�i�ނ��߂ɕK�v�Ȕ{��
// TODO �{������Ȃ��Ē����ɂ���
#define MOTOR_CONSTANT 1.60

// 90�x��]���邽�߂ɕK�v�ȕ␳�l
#define GYRO_CONSTANT 6.0

// �t�H���g�̃T�C�Y
#define FONT_SIZE 1

// �]��
#define ALIGN 30
#define ALIGN_V 15
#define CURSOR_ALIGN 15

// �O�ɂ��Ă���2��TOF�Z���T�[�̊Ԋu(�p�x�␳���Ɏg��)
#define TOF_INTERVAL 9.5

// ����ȏ�p�x�̂�������m������p�x�␳������
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

	// ��ނ����^�C�������̔z��A���̃^�C���ɂ͓��ݓ���Ȃ��悤�ɍs������
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
