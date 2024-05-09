#ifndef _ICTBC_H_
#define _ICTBC_H_
#define ICTBC_DEBUG false
#include "validatorHelper.h"
class ICTBC_BUS
{
private:
    HardwareSerial *MDB_Serial;
    uint8_t myAdrr;
    int8_t SumCalc(uint8_t *data, uint8_t len);

public:
    ICTBC_BUS(HardwareSerial *uart, uint8_t adrr);
    virtual int8_t SendCommand(uint8_t des, uint8_t soucre, uint8_t CMD, uint8_t *data, uint8_t dataLen, uint8_t *output, uint8_t *dataByteCount, bool revceive);
    int8_t GetData(uint8_t *dataBytes, uint8_t *dataByteCount);
};

#endif