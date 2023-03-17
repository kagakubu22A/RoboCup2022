#include "I2CAddressChangerManager.h"
#include <Wire.h>
#include <M5Stack.h>
#include "MachineManager.h"

void I2CAddressChangerManager::ChangeAddress(unsigned char channel)
{
	Wire.beginTransmission(PCA_ADDRESS);
	Wire.write((channel & 0x07) | 0x08);
	Wire.endTransmission(true);
	// Serial.printf("address changed to %d\n", channel);
}

bool I2CAddressChangerManager::TakeSemaphoreAndChangeAddress(unsigned char channel, int waitms)
{
	if (xSemaphoreTake(MachineManager::semaphore, waitms) == pdTRUE)
	{
		ChangeAddress(channel);
		return true;
	}
	else
	{
		// Serial.printf("can't get semaphore in time!! address is %d\n", (int)channel);
		return false;
	}
}

void I2CAddressChangerManager::ReleaseSemaphore()
{
	xSemaphoreGive(MachineManager::semaphore);
}