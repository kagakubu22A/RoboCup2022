#include <Arduino.h>
#include <Wire.h>

#include "MPU6050.h"

MPU6050::MPU6050(uint8_t filterStrength, bool addressSelect)
{
	_filterStrength = filterStrength;
	if (!addressSelect)
		address = 0x68;
	else
		address = 0x69;
}

void MPU6050::changeResolution(bool sensorType, uint16_t resolution)
{
	if (!sensorType)
	{
		if ((resolution == 2) || (resolution == 4) || (resolution == 8) || (resolution == 16))
		{
			_resolution[0] = resolution / 32767;
			if (resolution == 2)
				_initValue[2] = 0x00;
			else if (resolution == 4)
				_initValue[2] = 0x08;
			else if (resolution == 8)
				_initValue[2] = 0x10;
			else if (resolution == 16)
				_initValue[2] = 0x18;
		}
	}
	else
	{
		if ((resolution == 250) || (resolution == 500) || (resolution == 1000) || (resolution == 2000))
		{
			_resolution[1] = resolution / 32767;
			if (resolution == 250)
				_initValue[1] = 0x00;
			else if (resolution == 500)
				_initValue[1] = 0x08;
			else if (resolution == 1000)
				_initValue[1] = 0x10;
			else if (resolution == 2000)
				_initValue[1] = 0x18;
		}
	}
}

void MPU6050::changeDeadzone(bool sensorType, double deadzone)
{
	if (!sensorType)
		_deadzone[ACCEL] = deadzone;
	else
		_deadzone[GYRO] = deadzone;
}

void MPU6050::changeFilterStrength(uint8_t filterStrength)
{
	_filterStrength = filterStrength;
}

void MPU6050::switchIntegration(bool isIntegrating)
{
	_isIntegrating = isIntegrating;
}

void MPU6050::init(TwoWire *wire)
{
	_wire = wire;
	i2c.init(_wire);
	i2c.waitUntilExist(address);
	for (uint8_t counter = 0; counter < 5; counter++)
	{
		i2c.write(address, _initResister[counter], _initValue[counter]);
	}
	resetRequest = true;
}

void MPU6050::_getData()
{
	for (uint8_t counter = 0; counter < 6; counter++)
	{
		_raw[counter] = i2c.readS16(address, _resisterToRead[counter]);
	}
}

void MPU6050::_adjust()
{
	for (uint8_t counter = 0; counter < 6; counter++)
	{
		_valueDifferenceSum[counter] = 0;
	}
	for (uint8_t counter = 0; counter < 100; counter++)
	{
		//Serial.println("get data start");
		_getData();
		//Serial.println("get data end");
		for (uint8_t counter2 = 0; counter2 < 6; counter2++)
		{
			_valueDifferenceSum[counter2] += _raw[counter2];
		}
	}
	for (uint8_t counter = 0; counter < 6; counter++)
	{
		_valueDifference[counter] = _valueDifferenceSum[counter] / 100.;
	}
}

void MPU6050::_filter()
{
	for (uint8_t counter = 0; counter < 6; counter++)
	{
		_filteredValueSum[counter] = 0;
	}
	_filterNumber++;
	if (_filterNumber >= _filterStrength)
	{
		_filterNumber = 0;
	}
	for (uint8_t counter = 0; counter < 6; counter++)
	{
		_filterStorage[counter][_filterNumber] = _raw[counter] - _valueDifference[counter];
	}
	for (uint8_t counter = 0; counter < 6; counter++)
	{
		for (uint8_t counter1 = 0; counter1 < _filterStrength; counter1++)
		{
			_filteredValueSum[counter] += _filterStorage[counter][counter1];
		}
		_filteredValue[counter] = _filteredValueSum[counter] / _filterStrength;
	}
}

void MPU6050::_convert()
{
	for (uint8_t counter = 0; counter < 3; counter++)
	{
		if (abs(_filteredValue[counter] * G * _resolution[0]) >= _deadzone[0])
		{
			accel[counter][M_S2] = _filteredValue[counter] * G * _resolution[0];
		}
		else
		{
			accel[counter][M_S2] = 0.0;
		}
	}
	for (uint8_t counter = 0; counter < 3; counter++)
	{
		if (abs(_filteredValue[counter + 3] * _resolution[1]) >= _deadzone[1])
		{
			gyro[counter][DEG_S] = _filteredValue[counter + 3] * _resolution[1];
		}
		else
		{
			gyro[counter][DEG_S] = 0.0;
		}
	}
	accel[Z][M_S2] += G;

	polar[0] = sqrt(pow(accel[X][M_S2], 2) + pow(accel[Y][M_S2], 2) + pow(accel[Z][M_S2], 2));
	polar[1] = atan(accel[Y][M_S2] / accel[X][M_S2]) * 180 / PI;
	polar[2] = atan(accel[Z][M_S2] / sqrt(pow(accel[X][M_S2], 2) + pow(accel[Y][M_S2], 2))) * 180 / PI;
}

void MPU6050::_integrate()
{
	_integralTime = (double)micros() / 1000000.0 - _timeDifference;
	if (_isIntegrating)
	{
		for (uint8_t counter = 0; counter < 3; counter++)
		{
			accel[counter][1] += accel[counter][0] * _integralTime;
			accel[counter][2] += accel[counter][1] * _integralTime;
			gyro[counter][1] += gyro[counter][0] * _integralTime;
		}
	}
	_timeDifference = (double)micros() / 1000000.0;
}

void MPU6050::_reset()
{
	resetingFlag = true;

	_clear();
	_adjust();
	//Serial.println("Adjust ended");
	_integralTime = millis() / 1000.0 - _timeDifference;
	_timeDifference = millis() / 1000.0;
	resetRequest = false;

	resetingFlag = false;
}

void MPU6050::read()
{
	//Serial.println("read from read");
	if (resetRequest == true)
		_reset();
	_getData();
	_filter();
	_convert();
	_integrate();
	//Serial.println("Read finished");
}

void MPU6050::_clear()
{
	for (uint8_t counter = 0; counter < 3; counter++)
	{
		for (uint8_t counter1 = 0; counter1 < 3; counter1++)
		{
			accel[counter][counter1] = 0.0;
		}
		for (uint8_t counter1 = 0; counter1 < 2; counter1++)
		{
			gyro[counter][counter1] = 0.0;
		}
	}
}