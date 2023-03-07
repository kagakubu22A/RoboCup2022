#include "ToFManager.h"
#include "M5Stack.h"
#include "I2CAddressChangerManager.h"
#include "vector"
#include <numeric>
#include "MachineManager.h"

using std::vector;

/// @brief 距離をミリ単位で取得する
/// @param ang 方向
/// @param n 計測回数、正確性を確保するために多いほうが良い
/// @return 距離(mm)
uint16_t ToFManager::GetDistance(ToFAngle ang, int n)
{
	// セマフォをとる
	for (int i = 0; i < 5; i++)
	{
		if (xSemaphoreTake(MachineManager::semaphore, 2000) == pdTRUE)
		{
			break;
		}
		else
		{
			if (i == 4)
			{

				Serial.println("Cant get distance because of semaphore!!");
				break;
			}
			Serial.println("did not get semaphore in time. retrying...");
		}
	}

	I2CAddressChangerManager::ChangeAddress((unsigned char)ang);
	delay(30);

	// 先に何回か読む
	for (int i = 0; i < 5; i++)
	{
		Wire.beginTransmission(TOF_ADDRESS);
		Wire.write(0xD3);
		Wire.endTransmission();

		Wire.requestFrom(TOF_ADDRESS, 2);
		uint16_t tmp = Wire.read() << 8 | Wire.read();

		delay(6);

		// Tofは計測するごとに34msのクールタイムが必要
		/*delay(34);
		Wire.beginTransmission(TOF_ADDRESS);
		Wire.write(0xF5);
		Wire.endTransmission();
		delay(34);*/
	}

	int sum = 0;

	int amari = 0;

	for (int i = 0; i < n + amari; i++)
	{
		Wire.beginTransmission(TOF_ADDRESS);
		Wire.write(0xD3);
		Wire.endTransmission();

		Wire.requestFrom(TOF_ADDRESS, 2);

		uint16_t tmp = Wire.read() << 8 | Wire.read();

		if (tmp == 8888 || tmp == 0b1111111111111111 || tmp == 0)
		{
			M5.Lcd.printf("%u detected!! i is %d\n", tmp, i);
			Serial.printf("%u detected!! i is %d\n", tmp, i);
			if (tmp == 8888)
				amari++;

			// はずれが5回以上出たら壁はないものとして返す
			if (amari >= 5)
			{
				M5.Lcd.printf("too many errors detected! angle is %d\n", ang);
				Serial.printf("too many errors detected! angle is %d\n", ang);
				xSemaphoreGive(MachineManager::semaphore);
				return 10000;
			}
			continue;
		}

		sum += (int)tmp;

		delay(6);
	}

	xSemaphoreGive(MachineManager::semaphore);
	return (int)round((double)sum / n);
}
