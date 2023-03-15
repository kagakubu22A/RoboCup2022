#include "MachineManager.h"
#include "M5Stack.h"
#include "BaseX.h"
#include "ToFManager.h"
#include "Tileinfo.h"
#include "Adafruit_MLX90614.h"
#include "MappingManager.h"
#include "RouteManager.h"
#include <numeric>
#include "I2CAddressChangerManager.h"

// �Z�}�t�H
volatile SemaphoreHandle_t MachineManager::semaphore;

// �W���C���Z���T�̉ߋ��̒l
vector<double> MachineManager::gyro_y(GYRO_SAVENUM);

// �ŏ��@�̂������Ă���������k�ƒ�`����
Direction MachineManager::NowFacingAbs = Direction::North;

BASE_X MachineManager::bx = BASE_X();

vector<MachineManager::Task> MachineManager::taskvec;

// ��ނ����^�C������
vector<Point> MachineManager::_retreatTiles;

bool MachineManager::ForceStop = false;

// ���̂ق���x���̐��̕����A�k�̂ق���y���̐��̕���
Point MachineManager::_nowRobotPosition{0, 0};

// �W���C���Z���T
MPU6050 *MachineManager::mpu;

// ���x�Z���T
Adafruit_MLX90614 MachineManager::mlx1 = Adafruit_MLX90614();
Adafruit_MLX90614 MachineManager::mlx2 = Adafruit_MLX90614();

// �����̃f�t�H���g�̉��x
double MachineManager::floorTemp = 0.0;

// �ŏ��̈�񂪍ς񂾂�(���̈�񂾂�������m�F����A���R�͏��߂����͕ǂ����邩������Ȃ�����(2��ڈȍ~�͌��ɕǂȂ�Ă���킯���Ȃ�))
bool MachineManager::_inited = false;

int MachineManager::tofdisarr[5] = {0, 0, 0, 0, 0};

vector<double> MachineManager::_temps{50};

/// @brief �|�C���g��\������
/// @param p ���W
/// @param isn ���s���邩(�f�t�H���g�͂���(true))
void MachineManager::DisplayPoint(Point p, bool isn)
{
	M5.Lcd.print("( ");
	M5.Lcd.print(p.x);
	M5.Lcd.print(", ");
	M5.Lcd.print(p.y);
	M5.Lcd.print(" )");
	if (isn)
		M5.Lcd.print("\n");
}

/// @brief �w�肳�ꂽ�}�X������������true�A���ɉ����Ȃ����false
bool MachineManager::IsDanger(Point p)
{
	TileInfo tl;
	// TileManager::GetTileFromPosition(p, tl);
	MappingManager::GetTileFromPosition(p, tl);
	if (tl.fp == FloorType::KeepOut)
		return true;
	return false;
}

/// @brief �f�o�b�O���[�h���I���̎��̂݃f�B���C�����s���邾���̊֐�
/// @param ms
void MachineManager::debDelay(int ms)
{
	if (DEBUGGING_MODE)
	{
		delay(ms);
	}
}

/// @brief ���̃^�C���ɐi�񂾂��ނ��邱�Ƃ����܂��Ă�����true,���Ȃ����false
/// @param p
/// @return
bool MachineManager::HasRetreated(Point p)
{
	for (Point tp : _retreatTiles)
	{
		if (p == tp)
			return true;
	}
	return false;
}

string MachineManager::p2str(Point p)
{
	string res = "(";
	res += std::to_string(p.x);
	res += ", ";
	res += std::to_string(p.y);
	res += ")";
	return res;
}

/// <summary>
/// ��^�C�����i�߂�
/// </summary>
/// <param name="ang"></param>
void MachineManager::Move1Tile()
{
	// ����
	// wl�̔z��ɓ����̂͐��k����̏�(��ΓI�w�W)
	// tofdisarr�ɓ����͍̂��O�E��̏�(���ΓI�w�W)

	// ����E��
	bool wallexists[4] = {false, false, false, false};

	//ToF�Z���T��ǂݎ���
	const int sensortimes = 30;

	bool ltreached = false;
	bool ttreached = false;
	bool rtreached = false;

	// ����
	// �����𑪂遨�}�b�s���O�ɒǉ����遨�����������ɏ]���ċ@�̂��ړ�������

	// �ŏ�������������
	if (!_inited)
	{
		tofdisarr[TOF_BACKWARD] = ToFManager::GetDistance(ToFAngle::Backward, sensortimes);
		_inited = true;
	}
	else
	{
		// �������ʂ��Ă����ꏊ�ɕǂ�����킯�Ȃ�����Ȃ����B
		tofdisarr[TOF_BACKWARD] = 9999;
	}

	if (MappingManager::IsReached(_nowRobotPosition) == -1)
	{
		tofdisarr[TOF_LEFT] = ToFManager::GetDistance(ToFAngle::Left, sensortimes);
		delay(10);
		tofdisarr[TOF_FORWARD] = ToFManager::GetDistance(ToFAngle::Forward, sensortimes);
		delay(10);
		tofdisarr[TOF_RIGHT] = ToFManager::GetDistance(ToFAngle::Right, sensortimes);
		delay(10);

		// ���k����
		Wall wl[4]{Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists};

		// �ǂ����邩�𔻒肷��
		for (int i = 0; i < 4; i++)
		{
			if (tofdisarr[i] < WALL_THRESHOLD_MM)
			{
				wl[(int)_a2dir((Angle)i)] = Wall::WallExists;
				wallexists[i] = true;
			}
		}

		TileInfo ti;
		ti._wls = Walls{wl[0], wl[1], wl[2], wl[3]};
		ti._p = _nowRobotPosition;

		// TODO ��Q�����m�Ƃ��ǉ�����
		// ti._obsvec;

		MappingManager::PointAdd(ti);
	}
	else
	{ // �����^�C����s���~�܂�ň����Ԃ��Ă����Ƃ��́A�������������f�[�^���擾���邱�ƂŎ��Z
		M5.Lcd.println("old tile");
		TileInfo ti;
		MappingManager::GetTileFromPosition(_nowRobotPosition, ti);

		// �ǂ̕���
		for (int i = 0; i < 4; i++)
		{
			wallexists[i] = ti._wls.wall[((int)_dir2a((Direction)i))] == Wall::WallExists ? true : false;
		}
	}

	Direction ldir = _a2dir(Angle::Left);
	Direction tdir = NowFacingAbs;
	Direction rdir = _a2dir(Angle::Right);
	Direction bdir = (Direction)(((int)NowFacingAbs + 2) % 4);

	Point leftP{_dir2p(_nowRobotPosition, ldir)};
	Point topP{_dir2p(_nowRobotPosition, tdir)};
	Point rightP{_dir2p(_nowRobotPosition, rdir)};

	rtreached = MappingManager::IsReached(rightP) != -1;
	ttreached = MappingManager::IsReached(topP) != -1;
	ltreached = MappingManager::IsReached(leftP) != -1;

	// 0 : �s�������ƂȂ���
	// 1 : ���łɍs�������Ƃ���
	// 2 : ��΂�����
	// 3 : ��΂����Ȃ����A�s�����ƂĂ��łɍs�������Ƃ���(�ǉz���ɂ��������}�X������Ƃ��ɂȂ邩��?)
	int canForward = 0; // 2
	int canRight = 0;	// 1
	// int canBack = 0;	// 4
	int canLeft = 0; // 3
					 // �D��x

	//  �O�ɍs���Ȃ�(�ǂ����� OR �O���� OR �O�̃}�X�͌�ނ��邵���Ȃ��}�X)�Ƃ�
	if (wallexists[1] || IsDanger(topP) || HasRetreated(topP))
	{
		canForward += 2;
	}
	// �E�ɍs���Ȃ�(�ǂ�����ȉ���)
	if (wallexists[2] || IsDanger(rightP) || HasRetreated(rightP))
	{
		canRight += 2;
	}
	if (wallexists[0] || IsDanger(leftP) || HasRetreated(leftP))
	{
		canLeft += 2;
	}
	if (rtreached)
	{
		canRight++;
	}
	if (ttreached)
	{
		canForward++;
	}
	if (ltreached)
	{
		canLeft++;
	}

#if DEBUGGING_MODE
	Serial.println("\n----------------------------------Robot conditions--------------------------------");
	MappingManager::DisplayMap();
	Serial.printf("now I'm at %s,Facing at %s\n", p2str(_nowRobotPosition).c_str(), Dir2Str(NowFacingAbs).c_str());
	Serial.printf("leftP %s, forwardP %s, rightP %s\n", p2str(leftP).c_str(), p2str(topP).c_str(), p2str(rightP).c_str());
	Serial.printf("isdanger left: %d, forward: %d, right: %d\n", IsDanger(leftP), IsDanger(topP), IsDanger(rightP));
	Serial.printf("hasretreated left: %d, forward: %d, right: %d\n", HasRetreated(leftP), HasRetreated(topP), HasRetreated(rightP));
	Serial.printf("tof left: %d, forward: %d, right: %d\n", tofdisarr[0], tofdisarr[1], tofdisarr[2]);
	Serial.printf("condition left: %d, forward: %d, right %d\n", wallexists[0], wallexists[1], wallexists[2]);
	Serial.printf("canleft: %d, canforward: %d, canright: %d\n", canLeft, canForward, canRight);
	Serial.println("gyro sensor:");
	Serial.printf("y[0] : %lf\ny[1] : %lf\n", mpu->gyro[1][0], mpu->gyro[1][1]);
	Serial.println("----------------------------------Robot condtion end-------------------------------");
#endif

	// �E��@�ł��邩��A�E�ɍs����Ƃ킩������Ƃɂ����E�ɍs��
	if (canRight == 0)
	{
		M5.Lcd.println("Moving Right");
		Serial.println("Moving Right");
		RotateRobot(-90);
		MoveForward(TILE_LENGTH_CM + 2.0);
	}
	// �E�ɂ͍s���Ȃ����O�ɍs����Ƃ킩������O�ɍs��
	if (canRight != 0 && canForward == 0)
	{
		M5.Lcd.println("Moving Forward");
		Serial.println("Moving Forward");
		MoveForward(TILE_LENGTH_CM);
	}
	// �E�ɂ��O�ɂ��s���Ȃ������ɍs����Ƃ킩�����獶��
	if (canRight != 0 && canForward != 0 && canLeft == 0)
	{
		M5.Lcd.println("Moving Left");
		Serial.println("Moving Left");
		RotateRobot(90);
		MoveForward(TILE_LENGTH_CM + 2.0);
	}
	// �E�ɂ��O�ɂ����ɂ��s���Ȃ��Ƃ킩������A��ނ��邩�V�����Ƃ���ɍs����
	// 180����]���Ă���߂�Ζ��Ȃ��Ƃ̂��� by�c��
	if (canRight >= 1 && canForward >= 1 && canLeft >= 1)
	{
		// �ǂ��������Ă������Ȃ��Ƃ�(�S���Q�ȏ�̎�)�A���
		if (canRight >= 2 && canForward >= 2 && canLeft >= 2)
		{
			M5.Lcd.println("Moving BackWard");
			_retTilesAdd(_nowRobotPosition);
			RotateRobot(180);
			MoveForward(TILE_LENGTH_CM + 5.0);
		}
		else // �ǂ������炢����Ƃ��́A�����邯�Ǎs���ĂȂ������}�X��T��
		{
			vector<Point> unevec = MappingManager::FindUnesxplored();
			if (unevec.size() == 0)
			{
				M5.Lcd.println("Exploring has done. Now returns.");
				Serial.println("Exploring has done. Now returns.");

				// �A�Ҏ��̍œK�ȃ��[�g������
				Route_v2 backRoute = RouteManager::Pos2Pos(_nowRobotPosition, Point{0, 0});

				// ���[�g�����ۂ̓����ɖ|��
				Route2Move(backRoute);
			}
			else
			{
				M5.Lcd.println("looks for new routes");
				Serial.println("looks for new routes");
				// ����I�Ɉ�Ԍ��ɂ�����̂���ԋ߂����(�K��)
				// FIXME �Ă��Ɓ[�Ȃ�Ō�Ŋm���߂܂���
				Route_v2 nr = RouteManager::Pos2Pos(_nowRobotPosition, MappingManager::GetUnreachedAndDelete());
				for (int i = 0; i < nr.size(); i++)
				{
					Serial.println(p2str(nr.at(i)).c_str());
				}
				Route2Move(nr);
			}
		}
	}
	/*M5.Lcd.clear();
	M5.Lcd.setCursor(0, 0);
	M5.Lcd.println("Map is below:");
	MappingManager::DisplayMap();
	delay(5000);*/
}

void MachineManager::ResetEncoder()
{
	bx.SetEncoderValue(BASE_X_TIRE_PORT_LEFT, 0);
	bx.SetEncoderValue(BASE_X_TIRE_PORT_RIGHT, 0);
}

void MachineManager::_motor_off()
{
	bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, 0);
	bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, 0);
}

Point MachineManager::_dir2p(Point p, Direction dir)
{
	if (dir == Direction::West)
	{
		return Point(p.x - 1, p.y);
	}
	else if (dir == Direction::North)
	{
		return Point(p.x, p.y + 1);
	}
	else if (dir == Direction::East)
	{
		return Point(p.x + 1, p.y);
	}
	else if (dir == Direction::South)
	{
		return Point(p.x, p.y - 1);
	}
	M5.Lcd.println("Direction does not match the list  :  in _convpFromAngle");
	delay(40000);
	throw std::runtime_error("Direction does not match the list  :  in _convpFromAngle");
}

/// @brief �p�x�̕␳
void MachineManager::DirCorrection()
{

	// �O�ɂ��Ă���Q�̃Z���T�[�̋����̍�����p�x���o��
	int f1 = (int)ToFManager::GetDistance(ToFAngle::Forward, 80);
	int f2 = (int)ToFManager::GetDistance(ToFAngle::Forward2, 80);

	Serial.printf("f1: %d, f2: %d\n", f1, f2);

	// ��������Ɛ��m�Ȓl���Ԃ��Ă��Ȃ��̂�
	if (f1 > 150 || f2 > 150)
		return;
	M5.Lcd.println(abs(f1 - f2));

	double gap = atan((f1 - f2) / (TOF_INTERVAL * 10.0)) * 180.0 / PI;

	Serial.printf("dir is : %lf\n", gap);

	// �@�̂�8���ȏ�X���Ă�����␳
	if (abs(gap) > ANGLE_CORRECTION_THRESHOLD)
	{
		// 45�x������������␳
		if (abs(gap) < 45.0)
		{
			RotateRobot(-gap);
		}
		else
		{
			M5.Lcd.println("Can't correct angle! \nRobot is too turned or ToF returned wrong value!");
		}
		M5.Lcd.printf("f1: %d, f2: %d\n", f1, f2);
		// delay(5000);
	}
}

void MachineManager::MoveForward(double cm)
{
	int timer = 0;
	const int wt = 1000;
	bool isClimbing = false;

	if (cm == 0)
		return;
	ResetEncoder();
	delay(5);

	// ���[�^�̌���
	int8_t sign = cm > 0 ? -1 : 1;

	// ��������(cm)���p�x�ɕϊ������Ƃ��̑傫��
	double movecm = 180.0 * cm / (PI * 5.5 / 2) * MOTOR_CONSTANT;

	bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, MOTOR_POWER_MOVING * sign);
	bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, MOTOR_POWER_MOVING * sign);

	for (int i = 0; i < 10; i++)
	{
		TCSManager::TCS_read();
	}

	// �w�肳�ꂽ���������܂őҋ@�A�����}�X���������瑦���Ɍ�ނ���
	while (abs(bx.GetEncoderValue(BASE_X_TIRE_PORT_LEFT)) < abs(movecm))
	{
		TCSManager::TCS_read();
		mpu->read();

		Serial.printf("gyro z : %lf\ngyro y : %lf\n", mpu->gyro[2][1], mpu->gyro[1][1]);

		// ���ɓ��ݍ��񂾂�
		if (TCSManager::Clear < 200)
		{
			M5.Lcd.println("Black Floor detected!!!");
			_motor_off();
			delay(20);

			// ��ނ��Ă�Ƃ��ɍ��ɓ��ݓ���邱�Ƃ͂Ȃ��c���?
			bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, 80 * -sign);
			bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, 80 * -sign);

			while (bx.GetEncoderValue(BASE_X_TIRE_PORT_LEFT) < 0)
			{
				delay(1);
			}
			_motor_off();
			delay(10);

			Point bp = _dir2p(_nowRobotPosition, NowFacingAbs);

			// ��������֎~�]�[���͑S���ʕǂ�����Ƃ݂Ȃ�
			TileInfo tl{bp, Walls{Wall::WallExists, Wall::WallExists, Wall::WallExists, Wall::WallExists}, vector<Obstacle>{Obstacle::Nothing}, FloorType::KeepOut};
			MappingManager::PointAdd(tl);

			ResetEncoder();
			delay(10);
			return;
		}

		// �W���C���Z���T���Ď����A�@�̂��X���Ă��邩������
		// Y�����Ď�����

		// TODO �s����ɂȂ������Βl�O��
		if (abs(mpu->gyro[1][1]) > 10.0)
		{
			Serial.printf("saka detected, %d", timer);
			M5.Lcd.println("slope detected");
			M5.Lcd.clear();
			M5.Lcd.setCursor(0, 0);

			// �@�̂̌X�������m������A���ꂪ�o���v�Ȃǂɂ����̂��⓹�ɂ����̂�������

			// �o���p�[�ƍ�̈Ⴂ�̓Z���T���X�����܂܂��X���̒l���ς�邩�ł���
			if (timer >= 15)
			{
				// 0.5�b�҂��āAy�������̂ق��ɌX���Ă������A�X���Ă��Ȃ���΃o���v�Ƃ݂Ȃ�
				if (abs(mpu->gyro[1][1]) > 10.0)
				{ // �₾������o�肫��
					isClimbing = true;

					double gyave = std::accumulate(gyro_y.begin(), gyro_y.end(), 0.0) / GYRO_SAVENUM;
					// TODO ����
					while (abs(gyave) > 4.0)
					{
						Serial.printf("clibming. gyro y is %3.3lf\n", gyro_y[0]);
						delay(2);
						gyave = std::accumulate(gyro_y.begin(), gyro_y.end(), 0.0) / GYRO_SAVENUM;
					}
				}

				// �o��؂�������������i�߂�
				if (isClimbing)
				{
					delay(220);
				}
				// �o��؂������₶��Ȃ������Ɣ��肳�ꂽ�炱���ɗ���A������x�g����悤�Ƀ��Z�b�g����
				isClimbing = false;
				timer = 0;
			}
			else // �ҋ@���Ԓ�
			{
				timer++;
			}
		}

		delay(1);
	}

	_motor_off();

	// �p�x��␳����
	DirCorrection();

	// ���x�Z���T�Ō���
	double t1 = 0.0, t2 = 0.0;
	for (int i = 0; i < 50; i++)
	{
		t1 += mlx1.readObjectTempC();
		t2 += mlx2.readObjectTempC();
	}
	t1 /= 50.0;
	t2 /= 50.0;

	if (t1 > 28.0 && t1 < 40.0)
	{
		DispenseRescueKit();
	}
	if (t2 > 28.0 && t2 < 40.0)
	{
		DispenseRescueKit();
	}

	ResetEncoder();

	delay(10);

	// �V�����ꏊ�Ɉړ�������X�V
	_nowRobotPosition = _dir2p(_nowRobotPosition, NowFacingAbs);
}

Direction MachineManager::_a2dir(Angle ang)
{
	if (ang == Angle::Right)
	{
		return (Direction)(((int)NowFacingAbs + 1) % 4);
	}
	else if (ang == Angle::Left)
	{
		int div = (((int)NowFacingAbs - 1) % 4);
		if (div < 0)
			div += 4;
		return (Direction)div;
	}
	else if (ang == Angle::Forward)
	{
		return NowFacingAbs;
	}
	else if (ang == Angle::Backward)
	{
		return (Direction)(((int)NowFacingAbs + 2) % 4);
	}
	M5.Lcd.println("ANGLE is INVALID");
	delay(5000);
	throw std::runtime_error("RUNTIME ERROR IN _dirconv");
}

Angle MachineManager::_dir2a(Direction d)
{
	int sa = ((int)NowFacingAbs - (int)d) % 4;
	if (sa < 0)
		sa += 4;

	switch (sa)
	{
	case 1:
		return Angle::Left;
	case 0:
		return Angle::Forward;
	case 3:
		return Angle::Right;
	case 2:
		return Angle::Backward;
	default:
		M5.Lcd.println("An error occurred at _dir2a()!");
		M5.Lcd.println("Arguments: dir, nowfacing");
		M5.Lcd.print((int)d);
		M5.Lcd.print(", ");
		M5.Lcd.print((int)NowFacingAbs);
		throw new std::runtime_error("An error occurred at _dir2a()!");
		break;
	}
}

/// @brief �@�̂���]������
/// @param ang ���g�̕�������ǂ���ɉ�]�����邩
void MachineManager::RotateRobot(double ang)
{
	if (ang == 0)
		return;

	mpu->_clear();
	delay(5);

	if (ang > 0) // ��
	{
		bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, 60);
		bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, -60);
	}
	if (ang < 0) // �E
	{
		bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, -60);
		bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, 60);
	}
	int8_t sign = ang > 0 ? 1 : -1;

	while ((mpu->gyro[2][1] < ang - GYRO_CONSTANT && ang > 0) || (mpu->gyro[2][1] > ang + GYRO_CONSTANT && ang < 0))
	{
		if (ForceStop)
		{
			_motor_off();
			return;
		}
		delay(1);
	}

	_motor_off();

	// ���p�̕ϊ�
	if ((ang > 89.0 && ang < 91.0) || (ang < -89.0 && ang > -91.0))
	{
		NowFacingAbs = _a2dir(ang > 0 ? Angle::Left : Angle::Right);
	}
	// TODO �����Ƌt�]���Ă邩��?
	else if (abs(ang) > 179.0 && abs(ang) < 181.0)
	{
		NowFacingAbs = Direction(((int)NowFacingAbs + 2) % 4);
	}

	delay(10);
	ResetEncoder();
	mpu->_clear();
	delay(5);
}

/// @brief �ŏ��̈��B�ǂ̌����ɐi��ł����������߂�
void MachineManager::Initialize()
{

	// looptask��ۑ��A�ً}��~���ɋ����I��suspended�ɂ����邽��
	taskvec.push_back(Task{.task = xTaskGetHandle(LOOPTASK_NAME), .name = LOOPTASK_NAME});

	if (taskvec[taskvec.size() - 1].task == NULL)
	{
		Serial.println("fatal: loop task does not found!!!");
	}

	semaphore = xSemaphoreCreateBinary();

	// �Z�}�t�H�͏������삪�Ȃ����s����Ŏ擾���Ă��������ƂɂȂ炸�A���̂����ł��̃Z�}�t�H���ꐶ�g���Ȃ��Ȃ�Ƃ�����������������̂ň�x�擾���Ď����
	BaseType_t xsematama = xSemaphoreTake(semaphore, 20);
	xSemaphoreGive(semaphore);

	Serial.println("Sensor checking start...");

	// TCS(�F�Z���T)�̃`�F�b�N
	Wire.beginTransmission(0x29);
	if (Wire.endTransmission() != 0)
	{
		M5.Lcd.println("TCS color sensor does not found!!");
		Serial.println("TCS color sensor does not found!!");
	}
	// 6���Z���T�̃`�F�b�N
	Wire.beginTransmission(0x68);
	if (Wire.endTransmission() != 0)
	{
		M5.Lcd.println("MPU6050 6 axises sensor does not found!!");
		Serial.println("MPU6050 6 axises sensor does not found!!");
	}

	// PCA�̃`�F�b�N
	Wire.beginTransmission(0x77);
	if (Wire.endTransmission() != 0)
	{
		M5.Lcd.println("PCA multi connector does not found!!");
		Serial.println("PCA multi connector does not found!!");
	}
	else
	{
		for (int i = 0; i < 5; i++)
		{
			I2CAddressChangerManager::ChangeAddress((unsigned char)i);
			Wire.beginTransmission(TOF_ADDRESS);
			if (Wire.endTransmission() != 0)
			{
				M5.Lcd.printf("tof angle %d does not found!!\n", i);
				Serial.printf("tof angle %d does not found!!\n", i);
			}
		}
	}

	// ���x�Z���T�̃`�F�b�N
	Wire.beginTransmission(0x5A);
	if (Wire.endTransmission() != 0)
	{
		M5.Lcd.println("Temp sensor 1 does not found!!");
		Serial.println("Temp sensor 1 does not found!!");
	}
	Wire.beginTransmission(0x55);
	if (Wire.endTransmission() != 0)
	{
		M5.Lcd.println("Temp sensor 2 does not found!!");
		Serial.println("Temp sensor 2 does not found!!");
	}

	Serial.println("Sensor checking finished");

	// BaseX�̏����ݒ�
	for (uint8_t i = 1; i <= 4; i++)
	{
		bx.SetMode(i, NORMAL_MODE);
	}

	// TCS(�F�Z���T)�̏����ݒ�
	TCSManager::TCS_begin();
	M5.Lcd.println("Tcs initialize finished.");

	// GP906(���x�Z���T)�̏����ݒ�
	mlx1.begin();
	mlx2.begin(0x55);

	// �����̉��x���擾����B���m�Ȓl���o�����߂�50�����ĕ��ς��Ƃ�
	for (int i = 0; i < 50; i++)
	{
		floorTemp += mlx1.readObjectTempC();
	}
	floorTemp /= 50.0;

	M5.Lcd.println("GY-906 initialize finished.");

	return;

	// �X�^�[�g�n�_�̏󋵂�c������
	/*Wall wltmp[4];
	for (int i = 0; i < 4; i++)
	{
		if (ToFManager::GetDistance((ToFAngle)i, 50) < WALL_THRESHOLD_MM)
		{
			wltmp[i] = Wall::WallExists;
		}
		else
		{
			wltmp[i] = Wall::WallNotExists;
		}
	}
	TileManager::Initialize(Walls{wltmp[0], wltmp[1], wltmp[2], wltmp[3]});*/
}

/// @brief MPU��o�^�B�ŏI�I�ɂ�MPU�̃N���X��static�ɂ��Ă��̍�Ƃ���Ȃ��悤�ɂ������˂�
/// @param m
void MachineManager::RegisterMPU(MPU6050 *m)
{
	mpu = m;
}

void MachineManager::_retTilesAdd(Point p)
{

	for (Point tp : _retreatTiles)
	{
		if (tp == p)
			return;
	}
	_retreatTiles.push_back(p);
}

// �`�F�b�N�|�C���g�����蒼���Ƃ��ɕ\�������ʂƂ��̐ݒ�
void MachineManager::DisplayRetry()
{
	M5.Lcd.clear();
	int selectionid = 0;
	vector<TileInfo> chkpvec;
	vector<TileInfo> tiles = MappingManager::GetPassedTileVec();

	M5.Lcd.println(tiles.size());

	if (tiles.size() < 1)
	{
		return;
	}

	// �܂��`�F�b�N�|�C���g��ǂݏo��
	for (TileInfo ti : tiles)
	{
		if (ti.fp == FloorType::CheckPoint)
		{
			chkpvec.push_back(ti);
		}
	}
	// TODO ����
	// chkpvec.push_back(TileInfo{Point(2, 3), Walls(Wall::WallExists, Wall::WallExists, Wall::WallExists, Wall::WallExists), vector<Obstacle>{Obstacle::Nothing}, FloorType::CheckPoint});
	// chkpvec.push_back(TileInfo{Point(1, 4), Walls(Wall::WallExists, Wall::WallExists, Wall::WallExists, Wall::WallExists), vector<Obstacle>{Obstacle::Nothing}, FloorType::CheckPoint});
	// chkpvec.push_back(TileInfo{Point(3, 3), Walls(Wall::WallExists, Wall::WallExists, Wall::WallExists, Wall::WallExists), vector<Obstacle>{Obstacle::Nothing}, FloorType::CheckPoint});
	// chkpvec.push_back(TileInfo{Point(4, 3), Walls(Wall::WallExists, Wall::WallExists, Wall::WallExists, Wall::WallExists), vector<Obstacle>{Obstacle::Nothing}, FloorType::CheckPoint});

	M5.Lcd.setCursor(ALIGN + 25, ALIGN);
	M5.Lcd.setTextSize(FONT_SIZE);

	for (int i = 0; i < chkpvec.size(); i++)
	{
		M5.Lcd.print("( ");
		M5.Lcd.print(chkpvec[i]._p.x);
		M5.Lcd.print(", ");
		M5.Lcd.print(chkpvec[i]._p.y);
		M5.Lcd.print(" )");
		// ���̍��W�Ɉړ�(25 pixel �i�߂�)
		M5.Lcd.setCursor(ALIGN + 25, (i + 1) * 25 + ALIGN);
	}
	// �J�[�\���̕`��
	M5.Lcd.setCursor(CURSOR_ALIGN, ALIGN);
	M5.Lcd.print(">");
	// �{�^���̓��͑҂�
	while (true)
	{
		M5.update();
		if (M5.BtnA.wasPressed())
		{
			if (selectionid > 0)
			{
				selectionid -= 1;
				// ��x�h��Ԃ��ăJ�[�\�����㏑��
				M5.Lcd.fillRect(0, 0, ALIGN, 600, 0);
				M5.Lcd.setCursor(CURSOR_ALIGN, (selectionid)*25 + ALIGN);
				M5.Lcd.print(">");
			}
		}
		if (M5.BtnC.wasPressed())
		{
			if (selectionid < chkpvec.size() - 1)
			{
				// ��x�h��Ԃ��ăJ�[�\�����㏑��
				selectionid += 1;
				M5.Lcd.fillRect(0, 0, ALIGN, 600, 0);
				M5.Lcd.setCursor(CURSOR_ALIGN, (selectionid)*25 + ALIGN);
				M5.Lcd.print(">");
			}
		}
		if (M5.BtnB.wasPressed())
		{
			// ���X�^�[�g�n�_��ݒ�
			_nowRobotPosition = chkpvec[selectionid]._p;
			NowFacingAbs = Direction::North;
			M5.Lcd.clear();
			M5.Lcd.setCursor(0, 30);
			M5.Lcd.print("the robot will start from the point\n( ");
			M5.Lcd.print(chkpvec[selectionid]._p.x);
			M5.Lcd.print(", ");
			M5.Lcd.print(chkpvec[selectionid]._p.y);
			M5.Lcd.print(" ).\n");
			M5.Lcd.print("Restarting... away From the robot.");
			return;
		}
	}
}

/// @brief ����������W��Ԃ�
/// @param ang
/// @return ���̌����̍��W
Point MachineManager::_a2p(Angle ang)
{
	return _dir2p(_nowRobotPosition, _a2dir(ang));
}

/// @brief ���[�g������ۂ̓����ɖ|�󂷂�
/// @param r
void MachineManager::Route2Move(Route_v2 r)
{
	for (int i = 0; i < r.size() - 1; i++)
	{

		// ���ɍs��
		//  TODO �v�f�o�b�O
		if (_a2p(Angle::Left) == r.at(i + 1))
		{
			RotateRobot(90);
			MoveForward(TILE_LENGTH_CM);
		}
		else if (_a2p(Angle::Forward) == r.at(i + 1))
		{
			MoveForward(TILE_LENGTH_CM);
		}
		else if (_a2p(Angle::Right) == r.at(i + 1))
		{
			RotateRobot(-90);
			MoveForward(TILE_LENGTH_CM);
		}
		else
		{
			RotateRobot(180);
			MoveForward(TILE_LENGTH_CM);
		}
	}
}

/// @brief ���X�L���[�L�b�g��f���o��
void MachineManager::DispenseRescueKit()
{
	// TODO ���X�L���[�L�b�g�r�o�@�\��낤
	M5.Lcd.println("NAGEMA-------------------SU");
	// delay(1000);
}

void MachineManager::MLXRead()
{
	for (int i = 0; i < _temps.size() - 1; i++)
	{
		_temps[i] = _temps[i + 1];
	}
	_temps[_temps.size() - 1] = mlx1.readObjectTempC();
}

string MachineManager::Dir2Str(Direction dir)
{
	switch (dir)
	{
	case Direction::West:
		return "West";
		break;
	case Direction::North:
		return "North";
		break;
	case Direction::East:
		return "East";
		break;
	case Direction::South:
		return "South";
		break;
	default:
		return "Unknown direction";
		break;
	}
}