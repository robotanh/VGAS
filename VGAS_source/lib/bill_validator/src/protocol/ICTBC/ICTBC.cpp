#include "ICTBC.h"

ICTBC_BUS::ICTBC_BUS(HardwareSerial *uart, uint8_t adrr)
{
    this->MDB_Serial = uart;
    this->myAdrr = adrr;
}

int8_t ICTBC_BUS::SendCommand(uint8_t des, uint8_t soucre, uint8_t CMD, uint8_t *data, uint8_t dataLen, uint8_t *output, uint8_t *dataByteCount, bool revceive)
{
    static uint32_t lastSend = 0;
    uint8_t dataTmp[dataLen + 6];
    uint8_t dataTmpLenght = dataLen + 6;
    dataTmp[0] = des;
    dataTmp[1] = dataLen;
    dataTmp[2] = soucre;
    dataTmp[3] = 0x00;
    dataTmp[4] = CMD;
    int ptr = 5;
    if ((uint32_t)(millis() - lastSend) < 200)
    {
        delay(200 - (uint32_t)(millis() - lastSend));
    }
    lastSend = millis();
    for (uint8_t i = 0; i < dataLen; i++)
    {
        dataTmp[ptr] = data[i];
        ptr++;
    }
    int8_t sum = dataTmp[0];
    for (int i = 1; i < (dataTmpLenght - 1); i++)
    {
        sum = sum ^ dataTmp[i];
    }
    dataTmp[dataTmpLenght - 1] = sum; 
    while (this->MDB_Serial->available())
    {
        this->MDB_Serial->read();
        delay(1);
    }
    if (ICTBC_DEBUG)
        Serial.println("SEND: ");
    for (int i = 0; i < dataTmpLenght; i++)
    {
        if (ICTBC_DEBUG)
            Serial.printf(" %02X", dataTmp[i]);
        this->MDB_Serial->write(dataTmp, dataTmpLenght);
    }
    if (ICTBC_DEBUG)
        Serial.println();
    int8_t status = BV_CMD_RECEIVE_DONE;
    if (revceive)
    {
        status = GetData(output, dataByteCount);
    }
    return status;
}

int8_t ICTBC_BUS::GetData(uint8_t *dataBytes, uint8_t *dataByteCount)
{
    uint32_t _commandSentTime = millis();
    uint8_t index = 0;
    uint8_t packedLen = 0;
    while (true)
    {
        if (this->MDB_Serial->available())
        {
            int data = this->MDB_Serial->read();
            dataBytes[index] = data;
            index++;
            _commandSentTime = millis();
        }
        if (index > 1)
        {
            packedLen = dataBytes[1] + 6;
            if (packedLen <= index)
            {
                break;
            }
        }

        if ((uint32_t)(millis() - _commandSentTime) > 200)
        {
            return BV_CMD_RECEIVE_TIMEOUT;
        }
        delay(2);
    }
    *dataByteCount = index - 1;
    unsigned char sum = 0;
    if (ICTBC_DEBUG)
        Serial.println("RECV: ");
    for (uint8_t i = 0; i < packedLen; i++)
    {
        if (i < packedLen - 1)
        {
            sum = sum ^ dataBytes[i];
        }
        if (ICTBC_DEBUG)
            Serial.printf(" %02X", dataBytes[i]);
    }
    if (ICTBC_DEBUG)
        Serial.println();
    if (sum != dataBytes[packedLen - 1])
    {
        if (ICTBC_DEBUG)
            Serial.println("Checksum Error");
        return BV_CMD_RECEIVE_CHECKSUM_ERROR;
    }
    return BV_CMD_RECEIVE_DONE;
}
