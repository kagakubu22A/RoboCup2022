#ifndef __TCS_MANAGER_H__
#define __TCS_MANAGER_H__

#include <Arduino.h>
#include <Wire.h>

#define CMD_REPEAT 0x80
#define CMD_INCREMENT 0xA0
#define CMD_ATIME 0x01
#define CMD_AGAIN 0x0F
#define CMD_ENABLE 0x00
#define CMD_APERS 0x0c
#define CMD_AILTL 0x04
#define CMD_AILTH 0x05
#define CMD_AIHTL 0x06
#define CMD_AIHTH 0x07
#define CMD_CDATA 0x14
#define CMD_CLEARINTERRUPT 0x66
#define CMD_AIEN 0x10

class TCSManager
{
private:
private:
public:
    TCSManager(unsigned char channel);
    TCSManager(){}

    uint8_t ATIME = 0xD5;
    uint8_t AGAIN = 0x10;
    uint8_t ENABLE = 0x03;
    unsigned char ch;
    uint16_t Clear, Red, Green, Blue;
    void TCS_read();
};

#endif