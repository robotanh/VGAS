#include "ICT104.h"

ICT104_BUS::ICT104_BUS(HardwareSerial *uart)
{
    this->Serial = uart;
}

char ICT104_BUS::getICT104Type()
{
    return this->type104;
}

void ICT104_BUS::begin(int txPin, int rxPin, char type)
{
    this->rxPin = rxPin;
    this->txPin = txPin;
    this->type104 = type;
    int baud = 4800;
    if (this->type104 == 'U')
        baud = 9600;
    this->Serial->begin(baud,SERIAL_8E1, this->rxPin, this->txPin, false);
}
