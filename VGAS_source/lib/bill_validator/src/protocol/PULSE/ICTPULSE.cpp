#include "ICTPULSE.h"
uint32_t pulseCount = 0;
uint32_t tick = 0;
uint16_t pulseDataPin;
uint32_t interruptTick = 0;
void IRAM_ATTR pulseInterrupt()
{
    if (digitalRead(pulseDataPin) == 0)
    {
        tick = millis();
    }
    else
    {
        uint32_t driff = millis() - tick;
        if ((driff >= 40) && (driff < 60))
        {
            pulseCount++;
            //Serial.printf("tick driff %d - count %d\r\n", driff, pulseCount);
        }
    }
    interruptTick = millis();
}

ICTPULSE_BUS::ICTPULSE_BUS(int16_t enPin, int16_t pulsePin, HardwareSerial *uart)
{
    this->enPin = enPin;
    this->pulsePin = pulsePin;
    pulseDataPin = pulsePin;
}

void ICTPULSE_BUS::begin()
{
    pinMode(enPin, OUTPUT);
    pinMode(pulsePin, INPUT_PULLUP);
    attachInterrupt(pulsePin, pulseInterrupt, CHANGE);
}

uint32_t ICTPULSE_BUS::getPulseCounting()
{
    return pulseCount;
}

uint32_t ICTPULSE_BUS::getPulseCount()
{
    if (millis() - interruptTick >= 250)
    {
        uint32_t count = pulseCount;
        if (pulseCount)
            pulseCount = 0;
        return count;
    }
    return 0;
}