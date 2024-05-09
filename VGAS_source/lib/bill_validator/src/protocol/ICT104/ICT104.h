#ifndef _ICT104_H_
#define _ICT104_H_
#define ICTBC_DEBUG false
#include "validatorHelper.h"
class ICT104_BUS
{
private:
    uint8_t myAdrr;
    int txPin;
    int rxPin;
    char type104;

public:
    HardwareSerial *Serial;

    ICT104_BUS(HardwareSerial *uart);
    char getICT104Type();
    void begin(int txPin, int rxPin, char type);
};

#endif