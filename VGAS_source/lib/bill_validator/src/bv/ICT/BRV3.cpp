#include "BRV3.h"
BRV3::BRV3(ICTBC_BUS *bus)
{
    this->dataBus = bus;
}

BRV3::~BRV3()
{
}

void BRV3::_sendACK()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    this->dataBus->SendCommand(0x03, 0x01, 0x00, data, 0, receiveData, &dataCount, false);
}

int BRV3::reset()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x0A, data, 0, receiveData, &dataCount, true);
    if (status < 0)
    {
        return status;
    }
    return BV_CMD_SUCCESS;
}

int BRV3::setReclyerCapacity()
{
    uint8_t data[1] = {25};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x21, data, 1, receiveData, &dataCount, true);
    Serial.printf("Set Bill status :%02X\r\n", status);
    if (status < 0)
    {
        return status;
    }
    return BV_CMD_SUCCESS;
}

int BRV3::getReclyerCapacity()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x22, data, 0, receiveData, &dataCount, true);
    if (status < 0)
    {
        return status;
    }
    return receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET];
}

int BRV3::setBillPayoutValue(uint32_t value)
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int billChannel = -1;
    for (int i = 0; i < 16; i++)
    {
        if (_billTypeCredit[i] == value)
        {
            billChannel = i;
            break;
        }
    }
    // Serial.printf("setBillPayoutValue %d \r\n", billChannel);
    if (billChannel == -1)
        return BV_CMD_CANNOT_PROCESS;
    data[0] = billChannel + 1;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x17, data, 1, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    config.payoutValue = value;

    return BV_CMD_SUCCESS;
}

int BRV3::emptyReclyer()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x1F, data, 0, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    status = 0;
    int retry = 0;
    while (status <= 0)
    {
        status = _TransferStatus();
        if (status < 0)
        {
            if (retry++ > 5)
            {
                return BV_CMD_CANNOT_PROCESS;
            }
        }
        delay(10);
    }
    return BV_CMD_SUCCESS;
}

int BRV3::_TransferStatus()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x20, data, 0, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return ICTBC_ERROR_NACK;
    if (receiveData[1] != 7)
        return ICTBC_ERROR_PACKET_LENGHT;
    if (receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 1] != 0x11)
    {
        return 1;
    }
    return 0;
}

int BRV3::payoutStatus()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x1D, data, 0, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    if (receiveData[1] != 7)
        return BV_CMD_RECEIVE_LENGHT_ERROR;
    if (receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 1] != 0x11)
    {
        return BV_CMD_SUCCESS;
    }
    return BV_CMD_PROCESSING;
}

int BRV3::countBillInPayout(int16_t *billCount)
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x1B, data, 0, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    if (receiveData[1] != 1)
        return BV_CMD_RECEIVE_LENGHT_ERROR;
    *billCount = receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET];
    return BV_CMD_SUCCESS;
}

int BRV3::payoutValue(int32_t amount, int32_t *totalPayout)
{
    int32_t _BillPayoutValue = _payoutVal;
    int32_t billToPayout = amount / _BillPayoutValue;
    uint8_t data[1] = {billToPayout};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int16_t totalBillsInPayoutAfter;
    int16_t totalBillsInPayoutBefore = 0;
    if (countBillInPayout(&totalBillsInPayoutBefore) != BV_CMD_SUCCESS)
        return BV_CMD_CANNOT_PROCESS;
    // Serial.printf("Total Bills before: %d\n", totalBillsInPayoutBefore);
    if (totalBillsInPayoutBefore < billToPayout)
    {
        if (totalBillsInPayoutBefore == 0)
        {
            *totalPayout = 0;
            return BV_CMD_SUCCESS;
        }
        else
        {
            data[0] = totalBillsInPayoutBefore;
        }
    }

    int status = this->dataBus->SendCommand(0x03, 0x01, 0x1C, data, 1, receiveData, &dataCount, true);
    if (status < 0)
        return status;

    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    int retry = 0;
    while (1)
    {
        delay(200);
        status = payoutStatus();
        if (status == BV_CMD_SUCCESS)
        {
            break;
        }

        if (status < BV_CMD_SUCCESS)
        {
            retry++;
            if (retry > 3)
            {
                return BV_CMD_CANNOT_PROCESS;
            }
        }
        else
        {
            retry = 0;
        }
    }
    status = 0;
    for (int i = 0; i < 5; i++)
    {
        if (countBillInPayout(&totalBillsInPayoutAfter) == BV_CMD_SUCCESS)
        {
            // Serial.printf("Total Bills after: %d\n", totalBillsInPayoutAfter);
            status = 1;
            break;
        }
    }
    if (status == 0)
    {
        *totalPayout = amount;
    }
    else
    {
        *totalPayout = (totalBillsInPayoutBefore - totalBillsInPayoutAfter) * _BillPayoutValue;
    }
    return BV_CMD_SUCCESS;
}

int BRV3::acceptBill()
{
    uint8_t data[1] = {0x11};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x1A, data, 1, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    return BV_CMD_SUCCESS;
}

int BRV3::rejectBill()
{
    uint8_t data[1] = {0x22};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x1A, data, 1, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    _billHoldingVal = 0;
    return BV_CMD_SUCCESS;
}

int BRV3::enableEsCrow()
{
    uint8_t data[1] = {0xFF};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x11, data, 1, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    _escrowEnabled = true;
    return BV_CMD_SUCCESS;
}

int BRV3::disableEsCrow()
{
    uint8_t data[1] = {0x00};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x11, data, 1, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return BV_CMD_CANNOT_PROCESS;
    _escrowEnabled = true;
    return BV_CMD_SUCCESS;
}

int BRV3::_setBillEnable(uint8_t highByte, uint8_t lowByte)
{
    uint8_t data[2] = {highByte, lowByte};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x15, data, 2, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return ICTBC_ERROR_NACK;
    return 0;
}

int BRV3::_getBillEnable(uint8_t *highByte, uint8_t *lowByte)
{
    uint8_t data[2];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x16, data, 0, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if ((receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_ACK) && (receiveData[4] != ICTBC_RECEIVE_PACKET_DATA_REPLYCMD))
        return ICTBC_ERROR_NACK;
    *highByte = receiveData[5];
    *lowByte = receiveData[6];
    return 0;
}

int32_t BRV3::_getBillTypeCredit(uint8_t channel)
{
    uint8_t data[1] = {channel};
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x14, data, 1, receiveData, &dataCount, true);
    if (status < 0)
        return status;
    if (receiveData[1] != 3)
        return ICTBC_ERROR_PACKET_LENGHT;
    int32_t tmp = (((int32_t)receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET]) << 16) |
                  ((int32_t)receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 1] << 8) |
                  ((int32_t)receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 2]);
    return tmp;
}

void BRV3::setUsingBills(uint32_t *bills, uint8_t totalBills)
{
    for (unsigned int i = 0; i < totalBills; i++)
    {
        if (i > 16)
            return;
        _billsAccept[i] = bills[i];
        // Serial.printf("Notes setAcceptNotes %d \r\n", bills[i]);
    }
}

int BRV3::getBillErrorCode()
{
    return _errorCode;
}

void BRV3::_pollPraser(uint8_t evnt, uint8_t data)
{
    switch (evnt)
    {
    case ICTBC_POLL_EVENT_BILL_ESCROW:
        _billHoldingVal = _billTypeCredit[data - 1];
        Serial.printf("ICTBC_POLL_EVENT_BILL_ESCROW %ld \r\n", _billTypeCredit[data - 1]);
        for (int i = 0; i < 16; i++)
        {
            if ((_billHoldingVal >= _min) && (_billHoldingVal <= _max))
            {
                pollEventCallback(BV_EVENT_BILL_HOLDING, _billHoldingVal);
                break;
            }
        }
        break;
    case ICTBC_POLL_EVENT_BILL_STACKED_CASHBOX:
        _billHoldingVal = 0;
        Serial.printf("ICTBC_POLL_EVENT_BILL_STACKED_CASHBOX %ld \r\n", _billTypeCredit[data - 1]);
        pollEventCallback(BV_EVENT_BILL_STACKED_CASHBOX, _billTypeCredit[data - 1]);
        break;
    case ICTBC_POLL_EVENT_BILL_STACKED_RECYCLER:
        _billHoldingVal = 0;
        Serial.printf("ICTBC_POLL_EVENT_BILL_STACKED_RECYCLER %ld \r\n", _billTypeCredit[data - 1]);
        pollEventCallback(BV_EVENT_BILL_STACKED_RECLYER, _billTypeCredit[data - 1]);
        break;
    case ICTBC_POLL_EVENT_BILL_RETURNED:
        _billHoldingVal = 0;
        Serial.println("ICTBC_POLL_EVENT_BILL_RETURNED");
        pollEventCallback(BV_EVENT_BILL_REJECTED, 0);
        break;
    case ICTBC_POLL_EVENT_BILL_WAITING:
        Serial.println("ICTBC_POLL_EVENT_BILL_WAITING");
        pollEventCallback(ICTBC_POLL_EVENT_BILL_WAITING, 0);
        break;
    case ICTBC_POLL_EVENT_BILL_TO_CASHBOX:
        Serial.println("ICTBC_POLL_EVENT_BILL_TO_CASHBOX");
        pollEventCallback(ICTBC_POLL_EVENT_BILL_TO_CASHBOX, 0);
        break;
    case ICTBC_POLL_EVENT_FAULT:
        Serial.printf("ICTBC_POLL_EVENT_FAULT ,CODE : %d\r\n", data);
        _errorCode = data;
        if ((data == 5))
        {
            pollEventCallback(BV_EVENT_BILL_REJECTED, 0);
            _errorCode = 0;
        }
        else
            pollEventCallback(ICTBC_POLL_EVENT_FAULT, data);

        break;
    default:
        break;
    }
}

int BRV3::_Poll()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    int messageCnt = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x19, data, 0, receiveData, &dataCount, true);
    if (status != BV_CMD_SUCCESS)
        return status;
    if (receiveData[1] != 13)
        return BV_CMD_RECEIVE_LENGHT_ERROR;
    messageCnt = receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET];
    if ((messageCnt != lastMessageCnt))
    {
        Serial.printf("\r\n--------------------------------------------------\r\n");
        Serial.printf("LAST MESSCNT %d -TOTAL MESSAGE : %d\r\n", lastMessageCnt, messageCnt);
        if (messageCnt != 0)
        {
            int missing = 0;
            if (messageCnt != 1)
            {
                missing = messageCnt - lastMessageCnt;
            }
            else
            {
                if (lastMessageCnt > messageCnt)
                {
                    missing = 255 + messageCnt - lastMessageCnt;
                }
                else
                {
                    missing = 1;
                }
            }
            if (missing < 0)
            {
                missing = 0;
            }
            if (missing > 6)
            {
                missing = 6;
            }
            if (missing)
            {
                Serial.printf("Event Count : %d \r\n", missing);
            }

            for (int i = missing; i >= 1; i--)
            {
                /* code */
                int offset = (i) * 2;
                Serial.printf("\r\n offset : %d , idx %d \r\n", offset, i);
                if (isInitDone)
                    _pollPraser(receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + offset - 1], receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + offset]);
            }
        }
        else
        {
            isInitDone = false;
            lastMessageCnt = 0;
            Serial.println("VALIDATOR POWER UP");
        }
        Serial.printf("\r\n--------------------------------------------------\r\n");
        lastMessageCnt = messageCnt;
    }
    return 0;
}

void BRV3::startFillRecycler() { isFillReclyer = true; };
void BRV3::endFillRecycler() { isFillReclyer = false; };
int BRV3::getBillCountValue()
{
    return _billCountValue;
}

void BRV3::loop()
{
    static uint32_t tickCount = 0;
    static uint32_t payoutCountTick = 0;
    if (!isInitDone)
    {
        init();
    }
    else
    {
        _Poll();
        if (_billHoldingVal)
        {
            if (isFillReclyer)
            {
                if (_billHoldingVal == config.payoutValue)
                {
                    acceptBill();
                    return;
                }
                else
                {
                    rejectBill();
                    return;
                }
            }
        }

        if (_billHoldingVal != 0)
        {
            bool accept = false;
            Serial.printf("%d min: %d, max :%d\r\n", _billHoldingVal, _min, _max);
            if ((_billHoldingVal >= _min) && (_billHoldingVal <= (_max + 1)))
            {
                accept = true;
            }

            if (!accept)
            {
                rejectBill();
            }
        }

        if ((uint32_t)(millis() - payoutCountTick) > 10000)
        {
            int16_t bill = 0;
            int status = countBillInPayout(&bill);
            if (status == BV_CMD_SUCCESS)
            {
                if (bill < 0)
                    bill = 0;
                if (bill > 100)
                    bill = 0;
                _billCountValue = bill;
            }
            payoutCountTick = millis();
        }

        if (lastMessageCnt == 0) // try to detect bill validator has reset
        {
            if ((uint32_t)(millis() - tickCount) > 2000)
            {
                uint8_t lowByte, highByte;
                if (_getBillEnable(&lowByte, &highByte) >= 0)
                {
                    if ((lowByte != 0xFF) || (highByte != 0xFF))
                        isInitDone = false;
                }
                tickCount = millis();
            }
        }
    }
}

_BV_CONFIG_TYPE BRV3::getBVConfig()
{
    return config;
}

bool BRV3::init()
{
    int32_t noteVals[16];
    String FW = getValidatorFirmware();
    if (!isReset)
    {
        isReset = true;
        reset();
    }
    if (FW.length() == 0)
    {
        return false;
    }
    sprintf(config.fw, "%s", FW.c_str());
    if (enableEsCrow() != BV_CMD_SUCCESS)
        return false;

    for (int i = 0; i < 16; i++)
    {
        _billTypeCredit[i] = 0;
        int32_t noteVal = _getBillTypeCredit(i + 1);
        Serial.printf(" Note Channel %d : %dVND\r\n", i, noteVal);
        if (noteVal < 0)
            return false;
        noteVals[i] = noteVal;
    }

    if (_setBillEnable(0xFF, 0xFF) != BV_CMD_SUCCESS)
        return false;

    for (int i = 0; i < 16; i++)
    {
        _billTypeCredit[i] = noteVals[i];
    }
    //_setReclyerCapacity();
    if (setBillPayoutValue(_payoutVal) != BV_CMD_SUCCESS)
        return false;
    if (setReclyerCapacity() != BV_CMD_SUCCESS)
        return false;
    isInitDone = true;
    isReset = false;
    return true;
}

void BRV3::setBillsMinMax(uint32_t min, uint32_t max)
{
    _min = min;
    _max = max;
}

void BRV3::setPayoutValue(uint32_t value)
{
    _payoutVal = value;
}

String BRV3::getValidatorFirmware()
{
    uint8_t data[1];
    uint8_t receiveData[30];
    char output[40];
    uint8_t dataCount = 0;
    int status = this->dataBus->SendCommand(0x03, 0x01, 0x0F, data, 0, receiveData, &dataCount, true);
    if (status < 0)
        return String("");
    if (receiveData[1] != 24)
        return String("");
    for (int i = 0; i < 20; i++)
    {
        output[i] = receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + i];
    }
    sprintf(&output[20], "(%02X.%02X_%02X%02X)",
            receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 20],
            receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 21],
            receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 22],
            receiveData[ICTBC_RECEIVE_PACKET_DATA_OFFSET + 23]);
    return String(output);
}