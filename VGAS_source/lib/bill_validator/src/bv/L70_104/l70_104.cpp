#include "L70_104.h"
uint32_t noteValue[9] = {5000, 10000, 20000, 50000, 100000, 200000, 500000};
int count = 0;
uint32_t lockChannel0;
L70_104::~L70_104()
{
}

L70_104::L70_104(ICT104_BUS *bus)
{
    this->bus = bus;
}

void L70_104::processVType()
{
    char buffer[255];
    char ptr = 0;
    if (this->bus->Serial->available())
    {
        Serial.println("New Data :");

        while (this->bus->Serial->available() > 0)
        {
            int data = this->bus->Serial->read();
            if ((data != 0xff) && (data != 0x00))
            {
                Serial.printf(" %02X", data);
                buffer[ptr++] = data;
                if (ptr > 200)
                    break;
            }

            if (ptr > 3)
            {
                if ((buffer[ptr - 1] == 0x4E) && (buffer[ptr - 2] == 0x56) && (buffer[ptr - 3] == 0x8F) && (buffer[ptr - 4] == 0x80))
                {
                    Serial.println("VALIDATOR POWER ON");
                    this->bus->Serial->write(0x02);
                }
            }

            if (ptr > 1)
            {
                if ((buffer[ptr - 2] == 0x81) && (buffer[ptr - 1] >= 0x40) && (buffer[ptr - 1] <= 0x44))
                {
                    int channel = buffer[ptr - 1] - 0x40;
                    Serial.printf("\r\nNEW NOTE CHANNEL : %d\r\n", buffer[ptr - 1] - 0x40);
                    delay(20);
                    while (this->bus->Serial->available())
                    {
                        this->bus->Serial->read();
                        if (this->bus->Serial->available() == 0)
                        {
                            delay(5);
                        }
                    }
                    this->bus->Serial->write(0x02);
                    bool waitDone = false;
                    uint32_t tick = millis();
                    while (1)
                    {
                        if (millis() - tick > 5000)
                        {
                            break;
                        }
                        if (this->bus->Serial->available())
                        {
                            waitDone = true;
                            break;
                        }
                        delay(10);
                    }

                    if (waitDone)
                    {
                        if (this->bus->Serial->read() == 0x10)
                        {
                            Serial1.println("STACKED");
                            pollEventCallback(BV_EVENT_BILL_STACKED_CASHBOX, noteValue[channel]);
                        }
                        else
                        {
                            Serial1.println("REJECT");
                        }
                    }
                }
            }

            if (this->bus->Serial->available() == 0)
            {
                delay(20);
            }
        }
        Serial.println();
    }
}

void L70_104::processUType()
{
    char buffer[255];
    char ptr = 0;
    if (this->bus->Serial->available())
    {
        Serial.println("New Data :");

        while (this->bus->Serial->available() > 0)
        {
            int data = this->bus->Serial->read();
            if ((data != 0xff) && (data != 0x00))
            {
                Serial.printf(" %02X", data);
                buffer[ptr++] = data;
                if (ptr > 200)
                    break;
            }

            if (buffer[ptr - 1] == 0x2F)
            {
                if ((unsigned long)(millis() - lockChannel0) < 10000)
                {
                    lockChannel0 = millis();
                }
                else
                {
                    count++;
                }
            }

            if (ptr > 1)
            {
                if ((buffer[ptr - 1] == 0x8F) && (buffer[ptr - 2] == 0x80))
                {
                    Serial.println("VALIDATOR POWER ON");
                    this->bus->Serial->write(0x02);
                }
            }

            if (ptr > 2)
            {
                if ((buffer[ptr - 3] == 0x81) && (buffer[ptr - 2] == 0x8F) && (buffer[ptr - 1] >= 0x40) && (buffer[ptr - 1] <= 0x44))
                {
                    int channel = buffer[ptr - 1] - 0x40;
                    Serial.printf("\r\nNEW NOTE CHANNEL : %d\r\n", buffer[ptr - 1] - 0x40);
                    delay(20);
                    while (this->bus->Serial->available())
                    {
                        this->bus->Serial->read();
                        if (this->bus->Serial->available() == 0)
                        {
                            delay(5);
                        }
                    }
                    // Serial.printf("channel: %02X, count %d, coutLock %d\r\n", channel, count)/=;
                    if (channel == 0)
                    {
                        if (count >= 3)
                        {
                            lockChannel0 = millis();
                        }
                        if ((unsigned long)(millis() - lockChannel0) < 10000)
                        {
                            this->bus->Serial->write(0x0F);
                            Serial.printf("Reject: %d", (unsigned long)(millis() - lockChannel0));
                            count = 0;
                        }
                        else
                        {
                            this->bus->Serial->write(0x02);
                            count = 0;
                            lockChannel0 = 0;
                            Serial.println("Accept");
                        }
                    }
                    else
                    {
                        this->bus->Serial->write(0x02);
                        count = 0;
                        lockChannel0 = 0;
                        Serial.println("Accept");
                    }
                    bool waitDone = false;
                    uint32_t tick = millis();
                    while (1)
                    {
                        if (millis() - tick > 5000)
                        {
                            break;
                        }
                        if (this->bus->Serial->available())
                        {
                            waitDone = true;
                            break;
                        }
                        delay(10);
                    }

                    if (waitDone)
                    {
                        if (this->bus->Serial->read() == 0x10)
                        {
                            Serial1.println("STACKED");
                            pollEventCallback(BV_EVENT_BILL_STACKED_CASHBOX, noteValue[channel]);
                        }
                        else
                        {
                            Serial1.println("REJECT");
                        }
                    }
                }
            }

            if (this->bus->Serial->available() == 0)
            {
                delay(20);
            }
        }
        Serial.println();
    }
}

void L70_104::loop()
{
    processUType();
}