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
	while (1)
	{
		if (xSemaphoreTake(MachineManager::semaphore, 20) == pdTRUE)
		{
			mpu6050.read();
			//Serial.println("mpu read");

			for (int i = 0; i < GYRO_SAVENUM - 1; i++)
			{
				MachineManager::gyro_y[i] = MachineManager::gyro_y[i + 1];
			}
			MachineManager::gyro_y[GYRO_SAVENUM - 1] = mpu6050.gyro[1][1];

			//セマフォ返却
			xSemaphoreGive(MachineManager::semaphore);
		}else{
			//Serial.println("semaphore cant get in sensorread()");
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
	M5.begin(true, false, true, true);
	M5.Power.begin();

	Serial.begin(9600);
	Serial.println();
	// LED点滅用
	pinMode(LED_PIN, OUTPUT);

	// I2C通信をするための初期化
	Wire.begin();

	if (SKIP_SENSORS_CHECK)
	{
		isStarted = true;
		return;
	}

	M5.Lcd.println("M5 started.\nMPU6050 starting...");

	delay(3000);

	// MPU6050の初期化
	mpu6050.init(&Wire);

	// フィルターの強度を設定
	mpu6050.changeDeadzone(ACCEL, 0.2);
	mpu6050.changeDeadzone(GYRO, 0.5);

	mpu6050._reset();

	M5.Lcd.println("mpu6050 setup finished");

	MachineManager::RegisterMPU(&mpu6050);
	MachineManager::Initialize();

	// マルチタスク
	xTaskCreatePinnedToCore(buttonRead, "ReadButton", 4096, NULL, 1, NULL, 0);
	//xTaskCreatePinnedToCore(sensorRead, "SensorRead", 4096, NULL, 2, NULL, 1);

	enableCore1WDT();

	// 初期化終了を通知
	M5.Lcd.println("initialize done");
	delay(250);
	M5.Lcd.clear();
}

void loop()
{
	delay(10);

	// ボタンが押されるまで始まらないように
	if (!isStarted)
	{
		// Serial.println("Returned");
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

	/*MappingManager::DisplayMap();
	delay(10000000);
	return;*/

	/*Serial.printf("dis %d\n", ToFManager::GetDistance(ToFAngle::Right, 50));
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