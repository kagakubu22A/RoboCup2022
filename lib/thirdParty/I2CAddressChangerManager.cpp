#include "I2CAddressChangerManager.h"
#include <Wire.h>
#include <M5Stack.h>

void I2CAddressChangerManager::ChangeAddress(unsigned char channel)
{
	Wire.beginTransmission(PCA_ADDRESS);
	Wire.write((channel & 0x07) | 0x08);
	Wire.endTransmission();
}