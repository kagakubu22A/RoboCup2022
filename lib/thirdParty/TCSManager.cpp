#include "TCSManager.h"
#include <M5Stack.h>

#include "I2CAddressChangerManager.h"

TCSManager::TCSManager(unsigned char channel)
{
    ch = channel;
    // PCAなしで使うときはこのかっこを丸ごと削除すればよい
    if (I2CAddressChangerManager::TakeSemaphoreAndChangeAddress(channel, 20))
    {
        Wire.beginTransmission(0x29);
        Wire.write(CMD_REPEAT | CMD_ATIME);
        Wire.write(ATIME);
        Wire.endTransmission();
        Wire.beginTransmission(0x29);
        Wire.write(CMD_REPEAT | CMD_AGAIN);
        Wire.write(AGAIN);
        Wire.endTransmission();
        Wire.beginTransmission(0x29);
        Wire.write(CMD_REPEAT | CMD_APERS);
        Wire.write(0x01);
        Wire.endTransmission();
        Wire.beginTransmission(0x29);
        Wire.write(CMD_REPEAT | CMD_ENABLE);
        Wire.write(ENABLE | CMD_AIEN);
        Wire.endTransmission();
        delay(3);
        I2CAddressChangerManager::ReleaseSemaphore();
    }
}

void TCSManager::TCS_read()
{
    if (I2CAddressChangerManager::TakeSemaphoreAndChangeAddress(ch, 20))
    {
        Wire.beginTransmission(0x29);
        Wire.write(CMD_INCREMENT | CMD_CDATA);
        Wire.endTransmission(false);
        Wire.requestFrom(0x29, 8);

        Clear = Wire.read() | Wire.read() << 8;
        Red = Wire.read() | Wire.read() << 8;
        Green = Wire.read() | Wire.read() << 8;
        Blue = Wire.read() | Wire.read() << 8;

        I2CAddressChangerManager::ReleaseSemaphore();
    }
}