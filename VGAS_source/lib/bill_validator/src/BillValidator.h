#ifndef _VALIDATOR_H_
#define _VALIDATOR_H_
#include "protocol/ICTBC/ICTBC.h"
#include "protocol/ESSP/essp.h"
#include "protocol/PULSE/ICTPULSE.h"
#include "protocol/ICT104/ICT104.h"
#include "validatorHelper.h"

#define BV_EVENT_BILL_STACKED_CASHBOX 0
#define BV_EVENT_BILL_STACKED_RECLYER 1
#define BV_EVENT_ERROR 2
#define BV_EVENT_BILL_HOLDING 3
#define BV_EVENT_BILL_REJECTED 4

class BillValidator
{
private:
    int validatorProtocol; // 1 : MDB Protocol , 2 : ICTBC Protocol, 3 : ESSP protocol
    int getValidatorProtocol();
    uint8_t txPin, rxPin;
    HardwareSerial *uart;

public:
    BillValidator();
    virtual ~BillValidator();
    BillValidatorBaseClass *BV;
    ICTBC_BUS *ICTDataBus;
    ICTPULSE_BUS *ICTPulseBus;
    ICT104_BUS *ICT104Bus;
    essp *ESSPDataBus;
    void begin(uint8_t protocol);
    void init();
};

#endif