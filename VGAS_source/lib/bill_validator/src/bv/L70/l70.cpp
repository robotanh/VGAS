#include "l70.h"
L70::~L70()
{
}

L70::L70(ICTPULSE_BUS *bus)
{
    this->bus = bus;
}

void L70::loop()
{
    uint32_t pulseCount = this->bus->getPulseCount();
    static uint32_t lastCounting = 0;
    if (this->bus->getPulseCounting())
    {
        if (lastCounting != this->bus->getPulseCounting())
        {
            pollEventCallback(BV_EVENT_BILL_HOLDING, this->bus->getPulseCounting() * 5000);
            lastCounting = this->bus->getPulseCounting();
        }
    }

    if (pulseCount)
    {
        lastCounting=0;
        pollEventCallback(BV_EVENT_BILL_STACKED_CASHBOX, pulseCount * 5000);
    }
}