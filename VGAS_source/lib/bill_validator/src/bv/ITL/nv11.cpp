#include "nv11.h"
NV11::NV11(essp *bus)
{
    this->dataBus = bus;
    Device.ID = 0;
    begin();
}

int NV11::getBillCountValue()
{
    return _billCountValue;
}

void NV11::_addPollEvent(uint8_t event, SSP_POLL_TYPE pollType, String name, int8_t size, String description)
{
    SSP_POLL_EVENT_DEF pollEvent;
    pollEvent.event = event;
    pollEvent.size = size;
    pollEvent.name = name;
    pollEvent.description = description;
    pollEvent.type = pollType;
    _pollEventList.push_back(pollEvent);
}

void NV11::_on(uint8_t pollEvent, _OnEventCallbackHandler fn)
{
    _SSP_POLL_HANDLER pollHander;
    pollHander.event = pollEvent;
    pollHander.fn = fn;
    _PollHandlers.push_back(pollHander);
    // Serial.println(_PollHandlers.size());
}

void NV11::begin()
{

    _addPollEvent(SSP_POLL_RESET,
                  SSP_POLL_INFO,
                  "SSP_POLL_RESET",
                  0,
                  F("An event gven when the device has been powered up or power cycled and has run through its reset process"));

    _addPollEvent(SSP_POLL_READ,
                  SSP_POLL_INFO,
                  "SSP_POLL_READ",
                  1, F("An event given when the BNV is reading a banknote."));

    _addPollEvent(SSP_POLL_CREDIT,
                  SSP_POLL_INFO,
                  "SSP_POLL_CREDIT",
                  1, F("This event is generated when the banknote has been moved from the escrow position to a safe postion within the validator system where the baknote cannot be retreived by the user."));

    _addPollEvent(SSP_POLL_REJECTING,
                  SSP_POLL_INFO,
                  "SSP_POLL_REJECTING",
                  0, F("A bill is in the process of being rejected back to the user by the Banknte Validator."));

    _addPollEvent(SSP_POLL_REJECTED,
                  SSP_POLL_INFO,
                  "SSP_POLL_REJECTED",
                  0, F("A bill has been rejected back to the user by the Banknote Validator."));

    _addPollEvent(SSP_POLL_STACKING,
                  SSP_POLL_INFO,
                  "SSP_POLL_STACKING",
                  0, F("The bill is currently being transported to and through the device stacker."));

    _addPollEvent(SSP_POLL_STACKED,
                  SSP_POLL_INFO,
                  "SSP_POLL_STACKED",
                  0, F("A bill has been transported trough the banknote validator and is in it's stacked position."));

    _on(SSP_POLL_CREDIT, [&](SSP_POLL_EVENT_DATA data)
        {
            Serial.printf("SSP_POLL_CREDIT OnEVent %d %d \n", data.data.size());
            if (data.data.size() < 1)
                return;
            if ((data.data[0] < 1) | (data.data[0] >= 16))
                return;
            if (config.payoutValue == _billTypeCredit[data.data[0] - 1])
            {
                if (_payoutCapacity < 30)
                {
                    pollEventCallback(BV_EVENT_BILL_STACKED_RECLYER, _billTypeCredit[data.data[0] - 1]);
                }
                else
                {
                    pollEventCallback(BV_EVENT_BILL_STACKED_CASHBOX, _billTypeCredit[data.data[0] - 1]);
                }
            }else{
                pollEventCallback(BV_EVENT_BILL_STACKED_CASHBOX, _billTypeCredit[data.data[0] - 1]);

            } });

    _addPollEvent(SSP_POLL_SAFE_JAM,
                  SSP_POLL_ERROR,
                  "SSP_POLL_SAFE_JAM",
                  0, F("A bill has been detected as jammed during it's transport to the stacked position. A Safe jam indicates that the bill is not retrievable by the user at this point."));

    _addPollEvent(SSP_POLL_UNSAFE_JAM,
                  SSP_POLL_ERROR,
                  "SSP_POLL_UNSAFE_JAM",
                  0, F("A bill has been detected as jammed during it's transport through the validator. An unsafe jam indicates that this bill may be in a position when the user could retrieve it from the validator bezel."));

    _addPollEvent(SSP_POLL_DISABLED,
                  SSP_POLL_INFO,
                  "SSP_POLL_DISABLED",
                  0, F("A disabled event is given in response to a poll command when a device has been disabled by the host or by some other internal function of the device."));

    _on(SSP_POLL_DISABLED, [&](SSP_POLL_EVENT_DATA data)
        {
            Serial.printf("SSP_POLL_DISABLED OnEVent ");
            initDone=false; });

    _addPollEvent(SSP_POLL_FRAUD_ATTEMPT,
                  SSP_POLL_ERROR,
                  "SSP_POLL_FRAUD_ATTEMPT",
                  1, F("The validator system has detected an attempt to mauipulate the coin/banknote in order to fool the system to register credits with no monies added."));

    _addPollEvent(SSP_POLL_STACKER_FULL,
                  SSP_POLL_ERROR,
                  "SSP_POLL_STACKER_FULL",
                  0, F("Event in response to poll given when the device has detected that the stacker unit has stacked it's full limit of banknotes."));

    _addPollEvent(SSP_POLL_CLEARED_FROM_FRONT,
                  SSP_POLL_INFO,
                  "SSP_POLL_CLEARED_FROM_FRONT",
                  1, F("During the device power-up sequence a bill was detected as being in the note path. This bill is then rejected from the device via the bezel and this event is issued. If the bill value is known then the channel number is given in the data byte, otherwise the data byte will be zero value."));

    _addPollEvent(SSP_POLL_CLEARED_INTO_CASHBOX,
                  SSP_POLL_INFO,
                  "SSP_POLL_CLEARED_INTO_CASHBOX",
                  1, F("During the device power-up sequence a bill was detected as being in the stack path. This bill is then moved into the device cashbox and this event is issued. If the bill value is known then the channel number is given in the data byte, otherwise the data byte will be zero value."));
    _on(SSP_POLL_CLEARED_INTO_CASHBOX, [&](SSP_POLL_EVENT_DATA data)
        {
            if (data.data.size() < 1)
                return;
            if ((data.data[0] < 1) | (data.data[0] >= 16))
                return;
            pollEventCallback(BV_EVENT_BILL_STACKED_CASHBOX, _billTypeCredit[data.data[0] - 1]); });
    _addPollEvent(SSP_POLL_BARCODE_VALIDATE,
                  SSP_POLL_INFO,
                  "SSP_POLL_BARCODE_VALIDATE",
                  0, F("A barcode ticket has been scanned and identified by the system and is currently held in the escrow position."));

    _addPollEvent(SSP_POLL_BARCODE_ACK,
                  SSP_POLL_INFO,
                  "SSP_POLL_BARCODE_ACK",
                  0, F("The device has moved the barcode ticket to a safe stack position."));

    _addPollEvent(SSP_POLL_CASH_BOX_REMOVED,
                  SSP_POLL_INFO,
                  "SSP_POLL_CASH_BOX_REMOVED",
                  0, F("The system has detected that the cashbox unit has been removed from it's working position."));

    _addPollEvent(SSP_POLL_CASH_BOX_REPLACED,
                  SSP_POLL_INFO,
                  "SSP_POLL_CASH_BOX_REPLACED",
                  0, F("The device cashbox box unit has been detected as replaced into it's working position."));

    _addPollEvent(SSP_POLL_DISPENSING,
                  SSP_POLL_INFO,
                  "SSP_POLL_DISPENSING",
                  5, F("The device is in the process of paying out a requested value. The value paid at the poll is given in the event data."));

    _addPollEvent(SSP_POLL_DISPENSED,
                  SSP_POLL_INFO,
                  "SSP_POLL_DISPENSED",
                  5, F("Show the total value the device has dispensed in repsonse to a Dispense command."));
    _on(SSP_POLL_DISPENSED, [&](SSP_POLL_EVENT_DATA data)
        { isDispening = false; 
        initDone= false; });
    _addPollEvent(SSP_POLL_JAMMED,
                  SSP_POLL_ERROR,
                  "SSP_POLL_JAMMED",
                  5, F("An event showing the hopper unit has jammed and giving the value paid/floated upto that jam.On the smart payout this event is used when a jam occurs during a payout / float / empty operation."));

    _addPollEvent(SSP_POLL_HALTED,
                  SSP_POLL_ERROR,
                  "SSP_POLL_HALTED",
                  5, F("Triggered when payout is interrupted for some reason."));
    _on(SSP_POLL_PAYOUT_OUT_OF_SERVICE, [&](SSP_POLL_EVENT_DATA data)
        { isDispening = false; });
    _addPollEvent(SSP_POLL_FLOATING,
                  SSP_POLL_INFO,
                  "SSP_POLL_FLOATING",
                  -1, F("Event showing the amount of cash floated up to the poll point"));

    _addPollEvent(SSP_POLL_CASHBOX_PAID,
                  SSP_POLL_INFO,
                  "SSP_POLL_CASHBOX_PAID",
                  -1, F("Coin values have been detected and paid to the cashbox since the last poll."));

    _addPollEvent(SSP_POLL_COIN_CREDIT,
                  SSP_POLL_INFO,
                  "SSP_POLL_COIN_CREDIT",
                  7, F("A coin has been detected as added to the system. This would be usually via the seperate coin mech attached to the system port ."));

    _addPollEvent(SSP_POLL_EMPTYING,
                  SSP_POLL_INFO,
                  "SSP_POLL_EMPTYING",
                  0, F("The device is currently performing is empty operation following an Empty command request."));

    _addPollEvent(SSP_POLL_SMART_EMPTYING,
                  SSP_POLL_INFO,
                  "SSP_POLL_SMART_EMPTYING",
                  -1, F("The device is in the process of carrying out its Smart Empty command from the host. The value emptied at the poll point is given in the event data"));

    _addPollEvent(SSP_POLL_EMPTY,
                  SSP_POLL_INFO,
                  "SSP_POLL_EMPTY",
                  0, F("The device has completed it's empty operation in response to the Empty command"));
    _on(SSP_POLL_EMPTY, [&](SSP_POLL_EVENT_DATA data)
        { initDone = false; });

    _addPollEvent(SSP_POLL_COINS_LOW,
                  SSP_POLL_INFO,
                  "SSP_POLL_COINS_LOW",
                  0, F(""));

    _addPollEvent(SSP_POLL_COINS_EMPTY,
                  SSP_POLL_INFO,
                  "SSP_POLL_COINS_EMPTY",
                  0, F(""));

    _on(SSP_POLL_PAYOUT_OUT_OF_SERVICE, [&](SSP_POLL_EVENT_DATA data)
        { isDispening = false; });
    _addPollEvent(SSP_POLL_PAYOUT_OUT_OF_SERVICE,
                  SSP_POLL_ERROR,
                  "SSP_POLL_PAYOUT_OUT_OF_SERVICE",
                  0, F("This event is given if the payout goes out of service during operation. If this event is detected after a poll, the host can send the ENABLE PAYOUT DEVICE command to determine if the payout unit comes back into service."));
}

bool NV11::_sync()
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x11;
    CMD_PACKET.PacketLength = 1;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_setHostProtocolVersion(uint8_t version)
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x06;
    CMD_PACKET.PacketData[1] = version;
    CMD_PACKET.PacketLength = 2;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_setNoteReportByChannel(uint8_t version)
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x45;
    CMD_PACKET.PacketData[1] = version;
    CMD_PACKET.PacketLength = 2;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_0;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_setNoteRouteToCashBox(uint32_t value)
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x3B;
    value = value * 100;
    CMD_PACKET.PacketData[1] = 1;
    CMD_PACKET.PacketData[2] = value & 0xff;
    CMD_PACKET.PacketData[3] = (value >> 8) & 0xff;
    CMD_PACKET.PacketData[4] = (value >> 16) & 0xff;
    CMD_PACKET.PacketData[5] = (value >> 24) & 0xff;
    CMD_PACKET.PacketData[6] = 'V';
    CMD_PACKET.PacketData[7] = 'N';
    CMD_PACKET.PacketData[8] = 'D';
    CMD_PACKET.PacketLength = 9;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_enablePayout()
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x5C;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_setNoteRouteToPayout(uint32_t value)
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x3B;
    CMD_PACKET.PacketData[1] = 0;
    config.payoutValue = value;
    value = value * 100;
    CMD_PACKET.PacketData[2] = value & 0xff;
    CMD_PACKET.PacketData[3] = (value >> 8) & 0xff;
    CMD_PACKET.PacketData[4] = (value >> 16) & 0xff;
    CMD_PACKET.PacketData[5] = (value >> 24) & 0xff;
    CMD_PACKET.PacketData[6] = 'V';
    CMD_PACKET.PacketData[7] = 'N';
    CMD_PACKET.PacketData[8] = 'D';
    CMD_PACKET.PacketLength = 9;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_enable()
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x0A;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_0;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_getNoteData()
{
    uint8_t totalChannel, temp_offset;
    uint32_t temp_chanel_value;
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x05;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_0;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
    {
        uint8_t *ssp_rx_data = &Device.Buffer.rxData[3];
        dataBus->debugPrintHex("RX Data :", ssp_rx_data, Device.Buffer.rxBufferLength);
        totalChannel = ssp_rx_data[12];
        temp_offset = totalChannel * 5 + 17;
        for (int chanel = 0; chanel < totalChannel; chanel++)
        {
            temp_chanel_value = 0;
            temp_chanel_value =
                ((uint32_t)ssp_rx_data[temp_offset + 3] << 24) |
                ((uint32_t)ssp_rx_data[temp_offset + 2] << 16) |
                ((uint32_t)ssp_rx_data[temp_offset + 1] << 8) |
                (uint32_t)ssp_rx_data[temp_offset];
            temp_offset += 4;
            _billTypeCredit[chanel] = temp_chanel_value;
            Serial.println(temp_chanel_value);
        }
        // NoteTotalChannel = totalChannel;
        return true;
    }
    return false;
}

bool NV11::_disable()
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x09;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_0;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

bool NV11::_setInhibits(int Data)
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x02;
    CMD_PACKET.PacketData[1] = Data & 0xFF;
    CMD_PACKET.PacketData[2] = (Data >> 8) & 0xFF;
    CMD_PACKET.PacketLength = 3;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_0;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

void NV11::_SetEncryptKey(uint64_t FixedKey)
{
    Device.Encypt_Key.FixedKey = FixedKey;
}

bool NV11::_encrypt_init()
{
    return dataBus->encrypt_init(&Device);
}

void NV11::setBillsMinMax(uint32_t min, uint32_t max)
{
    _min = min;
    _max = max;
}

bool NV11::init()
{
    initDone = false;
    _sync();
    _SetEncryptKey(0x0123456701234567);
    _encrypt_init();
    sprintf(config.fw, "ITL_NV11_ESSP_06");
    if (!_setHostProtocolVersion(0x06))
    {
        Serial.println("_setHostProtocolVersion failed");
        return false;
    }
    /*
        if (!_setNoteReportByChannel(0))
    {
        Serial.println("_setNoteReportByChannel failed");
        return false;
    }
    */

    if (!_getNoteData())
    {
        Serial.println("_getNoteData failed");
        return false;
    }

    int inhibit = 0;
    for (int i = 0; i < 16; i++)
    {
        if ((_billTypeCredit[i] >= _min) && (_billTypeCredit[i] <= _max))
        {
            int x = 1;
            inhibit = inhibit | (x << i);
        }
    }

    Serial.printf("inhibit 0x%X\n", inhibit);
    if (!_setInhibits(inhibit))
    {
        Serial.println("_setInhibits failed");
        return false;
    }

    if (!_setNoteRouteToPayout(10000))
    {
        Serial.println("_setNoteRouteToPayout failed");
        // return false;
    }

    if (!_enablePayout())
    {
        Serial.println("_enablePayout failed");
        // return false;
    }
    /*
        if (!_setNoteReportByChannel(1))
    {
        Serial.println("_setNoteReportByChannel failed");
        return false;
    }
    */


    if (!_enable())
    {
        Serial.println("_enable failed");
        return false;
    }
    initDone = true;
    return true;
}

void NV11::_poll()
{
    SSP_POLL_DATA6 poll_response;
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x07;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_0;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
    {
        poll_response.event_count = 0;
        int maxLenght = Device.Buffer.rxBufferLength - 2;
        for (int i = 3; i < maxLenght; i++)
        {
            for (SSP_POLL_EVENT_DEF _evnt : _pollEventList)
            {
                // Serial.printf("evnt : %02X , rx:%02X\r\n",_evnt.event,Device.Buffer.rxData[i]);
                if (_evnt.event == Device.Buffer.rxData[i])
                {
                    if (_evnt.type == SSP_POLL_ERROR)
                    {
                        DebugSerial.printf("ERROR %s : %s \r\n", _evnt.name.c_str(), _evnt.description.c_str());
                    }
                    if (_evnt.type == SSP_POLL_INFO)
                    {
                        DebugSerial.printf("INFO %s : %s \r\n", _evnt.name.c_str(), _evnt.description.c_str());
                    }
                    poll_response.events[poll_response.event_count].event = Device.Buffer.rxData[i];
                    for (int x = 0; x < _evnt.size; x++)
                    {
                        i++;
                        poll_response.events[poll_response.event_count].data.push_back(Device.Buffer.rxData[i]);
                    }
                    poll_response.event_count++;
                }
                // i+=_evnt.size;
            }
        }

        for (_SSP_POLL_HANDLER n : _PollHandlers)
        {
            for (int x = 0; x < poll_response.event_count; x++)
            {
                if (n.event == poll_response.events[x].event)
                    n.fn(poll_response.events[x]);
            }
        }
    }
    else
    {
        if (Device.Buffer.rxData[3] == SSP_CMD_RESPONSE_STATUS_KEY_NOT_SET)
        {
            Serial.println("Reinit");
            Device.Encypt_Status = ESSP_ENCRYPTION_UNSET;
        }
    }
}

void NV11::loop()
{
    if ((Device.Encypt_Status == ESSP_ENCRYPTION_UNSET) || (!initDone))
    {
        //_poll();
        init();
    }
    else
    {
        int16_t totalBillsInPayoutBefore;
        if (countBillInPayout(&totalBillsInPayoutBefore) == BV_CMD_SUCCESS)
        {
            _billCountValue = totalBillsInPayoutBefore;
        }
        delay(100);
        _poll();
    }
}

NV11::~NV11()
{
}

_BV_CONFIG_TYPE NV11::getBVConfig()
{
    return config;
}

int NV11::countBillInPayout(int16_t *billCount)
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x41;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
    {
        uint8_t *ssp_rx_data = &Device.Buffer.rxData[3];
        // dataBus->debugPrintHex("RX Data :", ssp_rx_data, Device.Buffer.rxBufferLength);
        *billCount = ssp_rx_data[1];
        return BV_CMD_SUCCESS;
    }
    return BV_CMD_CANNOT_PROCESS;
}

int NV11::emptyReclyer()
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x3F;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
    {
        return BV_CMD_SUCCESS;
    }
    return BV_CMD_CANNOT_PROCESS;
}

bool NV11::_payoutOneNote()
{
    SSP_PACKET CMD_PACKET;
    CMD_PACKET.PacketData[0] = 0x42;
    CMD_PACKET.PacketLength = 1;
    CMD_PACKET.isEncrypt = ESSP_ENCRYPTION_LEVER_1;
    dataBus->TransmitData(&Device, &CMD_PACKET);
    if (dataBus->isDone(&Device))
        return true;
    return false;
}

int NV11::payoutValue(int32_t amount, int32_t *totalPayout)
{
    int32_t _BillPayoutValue = config.payoutValue;
    int32_t billToPayout = amount / _BillPayoutValue;
    int16_t totalBillsInPayoutBefore = 0;
    uint32_t totalBillPayout = 0;
    if (countBillInPayout(&totalBillsInPayoutBefore) != BV_CMD_SUCCESS)
        return BV_CMD_CANNOT_PROCESS;
    Serial.printf("Total Bills before: %d\n", totalBillsInPayoutBefore);
    if (totalBillsInPayoutBefore < billToPayout)
    {
        if (totalBillsInPayoutBefore == 0)
        {
            *totalPayout = 0;
            return BV_CMD_SUCCESS;
        }
        else
        {
            billToPayout = totalBillsInPayoutBefore;
        }
    }
    for (int i = 0; i < billToPayout; i++)
    {
        if (_payoutOneNote())
        {
            totalBillPayout++;
            isDispening = true;
            uint32_t tick = millis();
            while (isDispening)
            {
                if ((uint32_t)(millis() - tick) > 10000)
                {
                    break;
                }
                _poll();
                delay(500);
            }
            _enable();
        }
    }
    *totalPayout = totalBillPayout * _BillPayoutValue;
    return BV_CMD_SUCCESS;
}