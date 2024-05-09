#include "BillValidator.h"
#include "bv/ICT/BRV3.h"
#include "bv/ITL/nv11.h"
#include "bv/L70/l70.h"
#include "bv/L70_104/l70_104.h"

BillValidator::BillValidator()
{
}

BillValidator::~BillValidator()
{
}
void BillValidator::begin(uint8_t protocol)
{
    if (protocol == 3)
    {
        Serial.println("Begin SSP Protocol");
        this->BV = new NV11(this->ESSPDataBus);
        return;
    }
    if (protocol == 4)
    {
        Serial.println("Begin PULSE Protocol");
        this->BV = new L70(this->ICTPulseBus);
        this->ICTPulseBus->begin();
        return;
    }
    if (protocol == 5)
    {
        Serial.println("Begin ICT104 Protocol");
        this->BV = new L70_104(this->ICT104Bus);
        this->ICT104Bus->begin(05, 18,'U');
        return;
    }
    this->BV = new BRV3(this->ICTDataBus);
}