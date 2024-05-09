#include "validatorHelper.h"
#include "protocol/ICTBC/ICTBC.h"
#include "protocol/ESSP/essp.h"
#include "BillValidator.h"
int getBVProtocol(uint8_t txPin, uint8_t rxPin, HardwareSerial *uart)
{
    uart->begin(9600, SERIAL_8E1, rxPin, txPin, false, 1);
    essp dataESSPBus(uart, Master);
    ICTBC_BUS dataICTBus(uart, 0x01);
    uint8_t data[1];
    uint8_t receiveData[30];
    uint8_t dataCount = 0;
    uint32_t tick = millis();

    for (int i = 0; i < 10; i++)
    {
        int status = dataICTBus.SendCommand(0x03, 0x01, 0x0F, data, 0, receiveData, &dataCount, true);
        if (status == BV_CMD_RECEIVE_DONE)
        {
            Serial.printf("Validator Protocol : ICTBC\r\n");
            return 1;
        }
        SSP_PACKET CMD_PACKET;
        SSP_Device Device;
        Device.ID = 0;
        CMD_PACKET.PacketData[0] = 0x11;
        CMD_PACKET.PacketLength = 1;
        dataESSPBus.TransmitData(&Device, &CMD_PACKET);
        if (dataESSPBus.isDone(&Device))
            return 3;
        delay(1000);
    }
    return 1;
}

void BillValidatorBaseClass::BillValidatorBaseClass::pollEventCallback(uint8_t event, uint32_t data)
{
    for (_BV_POLL_HANDLER n : _PollHandlers)
    {
        if (n.event == event)
        {
            n.fn(data);
        }
    }
}

void BillValidatorBaseClass::on(uint8_t event, _BVOnEventCallbackType fn)
{
    _BV_POLL_HANDLER pollHander;
    pollHander.event = event;
    pollHander.fn = fn;
    _PollHandlers.push_back(pollHander);
}

bool BillValidatorBaseClass::BillValidatorBaseClass::init()
{
}
void BillValidatorBaseClass::loop()
{
}
int BillValidatorBaseClass::enableEsCrow() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::disableEsCrow() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::acceptBill() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::rejectBill() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::reset() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::setBillPayoutValue(uint32_t value) { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::payoutValue(int32_t amount, int32_t *totalPayout) { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::emptyReclyer() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::disableValidator() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::enableValidator() { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::countBillInPayout(int16_t *billCount) { return BV_CMD_SUCCESS; }
int BillValidatorBaseClass::payoutStatus() { return BV_CMD_SUCCESS; }
void BillValidatorBaseClass::setUsingBills(uint32_t *bills, uint8_t totalBills) {}
void BillValidatorBaseClass::startFillRecycler(){};
void BillValidatorBaseClass::endFillRecycler(){};
void BillValidatorBaseClass::setBillsMinMax(uint32_t min, uint32_t max){};
void BillValidatorBaseClass::setPayoutValue(uint32_t value){};
int BillValidatorBaseClass::getBillCountValue(){return 0;};
int BillValidatorBaseClass::getBillErrorCode(){return 0;};
int BillValidatorBaseClass::setReclyerCapacity(){return 0;};
int BillValidatorBaseClass::getReclyerCapacity(){return 0;};
_BV_CONFIG_TYPE BillValidatorBaseClass::getBVConfig()
{
    _BV_CONFIG_TYPE config;
    sprintf(config.fw, "0000000000");
    sprintf(config.manufacturer, "VHITEK");
    sprintf(config.model, "VHITEK_VM_01");
    config.multiplier = 1;
    config.payoutValue = 0;
    return config;
}
String BillValidatorBaseClass::getValidatorFirmware()
{
    return String("FW_BASECLASS");
}
String BillValidatorBaseClass::getValidatorName() { return String("NAME_BASECLASS"); }