#ifndef _VALIDATOR_HELPER_H_
#define _VALIDATOR_HELPER_H_
#include "Arduino.h"
#include <functional>

typedef std::function<void(unsigned long)> _BVOnEventCallbackType;
typedef struct
{
    _BVOnEventCallbackType fn;
    unsigned char event;
} _BV_POLL_HANDLER;

typedef struct
{
    uint32_t multiplier;
    uint32_t payoutValue;
    char fw[50];
    char manufacturer[8];
    char model[20];
} _BV_CONFIG_TYPE;

enum BVCMDStatus
{
    BV_CMD_BUSY = -5,
    BV_CMD_RECEIVE_TIMEOUT = -4,
    BV_CMD_RECEIVE_CHECKSUM_ERROR = -3,
    BV_CMD_RECEIVE_LENGHT_ERROR = -2,
    BV_CMD_CANNOT_PROCESS = -1,
    BV_CMD_SUCCESS = 0,
    BV_CMD_RECEIVE_DONE = 0,
    BV_CMD_PROCESSING
};

class BillValidatorBaseClass
{
public:
    std::vector<_BV_POLL_HANDLER> _PollHandlers;
    virtual void pollEventCallback(uint8_t event, uint32_t data);
    virtual void on(uint8_t event, _BVOnEventCallbackType fn);
    virtual bool init();
    virtual void loop();
    virtual void setUsingBills(uint32_t *bills, uint8_t totalBills);
    virtual int enableEsCrow();
    virtual int disableEsCrow();
    virtual int acceptBill();
    virtual int rejectBill();
    virtual int reset();
    virtual int setBillPayoutValue(uint32_t value);
    virtual int payoutValue(int32_t amount, int32_t *totalPayout);
    virtual int payoutStatus();
    virtual void startFillRecycler();
    virtual void endFillRecycler();
    virtual int emptyReclyer();
    virtual int disableValidator();
    virtual int enableValidator();
    virtual int countBillInPayout(int16_t *billCount);
    virtual int getBillCountValue();
    virtual String getValidatorFirmware();
    virtual String getValidatorName();
    virtual int getBillErrorCode();
    virtual int setReclyerCapacity();
    virtual _BV_CONFIG_TYPE getBVConfig();
    virtual void setBillsMinMax(uint32_t min, uint32_t max);
    virtual void setPayoutValue(uint32_t value);
    virtual int getReclyerCapacity();
};

int getBVProtocol(uint8_t txPin, uint8_t rxPin, HardwareSerial *uart);

#endif