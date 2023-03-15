#include "i2c_ReadWrite.h"
#include <M5Stack.h>

i2c_ReadWrite::i2c_ReadWrite() {}

void i2c_ReadWrite::init(TwoWire *wire)
{
	_wire = wire;
}

bool i2c_ReadWrite::exist(uint8_t addr)
{
	bool found;
	_wire->beginTransmission(addr);
	if (!_wire->endTransmission())
		found = true;
	else
	{
		found = false;
		Serial.printf("Address 0x%x is not found!\n", addr);
		delay(100);
	}
	return found;
}

void i2c_ReadWrite::waitUntilExist(uint8_t addr)
{
	bool wait = false;
	while (!exist(addr))
	{
		wait = true;
	}
	if (wait)
		delay(100);
}

void i2c_ReadWrite::scan()
{
	byte error, address;
	int nDevices;

	nDevices = 0;
	for (address = 1; address < 127; address++)
	{
		_wire->beginTransmission(address);
		error = _wire->endTransmission();

		if (error == 0)
		{
			Serial.print("I2C device found at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.print(address, HEX);
			Serial.println("  !");

			nDevices++;
		}
		else if (error == 4)
		{
			Serial.print("Unknown error at address 0x");
			if (address < 16)
				Serial.print("0");
			Serial.println(address, HEX);
		}
	}
	if (nDevices == 0)
		Serial.println("No I2C devices found\n");
	else
		Serial.println("done\n");
}

uint8_t i2c_ReadWrite::write(uint8_t addr, uint8_t reg, uint8_t val)
{
	uint8_t communication;
	_wire->beginTransmission(addr);
	_wire->write(reg);
	_wire->write(val);
	communication = _wire->endTransmission();
	if (communication != 0)
		Serial.printf("Communication error has occured at address 0x%x!\n", addr);
	return communication;
}

uint8_t i2c_ReadWrite::write(uint8_t addr, uint8_t val)
{
	uint8_t communication;
	_wire->beginTransmission(addr);
	_wire->write(val);
	communication = _wire->endTransmission();
	if (communication != 0)
		Serial.printf("Communication error has occured at address 0x%x!\n", addr);
	return communication;
}

bool i2c_ReadWrite::read(uint8_t addr, uint8_t reg, uint8_t num)
{
	bool communication;
	_wire->beginTransmission(addr);
	_wire->write(reg);
	//2023 03 10 引数なしから引数をfalseに変更
	_wire->endTransmission(false);
	_wire->requestFrom(addr, num);
	uint8_t counter = 0;
	if (_wire->available() == num)
	{
		while (_wire->available())
		{
			data[counter] = _wire->read();
			counter++;
		}
		communication = 0;
	}
	else
	{
		Serial.printf("Communication error has occured at address 0x%x!\n", addr);
		Serial.printf("Expected is %d, real is %d\n", num, _wire->available());
		communication = 1;
	}
	return communication;
}

uint8_t i2c_ReadWrite::readU8(uint8_t addr, uint8_t reg, bool ord)
{
	read(addr, reg, 1);
	uint8_t value;
	if (ord)
		value = data[0];
	else
		value = data[0];
	return value;
}

uint16_t i2c_ReadWrite::readU16(uint8_t addr, uint8_t reg, bool ord)
{
	read(addr, reg, 2);
	uint16_t value;
	if (ord)
		value = data[0] << 8 | data[1];
	else
		value = data[0] | data[1] << 8;
	return value;
}

uint32_t i2c_ReadWrite::readU32(uint8_t addr, uint8_t reg, bool ord)
{
	read(addr, reg, 4);
	uint32_t value;
	if (ord)
		value = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
	else
		value = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24;
	return value;
}

uint64_t i2c_ReadWrite::readU64(uint8_t addr, uint8_t reg, bool ord)
{
	read(addr, reg, 8);
	uint64_t value;
	if (ord)
		value = data[0] << 56 | data[1] << 48 | data[2] << 40 | data[3] << 32 | data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
	else
		value = data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24 | data[4] << 32 | data[5] << 40 | data[6] << 48 | data[7] << 56;
	return value;
}

int8_t i2c_ReadWrite::readS8(uint8_t addr, uint8_t reg, bool ord)
{
	return readU8(addr, reg, ord);
}

int16_t i2c_ReadWrite::readS16(uint8_t addr, uint8_t reg, bool ord)
{
	return readU16(addr, reg, ord);
}

int32_t i2c_ReadWrite::readS32(uint8_t addr, uint8_t reg, bool ord)
{
	return readU32(addr, reg, ord);
}

int64_t i2c_ReadWrite::readS64(uint8_t addr, uint8_t reg, bool ord)
{
	return readU64(addr, reg, ord);
}