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
	delay(3);

	// 先に何回か読む
	for (int i = 0; i < 5; i++)
	{
		Wire.beginTransmission(TOF_ADDRESS);
		Wire.write(0xD3);
		// Wire.endTransmission(false);
		Wire.endTransmission();

		Wire.requestFrom(TOF_ADDRESS, 2);
		uint16_t tmp = Wire.read() << 8 | Wire.read();

		delay(34);

		// Tofは計測するごとに34msのクールタイムが必要
		/*delay(34);
		Wire.beginTransmission(TOF_ADDRESS);
		Wire.write(0xF5);
		Wire.endTransmission();
		delay(34);*/
	}

	vector<uint16_t> tmpv;

	int sum = 0;

	int amari = 0;

	uint16_t min = 8888, max = 0;

	for (int i = 0; i < n + amari; i++)
	{
		Wire.beginTransmission(TOF_ADDRESS);
		int writecondition = (int)Wire.write(0xD3);

		int endcond = (int)Wire.endTransmission(false);

		int reqcond = (int)Wire.requestFrom(TOF_ADDRESS, 2);

		if (Wire.available() == 0)
		{
			Serial.printf("no data can be received!!! i is %d, dir is %d\n", i, (int)ang);
			Serial.printf("returns:\nWire.write(): %d\nWire.endTransmission(): %d\nWire.requestFrom(): %d\n", writecondition, endcond, reqcond);
			continue;
		}

		uint16_t tmp = Wire.read() << 8 | Wire.read();

		if (tmp > (uint16_t)8888 || tmp == (uint16_t)0 || tmp == (uint16_t)256)
		{
			M5.Lcd.printf("%u detected!! i is %d, dir is %d\n", tmp, i, (int)ang);
			Serial.printf("%u detected!! i is %d, dir is %d\n", tmp, i, (int)ang);

			continue;
		}
		else if (tmp == 8888)
			amari++;

		// はずれが5回以上出たら壁はないものとして返す
		if (amari >= 5)
		{
			xSemaphoreGive(MachineManager::semaphore);
			return 8888;
		}

		if (max < tmp)
		{
			max = tmp;
		}
		if (min > tmp)
		{
			min = tmp;
		}

		sum += (int)tmp;

		tmpv.push_back(tmp);

		delay(34);
	}

	Serial.printf("min: %d,max: %d\n", (int)min, (int)max);
	/*for (int i = 0; i < tmpv.size(); i++)
	{
		Serial.printf("%d,", (int)tmpv[i]);
	}
	Serial.println();*/

	xSemaphoreGive(MachineManager::semaphore);
	return (int)round((double)sum / n);
}
