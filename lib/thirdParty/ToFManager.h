#ifndef __ToF_MANAGER_H__
#define __ToF_MANAGER_H__
#include <Wire.h>
#include "AngDir.h"

#define TOF_ADDRESS 0x52

enum class ToFAngle{
    Left,
    Forward,
    Right,
    Backward,
    Forward2,
};

class ToFManager{
    private:
    ToFManager();
    private:

    static Angle prevA;

    public:
    static uint16_t GetDistance(ToFAngle ang, int n);
};

#endif