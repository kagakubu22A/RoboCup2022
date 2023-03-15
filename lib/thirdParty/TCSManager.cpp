#include "TCSManager.h"
#include <M5Stack.h>

uint8_t TCSManager::ATIME = 0xD5;
uint8_t TCSManager::AGAIN = 0x10;
uint8_t TCSManager::ENABLE = 0x03;
uint16_t TCSManager::Clear, TCSManager::Red, TCSManager::Green, TCSManager::Blue;

void TCSManager::TCS_begin()
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
}

void TCSManager::TCS_read()
{
    Wire.beginTransmission(0x29);
    Wire.write(CMD_INCREMENT | CMD_CDATA);
    Wire.endTransmission(false);
    Wire.requestFrom(0x29, 8);
    //while (Wire.available() < 8)
    //{
    //    delay(10);
    //    M5.Lcd.println("No data sent from TCS.");
    //}

    Clear = Wire.read() | Wire.read() << 8;
    Red = Wire.read() | Wire.read() << 8;
    Green = Wire.read() | Wire.read() << 8;
    Blue = Wire.read() | Wire.read() << 8;
}