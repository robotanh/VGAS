#ifndef _ICTPULSE_H_
#define _ICTPULSE_H_
#include "validatorHelper.h"
class ICTPULSE_BUS
{
private:
    int16_t enPin;
    int16_t pulsePin;

public:
    ICTPULSE_BUS(int16_t enPin,int16_t pulsePin,HardwareSerial *uart);
    void busEnable();
    void busDisable();
    uint32_t getPulseCount();
    uint32_t getPulseCounting();
    void begin();
};

#endif