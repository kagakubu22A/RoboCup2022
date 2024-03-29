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

// セマフォ
volatile SemaphoreHandle_t MachineManager::semaphore;

// ジャイロセンサの過去の値
vector<double> MachineManager::gyro_y(GYRO_SAVENUM);

// 最初機体が向いていた方向を北と定義する
Direction MachineManager::NowFacingAbs = Direction::North;

BASE_X MachineManager::bx = BASE_X();

array<TCSManager, 3> MachineManager::tcs;

vector<MachineManager::Task> MachineManager::taskvec;

// 後退したタイルたち
vector<Point> MachineManager::_retreatTiles;

bool MachineManager::ForceStop = false;

// 何かしらの段差を検知したら、次は前進か後退かしか与えない
bool MachineManager::isprevclimb = false;

// 東のほうがx軸の正の方向、北のほうがy軸の制の方向
Point MachineManager::_nowRobotPosition{0, 0};

// ジャイロセンサ
MPU6050 *MachineManager::mpu;

// 温度センサ
Adafruit_MLX90614 MachineManager::mlx1 = Adafruit_MLX90614();
Adafruit_MLX90614 MachineManager::mlx2 = Adafruit_MLX90614();

// 坂(何かしらの傾斜)を検出したときのモータのエンコード値
int MachineManager::prevMotorEnc = -1;

// さっきバンプを上ったか
bool MachineManager::bumpclimbed = false;

// さっき階段を上ったか
bool MachineManager::stairClimbed = false;

// 部屋のデフォルトの温度
double MachineManager::floorTemp = 0.0;

// 教授不要な被災者発見
bool MachineManager::safeVictimFound = false;

// 要救助者発見
bool MachineManager::victimInNeedFound = false;

// 最初の一回が済んだか(この一回だけ後方を確認する、理由は初めだけは壁があるかもしれないから(2回目以降は後ろに壁なんてあるわけがない))
bool MachineManager::_inited = false;

// 坂やバンプの影響を受けて後退するときにtrueになる。trueになっているとき、坂の検知を無効にする。
// TODO 2個連続してバンプが置かれないという前提の上で成立します。後で対応させること。
bool MachineManager::SpecialMoving = false;

int MachineManager::tofdisarr[5] = {0, 0, 0, 0, 0};

vector<double> MachineManager::_temps{50};

/// @brief ポイントを表示する
/// @param p 座標
/// @param isn 改行するか(デフォルトはする(true))
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

/// @brief 指定されたマスが黒だったらtrue、特に何もなければfalse
bool MachineManager::IsDanger(Point p)
{
	TileInfo tl;
	// TileManager::GetTileFromPosition(p, tl);
	MappingManager::GetTileFromPosition(p, tl);
	if (tl.fp == FloorType::KeepOut)
		return true;
	return false;
}

/// @brief デバッグモードがオンの時のみディレイを実行するだけの関数
/// @param ms
void MachineManager::debDelay(int ms)
{
	if (DEBUGGING_MODE)
	{
		delay(ms);
	}
}

/// @brief そのタイルに進んだら後退することが決まっていたらtrue,しなければfalse
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
	res += ", ";
	res += std::to_string(p.floor);
	res += ")";
	return res;
}

/// <summary>
/// 一タイル分進める。順序:測定する→レスキューキット吐くなどの処理→進む
/// </summary>
/// <param name="ang"></param>
void MachineManager::Move1Tile()
{
	// 注意
	// wlの配列に入れるのは西北東南の順(絶対的指標)
	// tofdisarrに入れるのは左前右後の順(相対的指標)

	// 左上右下
	bool wallexists[4] = {false, false, false, false};

	// ToFセンサを読み取る回数
	const int sensortimes = 10;

	SpecialMoving = false;

	bool ltreached = false;
	bool ttreached = false;
	bool rtreached = false;

	// 流れ
	// 距離を測る→マッピングに追加する→測った距離に従って機体を移動させる

	// 坂などの障害物がなかった時には毎回リセットをする
	if (!isprevclimb)
	{
		if (xSemaphoreTake(semaphore, 20) == pdTRUE)
		{
			mpu->_reset();
			delay(10);
			xSemaphoreGive(semaphore);
			Serial.println("gyro sensor resetted");
		}
		else
		{
			Serial.println("tried to get semaphore to reset mpu, but failed to do!");
		}
	}
	else
	{
		Serial.println("gyro sensor reset skipped");
		SpecialMoving = true;
	}

	// 最初だけ後ろも測る
	if (!_inited)
	{
		tofdisarr[TOF_BACKWARD] = ToFManager::GetDistance(ToFAngle::Backward, sensortimes);
		_inited = true;
	}
	else
	{
		// さっき通ってきた場所に壁があるわけないじゃないか。
		tofdisarr[TOF_BACKWARD] = 9999;
	}

	// 初めて来たところだったら計測する
	if (MappingManager::IsReached(_nowRobotPosition) == -1)
	{
		// 壁の有無を計測
		tofdisarr[TOF_LEFT] = ToFManager::GetDistance(ToFAngle::Left, sensortimes);
		delay(10);
		tofdisarr[TOF_FORWARD] = ToFManager::GetDistance(ToFAngle::Forward, sensortimes);
		delay(10);
		tofdisarr[TOF_RIGHT] = ToFManager::GetDistance(ToFAngle::Right, sensortimes);
		delay(10);

		//前回の探索で被災者を検知したら知らせる
		if (victimInNeedFound)
		{
			FlashLED();
			DispenseRescueKit();
		}
		else if (safeVictimFound)
		{
			FlashLED();
		}

		// 西北東南
		Wall wl[4]{Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists, Wall::WallNotExists};

		// 壁があるかを判定する
		for (int i = 0; i < 4; i++)
		{
			if (tofdisarr[i] < WALL_THRESHOLD_MM)
			{
				wl[(int)_a2dir((Angle)i)] = Wall::WallExists;
				wallexists[i] = true;
			}
		}

		TileInfo ti;

		// 地面の色を見て、銀色だったらチェックポイントとして追加
		// TODO 銀判定
		tcs[0].TCS_read();
		if (tcs[0].Clear > 2000)
		{
			ti.fp = FloorType::CheckPoint;
		}

		ti._wls = Walls{wl[0], wl[1], wl[2], wl[3]};
		ti._p = _nowRobotPosition;

		// TODO バンプの場合、さっきいたタイルの端のほうにあったのか、今いるタイルの最初のほうにいるのか見分けをつける
		// さっき何かしら上った→バンプか階段かのどちらか
		if (isprevclimb)
		{
			if (bumpclimbed)
			{
				BothPrintln("Bump detected");
				ti.obs = Obstacle::Bump;
			}
			else
			{
				// スロープだったら、1マス分進んでも傾きは大きいままのはず
				if (mpu->gyro[1][1] > 18.0)
				{
					ti.obs = Obstacle::Slope;
					MoveUntilFlat();
					MappingManager::FloorChanged(2);
					return;
				}
				else
				{
					BothPrintln("Stair detected");
					ti.obs = Obstacle::Step;
				}
			}
		}
		MappingManager::PointAdd(ti);
	}
	else
	{ // 黒いタイルや行き止まりで引き返してきたときは、さっき測ったデータを取得することで時短
		M5.Lcd.println("old tile");
		TileInfo ti;
		MappingManager::GetTileFromPosition(_nowRobotPosition, ti);

		// 壁の復元
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

	// 0 : 行ったことないよ
	// 1 : すでに行ったことある
	// 2 : 絶対いけん。もしくは、さっき何かしらの段差か坂を超えたためにいけない
	// 3 : 絶対いけないし、行ったとてすでに行ったことある(壁越しにそういうマスがあるときになるかな?)
	int canForward = 0; // 2
	int canRight = 0;	// 1
	// int canBack = 0;	// 4
	int canLeft = 0; // 3
					 // 優先度

	//  前に行けない(壁がある OR 前が黒 OR 前のマスは後退するしかないマス)とき
	if (wallexists[1] || IsDanger(topP) || HasRetreated(topP))
	{
		canForward += 2;
	}
	// 右に行けない(壁がある以下略)
	if (wallexists[2] || IsDanger(rightP) || HasRetreated(rightP) || isprevclimb)
	{
		canRight += 2;
	}
	if (wallexists[0] || IsDanger(leftP) || HasRetreated(leftP) || isprevclimb)
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
	Serial.printf("hasreached left: %d, forward: %d, right: %d\n", MappingManager::IsReached(leftP), MappingManager::IsReached(topP), MappingManager::IsReached(rightP));
	Serial.printf("tof left: %d, forward: %d, right: %d\n", tofdisarr[0], tofdisarr[1], tofdisarr[2]);
	Serial.printf("condition left: %d, forward: %d, right %d\n", wallexists[0], wallexists[1], wallexists[2]);
	Serial.printf("canleft: %d, canforward: %d, canright: %d\n", canLeft, canForward, canRight);
	Serial.println("gyro sensor:");
	Serial.printf("y[0] : %lf\ny[1] : %lf\n", mpu->gyro[1][0], mpu->gyro[1][1]);
	Serial.println("----------------------------------Robot condtion end-------------------------------");
#endif

	// 右手法であるから、右に行けるとわかったらとにかく右に行く
	if (canRight == 0)
	{
		M5.Lcd.println("Moving Right");
		Serial.println("Moving Right");
		RotateRobot(-90);
		MoveForward(TILE_LENGTH_CM + 2.0);
	}
	// 右には行けないが前に行けるとわかったら前に行く
	if (canRight != 0 && canForward == 0)
	{
		isprevclimb = false;
		M5.Lcd.println("Moving Forward");
		Serial.println("Moving Forward");
		MoveForward(TILE_LENGTH_CM);
	}
	// 右にも前にも行けないが左に行けるとわかったら左へ
	if (canRight != 0 && canForward != 0 && canLeft == 0)
	{
		M5.Lcd.println("Moving Left");
		Serial.println("Moving Left");
		RotateRobot(90);
		MoveForward(TILE_LENGTH_CM + 2.0);
	}
	// 右にも前にも左にも行けないとわかったら、後退するか新しいところに行くか
	// 180°回転してから戻れば問題ないとのこと by田中
	if (canRight >= 1 && canForward >= 1 && canLeft >= 1)
	{
		// どうあがいてもいけないとき(全部２以上の時)、後退
		if (canRight >= 2 && canForward >= 2 && canLeft >= 2)
		{
			M5.Lcd.println("Moving BackWard");
			Serial.println("Moving BackWard");
			_retTilesAdd(_nowRobotPosition);

			// さっき坂を上ったので、今坂の上、あるいはバンプの上にいる可能性が高い。その状態で先に回ると大事件になるので順序を逆に。
			// 逆に、さっき上っていなかった場合は、戻った先にバンプがある可能性もあるので先に回ってから戻る
			if (isprevclimb)
			{
				SpecialMoving = true;
				isprevclimb = false;
				MoveForward(-(TILE_LENGTH_CM + 5.0));
				RotateRobot(180);
			}
			else
			{
				RotateRobot(180);
				MoveForward(TILE_LENGTH_CM + 5.0);
			}
		}
		else // どこかしらいけるときは、いけるけど行ってなかったマスを探す
		{
			vector<Point> unevec = MappingManager::FindUnesxplored();
			if (unevec.size() == 0)
			{
				BothPrintln("Exploring has done. Now returns.");

				// 帰還時の最適なルートを検索
				Route_v2 backRoute = RouteManager::Pos2Pos(_nowRobotPosition, Point{0, 0});

				// ルートを実際の動きに翻訳
				Route2Move(backRoute);
			}
			else
			{
				BothPrintln("looks for new routes");
				// 動作的に一番後ろにあるものが、自身がいるところから一番近い
				Route_v2 nr = RouteManager::Pos2Pos(_nowRobotPosition, MappingManager::GetUnreachedAndDelete());
				for (int i = 0; i < nr.size(); i++)
				{
					Serial.println(p2str(nr.at(i)).c_str());
				}
				Route2Move(nr);
			}
		}
	}
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

/// @brief 角度の補正
void MachineManager::DirCorrection()
{
	bool f1noerr = true, f2noerr = true;
	// 前についている２つのセンサーの距離の差から角度を出す
	int f1 = (int)ToFManager::GetDistance(ToFAngle::Forward, 10, &f1noerr);
	int f2 = (int)ToFManager::GetDistance(ToFAngle::Forward2, 10, &f2noerr);

	Serial.printf("f1: %d, f2: %d\n", f1, f2);

	// 遠すぎると正確な値が返ってこない、また計測中にエラーがあったらスキップ
	if (f1 > 200 || f2 > 200 || !f1noerr || !f2noerr)
		return;
	M5.Lcd.println(abs(f1 - f2));

	double gap = atan((f1 - f2) / (TOF_INTERVAL * 10.0)) * 180.0 / PI;

	Serial.printf("dir is : %lf\n", gap);

	// 機体が閾値以上傾いていたら補正
	if (abs(gap) > ANGLE_CORRECTION_THRESHOLD)
	{
		// 45度未満だったら補正
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

	// モータの向き
	int8_t sign = cm > 0 ? -1 : 1;

	// 動く距離(cm)を角度に変換したときの大きさ
	double movecm = 180.0 * cm / (PI * 5.5 / 2) * MOTOR_CONSTANT;

	bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, MOTOR_POWER_MOVING * sign);
	bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, MOTOR_POWER_MOVING * sign);

	for (int i = 0; i < 10; i++)
	{
		tcs[0].TCS_read();
	}

	// 指定された長さ動くまで待機、黒いマスを見つけたら即座に後退する
	while (abs(bx.GetEncoderValue(BASE_X_TIRE_PORT_LEFT)) < abs(movecm))
	{
		// 被災者検知。色センサーと温度センサーを読み込む
		// TODO 色の閾値設定
		for (int tcsnum = 0; tcsnum < 1; tcsnum++)
		{
			tcs[tcsnum + 1].TCS_read();

			// 赤か黄色か緑の被災者を発見したら
			if (tcs[tcsnum + 1].Red > 1000 ||
				(tcs[tcsnum + 1].Red > 300 && tcs[tcsnum + 1].Green > 300 && tcs[tcsnum + 1].Blue < 100))
			{
				victimInNeedFound = true;
			}
			else if (tcs[tcsnum + 1].Green > 1000)
			{
				safeVictimFound = true;
			}
		}

		{
			// 温度を発する被災者を検知
			double temp1 = mlx1.readObjectTempC();
			double temp2 = mlx2.readObjectTempC();
			if (temp1 == NAN || temp2 == NAN)
			{
			}
			else
			{
				if (28.0 < temp1 && temp1 < 40.0 && temp1 > floorTemp + 7.0)
				{
					victimInNeedFound = true;
				}
				if (28.0 < temp2 && temp2 < 40.0 && temp2 > floorTemp + 7.0)
				{
					victimInNeedFound = true;
				}
			}
		}

		tcs[0]
			.TCS_read();
		// 黒に踏み込んだら
		if (tcs[0].Clear < 500)
		{
			M5.Lcd.println("Black Floor detected!!!");
			_motor_off();
			delay(20);

			// 後退してるときに黒に踏み入れることはない…よね?
			bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, 80 * -sign);
			bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, 80 * -sign);

			while (bx.GetEncoderValue(BASE_X_TIRE_PORT_LEFT) < 0)
			{
				delay(1);
			}
			_motor_off();
			delay(10);

			Point bp = _dir2p(_nowRobotPosition, NowFacingAbs);

			// 立ち入り禁止ゾーンは全方位壁があるとみなす
			TileInfo tl{bp, Walls{Wall::WallExists, Wall::WallExists, Wall::WallExists, Wall::WallExists}, {Obstacle::Nothing}, FloorType::KeepOut};
			MappingManager::PointAdd(tl);

			ResetEncoder();
			delay(10);
			return;
		}

		// ジャイロセンサを監視し、機体が傾いているかを見る。戻る動作時に段差に反応されても困るのでロックをかける
		// Y軸を監視する
		if (!SpecialMoving)
		{
			if ((mpu->gyro[1][1]) > 5.0)
			{
				Serial.printf("saka detected, %d\n", timer);
				isprevclimb = true;
				M5.Lcd.println("slope detected");
				M5.Lcd.clear();
				M5.Lcd.setCursor(0, 0);
			}
			// 登っているときにジャイロの傾きが負になったらバンプ
			else if (mpu->gyro[1][1] < -0.3)
			{
				bumpclimbed = true;
				Serial.println("bump detected!!");
			}
		}

		delay(1);
	}

	_motor_off();

	// 角度を補正する
	DirCorrection();

	ResetEncoder();

	delay(10);

	// 新しい場所に移動したら更新
	if (cm > 0)
	{
		_nowRobotPosition = _dir2p(_nowRobotPosition, NowFacingAbs);
	}
	else
	{ // ロボットをそのまま後退させたときは進んだ向きが逆になる
		_nowRobotPosition = _dir2p(_nowRobotPosition, (Direction)(((int)NowFacingAbs + 2) % 4));
	}
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

/// @brief 機体を回転させる
/// @param ang 自身の方向からどちらに回転させるか
void MachineManager::RotateRobot(double ang)
{
	if (ang == 0)
		return;

	mpu->_clear();
	delay(5);

	if (ang > 0) // 左
	{
		bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, 60);
		bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, -60);
	}
	if (ang < 0) // 右
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

	// 方角の変換
	if ((ang > 89.0 && ang < 91.0) || (ang < -89.0 && ang > -91.0))
	{
		NowFacingAbs = _a2dir(ang > 0 ? Angle::Left : Angle::Right);
	}
	// TODO ちゃんと逆転してるかな?
	else if (abs(ang) > 179.0 && abs(ang) < 181.0)
	{
		NowFacingAbs = Direction(((int)NowFacingAbs + 2) % 4);
	}

	delay(10);
	ResetEncoder();
	mpu->_clear();
	delay(5);
}

/// @brief 最初の一回。どの向きに進んでいくかを決める
void MachineManager::Initialize()
{

	// looptaskを保存、緊急停止時に強制的にsuspendedにさせるため
	taskvec.push_back(Task{.task = xTaskGetHandle(LOOPTASK_NAME), .name = LOOPTASK_NAME});

	if (taskvec[taskvec.size() - 1].task == NULL)
	{
		Serial.println("fatal: loop task does not found!!!");
	}

	semaphore = xSemaphoreCreateBinary();

	// セマフォは初期動作がなぜか不安定で取得してもしたことにならず、そのせいでそのセマフォが一生使えなくなるという珍事が発生するので一度取得して手放す
	BaseType_t xsematama = xSemaphoreTake(semaphore, 20);
	xSemaphoreGive(semaphore);

	Serial.println("Sensor checking start...");

	// TCS(色センサ)のチェック
	for (int i = 0; i < 1; i++)
	{
		I2CAddressChangerManager::ChangeAddress((unsigned char)i + 3);
		Wire.beginTransmission(0x29);
		if (Wire.endTransmission() != 0)
		{
			Serial.printf("TCS color sensor %d does not found!!\n", i + 3);
			M5.Lcd.printf("TCS color sensor %d does not found!!\n", i + 3);
		}
		else
		{
			TCSManager ttcs(i + 3);
			tcs[i + 1] = ttcs;
		}
	}
	I2CAddressChangerManager::ChangeAddress(0);
	Wire.beginTransmission(0x29);
	if (Wire.endTransmission() != 0)
	{
		BothPrintln("TCS color sensor FLOOR does not found!!");
	}
	else
	{
		TCSManager ttcs(0);
		tcs[0] = ttcs;
	}
	// TCS(色センサ)の初期設定
	M5.Lcd.println("TCS initialize finished.");

	// 6軸センサのチェック
	Wire.beginTransmission(0x68);
	if (Wire.endTransmission() != 0)
	{
		BothPrintln("MPU6050 6 axises sensor does not found!!");
	}

	// PCAのチェック
	Wire.beginTransmission(0x77);
	if (Wire.endTransmission() != 0)
	{
		BothPrintln("PCA multi connector does not found!!");
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

	// 温度センサのチェック
	Wire.beginTransmission(0x5A);
	if (Wire.endTransmission() != 0)
	{
		BothPrintln("Temp sensor 1 does not found!!");
	}
	Wire.beginTransmission(0x55);
	if (Wire.endTransmission() != 0)
	{
		BothPrintln("Temp sensor 2 does not found!!");
	}

	Serial.println("Sensor checking finished");

	// BaseXの初期設定
	for (uint8_t i = 1; i <= 4; i++)
	{
		bx.SetMode(i, NORMAL_MODE);
	}

	// GP906(温度センサ)の初期設定
	mlx1.begin();
	mlx2.begin(0x55);

	// 部屋の温度を取得する。正確な値を出すために20回やって平均をとる
	int i = 0;
	while (i < 20)
	{
		double tmptmp = mlx1.readAmbientTempC();
		if (tmptmp == NAN)
		{
		}
		else
		{
			floorTemp += tmptmp;
			i++;
		}
	}
	floorTemp /= 20.0;

	M5.Lcd.println("GY-906 initialize finished.");

	return;
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

// チェックポイントからやり直すときに表示する画面とその設定
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

	// まずチェックポイントを読み出す
	for (TileInfo ti : tiles)
	{
		if (ti.fp == FloorType::CheckPoint)
		{
			chkpvec.push_back(ti);
		}
	}

	M5.Lcd.setCursor(ALIGN + 25, ALIGN);
	M5.Lcd.setTextSize(FONT_SIZE);

	for (int i = 0; i < chkpvec.size(); i++)
	{
		M5.Lcd.print("( ");
		M5.Lcd.print(chkpvec[i]._p.x);
		M5.Lcd.print(", ");
		M5.Lcd.print(chkpvec[i]._p.y);
		M5.Lcd.print(" )");
		// 次の座標に移動(25 pixel 進める)
		M5.Lcd.setCursor(ALIGN + 25, (i + 1) * 25 + ALIGN);
	}
	// カーソルの描画
	M5.Lcd.setCursor(CURSOR_ALIGN, ALIGN);
	M5.Lcd.print(">");
	// ボタンの入力待ち
	while (true)
	{
		M5.update();
		if (M5.BtnA.wasPressed())
		{
			if (selectionid > 0)
			{
				selectionid -= 1;
				// 一度塗りつぶしてカーソルを上書き
				M5.Lcd.fillRect(0, 0, ALIGN, 600, 0);
				M5.Lcd.setCursor(CURSOR_ALIGN, (selectionid)*25 + ALIGN);
				M5.Lcd.print(">");
			}
		}
		if (M5.BtnC.wasPressed())
		{
			if (selectionid < chkpvec.size() - 1)
			{
				// 一度塗りつぶしてカーソルを上書き
				selectionid += 1;
				M5.Lcd.fillRect(0, 0, ALIGN, 600, 0);
				M5.Lcd.setCursor(CURSOR_ALIGN, (selectionid)*25 + ALIGN);
				M5.Lcd.print(">");
			}
		}
		if (M5.BtnB.wasPressed())
		{
			// リスタート地点を設定
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

/// @brief 方向から座標を返す
/// @param ang
/// @return その向きの座標
Point MachineManager::_a2p(Angle ang)
{
	return _dir2p(_nowRobotPosition, _a2dir(ang));
}

/// @brief ルートから実際の動きに翻訳する
/// @param r
void MachineManager::Route2Move(Route_v2 r)
{
	for (int i = 0; i < r.size() - 1; i++)
	{

		// 左に行く
		//  TODO 要デバッグ
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

/// @brief レスキューキットを吐き出す
void MachineManager::DispenseRescueKit()
{
	// TODO レスキューキット排出機構作ろう
	M5.Lcd.println("NAGEMA-------------------SU");
	// delay(1000);
	bx.SetMotorSpeed(3, 100);
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

uint16_t MachineManager::Readtmp(int i, int n)
{
	return ToFManager::GetDistance((ToFAngle)i, n);
}

void MachineManager::MoveUntilFlat()
{
	BothPrintln("Climbing slope");

	// TODO 符号逆?
	bx.SetMotorSpeed(BASE_X_TIRE_PORT_LEFT, -MOTOR_POWER_MOVING);
	bx.SetMotorSpeed(BASE_X_TIRE_PORT_RIGHT, -MOTOR_POWER_MOVING);

	while (abs(mpu->gyro[1][1]) > 4.0)
	{
		delay(1);
	}

	// 登り切ったらもう少し進める
	delay(220);

	_motor_off();
	ResetEncoder();
}

size_t MachineManager::BothPrintln(const char *text)
{
#if DEBUGGING_MODE
	Serial.println(text);
#endif
	return M5.Lcd.println(text);
}

void MachineManager::FlashLED(int ms)
{
	int i = 0;
	while (i < ms)
	{
		digitalWrite(LED_PIN, HIGH);
		delay(200);
		digitalWrite(LED_PIN, LOW);
		delay(200);
		i++;
	}
}