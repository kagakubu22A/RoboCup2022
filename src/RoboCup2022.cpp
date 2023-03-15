#include <Arduino.h>
#include "M5Stack.h"
#include "Route_v2.h"
#include "BaseX.h"
#include "MachineManager.h"
#include <Wire.h>
#include "ToFManager.h"
#include "TCSManager.h"
#include "MPU6050.h"
#include "I2CAddressChangerManager.h"
#include <numeric>
#include "MappingManager.h"

MPU6050 mpu6050(10);

volatile bool isStarted = false;

void buttonRead(void *arg)
{
	bool isButtonRead = false;
	while (true)
	{
		M5.update();
		isButtonRead = M5.BtnB.wasPressed();

		if (isStarted && isButtonRead)
		{
			// looptaskをサスペンドする
			// TODO 果たしてsuspendでいいのか?
			for (int i = 0; i < MachineManager::taskvec.size(); i++)
			{
				if (MachineManager::taskvec[i].name == LOOPTASK_NAME)
				{
					vTaskSuspend(MachineManager::taskvec[i].task);
				}
			}
			isStarted = false;
			Serial.println("Robot stopped");
			delay(100);
			MachineManager::ForceStop = true;
			MachineManager::DisplayRetry();

			mpu6050._reset();
			M5.Lcd.setCursor(0, 0);
			delay(1500);
			isStarted = true;
			continue;
		}
		if (!isStarted && isButtonRead)
		{
			M5.Lcd.clear();

			M5.Lcd.println("Reset finished. Now Exploring starts.");
			Serial.println("Reset finished. Now Exploring starts.");
			M5.Lcd.setCursor(0, 0);
			isStarted = true;
			delay(2000);
			continue;
		}
		// vTaskDelay(1);
		delay(1);
	}
}

/// @brief センサーを読み、ジャイロのY軸に関しては坂道を検知するために過去5回のデータを保存しておく
/// @param arg
void sensorRead(void *arg)
{
	static int i = 0;
	while (1)
	{
		if (xSemaphoreTake(MachineManager::semaphore, 20) == pdTRUE)
		{
			I2CAddressChangerManager::ChangeAddress(5);
			mpu6050.read();
			// Serial.printf("mpu read, i = %d\n", i++);
			// TCSManager::TCS_read();

			for (int i = 0; i < GYRO_SAVENUM - 1; i++)
			{
				MachineManager::gyro_y[i] = MachineManager::gyro_y[i + 1];
			}
			MachineManager::gyro_y[GYRO_SAVENUM - 1] = mpu6050.gyro[1][1];

			// セマフォ返却
			xSemaphoreGive(MachineManager::semaphore);
		}
		else
		{
			// Serial.println("semaphore cant get in sensorread()");
		}
		// TCSManager::TCS_read();
		//  vTaskDelay(1);
		delay(1);
		// Serial.print("read end");
	}
}

void setup()
{
	// M5Stackの初期化
	M5.begin();

	// I2C通信をするための初期化 M5.begin()の引数にi2cを有効化するかどうかがあるがそこでやると高確率でエラー吐くようになった
	Wire.begin();

	// シリアル通信開始、はじめはノイズがあるので改行して見やすく
	Serial.begin(9600);
	Serial.println();

	// LED点滅用
	pinMode(LED_PIN, OUTPUT);

	if (SKIP_SENSORS_CHECK)
	{
		isStarted = true;
		return;
	}

	M5.Lcd.println("M5 started.\nMPU6050 starting...");

	delay(3000);

	if (true)
	{
		I2CAddressChangerManager::ChangeAddress(5);

		// MPU6050の初期化
		mpu6050.init(&Wire);

		// フィルターの強度を設定
		mpu6050.changeDeadzone(ACCEL, 0.2);
		mpu6050.changeDeadzone(GYRO, 0.5);

		mpu6050._reset();

		M5.Lcd.println("mpu6050 setup finished");
	}

	MachineManager::RegisterMPU(&mpu6050);
	MachineManager::Initialize();

	// いくつタスクを追加するか
	const int taskaddnum = 2;

	vector<TaskHandle_t> tmptasks{taskaddnum};

	// マルチタスク
	xTaskCreatePinnedToCore(buttonRead, BUTTON_READ_TASK_NAME, 4096, NULL, 1, &tmptasks[0], 1);
	xTaskCreatePinnedToCore(sensorRead, SENSOR_READ_TASK_NAME, 4096, NULL, 5, &tmptasks[1], 1);

	MachineManager::taskvec.push_back(MachineManager::Task{.task = tmptasks[0], .name = BUTTON_READ_TASK_NAME});
	MachineManager::taskvec.push_back(MachineManager::Task{.task = tmptasks[1], .name = SENSOR_READ_TASK_NAME});

	// 初期化終了を通知
	M5.Lcd.println("initialize done");
	delay(250);
	M5.Lcd.clear();
}

void loop()
{
	// ボタンが押されるまで始まらないように
	if (!isStarted)
	{
		// M5.Lcd.printf("gyro: %lf", mpu6050.gyro[1][1]);
		return;
	}

	M5.Lcd.clear();
	M5.Lcd.setCursor(0, 0);

	/*
		M5.Lcd.println(mpu6050.gyro[1][1]);
		return;
	*/
	/*TCSManager::TCS_read();
	M5.Lcd.println(TCSManager::Clear);
	if(TCSManager::Clear<200){
		M5.Lcd.println("Black detected");
	}
	return;
	*/

	/*for (int i = 0; i < 5; i++)
	{
		Serial.printf("direction : %d, d = %d\n", i, MachineManager::Readtmp(i, 35));
		delay(10);
	}
	// Serial.printf("gyro: %lf\n", mpu6050.gyro[1][1]);
	return;*/

	// これより下に常時動かすコードを書くこと
	if (!MachineManager::ForceStop)
	{
		// TODO マッピング機能の充実、右手法の改良
		M5.Lcd.clear();
		M5.Lcd.setCursor(0, 0);
		MachineManager::Move1Tile();
	}
}