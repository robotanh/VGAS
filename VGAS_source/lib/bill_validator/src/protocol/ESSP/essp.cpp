#include "essp.h"
#include "rand.h"
#include "Encryption.h"

essp::essp(HardwareSerial *uart, SSP_MODE MODE)
{
    SSP_Serial = uart;
}

void essp::debugPrintHex(String dbgName, uint8_t *Data, uint8_t len)
{
    return;
    DebugSerial.println(dbgName);
    for (uint8_t i = 0; i < len; i++)
    {
        DebugSerial.printf(" %02X", Data[i]);
    }
    DebugSerial.println();
}

void essp::begin()
{
}

bool essp::isDone(SSP_Device *Device)
{
    if (Device->Buffer.txBufferLength > 3)
    {
        if (Device->Buffer.rxData[3] == 0xF0)
            return true;
    }
    return false;
}

void essp::TransmitData(SSP_Device *Device, SSP_PACKET *Data)
{
    complie_packed(Device, Data);
    Transmit(Device);
    Receive(Device);
}

bool essp::Transmit(SSP_Device *Device)
{
    SSP_Serial->write(Device->Buffer.txData, Device->Buffer.txBufferLength);
    return true;
}

bool essp::encrypt_init(SSP_Device *Device)
{
    uint8_t i;
    uint64_t swap;
    uint64_t generatorKey = GeneratePrime();
    uint64_t modulusKey = GeneratePrime();
    uint64_t hostRandom = GeneratePrime();
    uint64_t hostInterKey, slaveInterkey;
    uint64_t Key;
    Device->ePackedCount = 0;
    if (generatorKey > modulusKey)
    {
        swap = generatorKey;
        generatorKey = modulusKey;
        modulusKey = swap;
    }
    hostInterKey = XpowYmodN(generatorKey, hostRandom, modulusKey);
    if (!encrypt_set_generator(Device, generatorKey))
        return false;
    if (!encrypt_set_modulus(Device, modulusKey))
        return false;
    slaveInterkey = encrypt_key_exchange(Device, hostInterKey);
    if (slaveInterkey != 0)
    {
        Key = XpowYmodN(slaveInterkey, hostRandom, modulusKey);
        for (i = 0; i < 8; i++)
        {
            Device->Encypt_Key.EncryptKey[i] = byteOf(Device->Encypt_Key.FixedKey, i);
        }
        for (i = 0; i < 8; i++)
        {
            Device->Encypt_Key.EncryptKey[i + 8] = byteOf(Key, i);
        }
        Device->Encypt_Status = ESSP_ENCRYPTION_SET;
        return true;
    }
    return false;
}

bool essp::encrypt_set_modulus(SSP_Device *Device, uint64_t modulus)
{
    uint8_t i;
    SSP_PACKET CMD;
    CMD.PacketLength = 9;
    CMD.PacketData[0] = SSP_CMD_SET_MODULUS;
    for (i = 0; i < 8; ++i)
        CMD.PacketData[1 + i] = (unsigned char)(modulus >> (i * 8));
    TransmitData(Device, &CMD);
    if (Device->Buffer.NewResponse)
    {
        if (Device->Buffer.rxData[3] == 0xF0)
        {
            return true;
        }
    }
    return false;
}

bool essp::encrypt_set_generator(SSP_Device *Device, uint64_t generator)
{
    uint8_t i;
    SSP_PACKET CMD;
    CMD.PacketLength = 9;
    CMD.PacketData[0] = SSP_CMD_SET_GENERATOR;
    for (i = 0; i < 8; ++i)
        CMD.PacketData[1 + i] = (unsigned char)(generator >> (i * 8));
    TransmitData(Device, &CMD);
    if (Device->Buffer.NewResponse)
    {
        if (Device->Buffer.rxData[3] == 0xF0)
        {
            return true;
        }
    }
    return false;
}

uint64_t essp::encrypt_key_exchange(SSP_Device *Device, uint64_t hostInterKey)
{
    uint8_t i;
    uint64_t SlaveInterKey = 0;
    SSP_PACKET CMD;
    CMD.PacketLength = 9;
    CMD.PacketData[0] = SSP_CMD_KEY_EXCHANGE;
    Device->ePackedCount = 0;
    for (i = 0; i < 8; ++i)
        CMD.PacketData[1 + i] = (unsigned char)(hostInterKey >> (i * 8));
    TransmitData(Device, &CMD);
    if (Device->Buffer.NewResponse)
    {
        if (Device->Buffer.rxData[3] == 0xF0)
        {
            for (i = 0; i < 8; ++i)
                SlaveInterKey += ((long long)(Device->Buffer.rxData[4 + i])) << (8 * i);
            return SlaveInterKey;
        }
    }
    return 0;
}

bool essp::encrypt_packed(SSP_Device *Device, SSP_PACKET *Data)
{
    uint8_t ePackedLenght = 7 + Data->PacketLength;
    uint8_t PackedFillLenght = 0;
    uint8_t i, tmpData[255];
    uint16_t crc;
    if (ePackedLenght % 16 != 0)
    {
        PackedFillLenght = 16 - (ePackedLenght % 16);
    }
    ePackedLenght += PackedFillLenght;
    tmpData[0] = Data->PacketLength;
    for (i = 0; i < 4; i++)
    {
        tmpData[1 + i] = (uint8_t)((Device->ePackedCount >> (8 * i)) & 0xFF);
    }
    for (i = 0; i < Data->PacketLength; i++)
        tmpData[i + 5] = Data->PacketData[i];
    for (i = 0; i < PackedFillLenght; i++)
        tmpData[5 + Data->PacketLength + i] = (unsigned char)(rand() % 255);
    crc = cal_crc_loop_CCITT_A(ePackedLenght - 2, tmpData, CRC_SSP_SEED, CRC_SSP_POLY);
    tmpData[ePackedLenght - 2] = (unsigned char)(crc & 0xFF);
    tmpData[ePackedLenght - 1] = (unsigned char)((crc >> 8) & 0xFF);
    if (aes_encrypt(C_AES_MODE_ECB, Device->Encypt_Key.EncryptKey, C_MAX_KEY_LENGTH, NULL, 0, tmpData, &Data->PacketData[1], ePackedLenght) != E_AES_SUCCESS)
        return false;
    Data->PacketLength = ePackedLenght + 1;
    Data->PacketData[0] = SSP_STEX;
    Device->ePackedCount++;
    return true;
}

bool essp::Receive(SSP_Device *Device)
{
    uint32_t txTime, slaveCount;
    uint8_t buffer;
    uint16_t crcR;
    uint8_t encryptLength, i;
    uint8_t tData[255];
    txTime = millis();
    while (!Device->Buffer.NewResponse)
    {
        if (millis() - txTime > 200)
        {
            return false;
        }
        while (SSP_Serial->available() > 0)
        {
            buffer = SSP_Serial->read();
            SSPDataIn(buffer, Device);
        }
    }
    debugPrintHex("RX Data :", Device->Buffer.rxData, Device->Buffer.rxBufferLength);

    if (Device->Buffer.rxData[3] == SSP_STEX)
    { /* check for encrpted packet    */
        encryptLength = Device->Buffer.rxData[2] - 1;
        if (aes_decrypt(C_AES_MODE_ECB, Device->Encypt_Key.EncryptKey, C_MAX_KEY_LENGTH, NULL, 0, &Device->Buffer.rxData[4], &Device->Buffer.rxData[4], encryptLength) != E_AES_SUCCESS)
            return 0;
        /* check the checsum    */
        crcR = cal_crc_loop_CCITT_A(encryptLength - 2, &Device->Buffer.rxData[4], CRC_SSP_SEED, CRC_SSP_POLY);
        if ((uint8_t)(crcR & 0xFF) != Device->Buffer.rxData[Device->Buffer.rxData[2] + 1] || (uint8_t)((crcR >> 8) & 0xFF) != Device->Buffer.rxData[Device->Buffer.rxData[2] + 2])
        {
            return false;
        }
        /* check the slave count against the host count  */
        slaveCount = 0;
        for (i = 0; i < 4; i++)
            slaveCount += (uint16_t)(Device->Buffer.rxData[5 + i]) << (i * 8);
        /* no match then we discard this packet and do not act on it's info  */
        if (slaveCount != Device->ePackedCount)
        {
            Device->Encypt_Status = ESSP_ENCRYPTION_UNSET;
            return false;
        }

        /* restore data for correct decode  */
        Device->Buffer.rxBufferLength = Device->Buffer.rxData[4] + 5;
        tData[0] = Device->Buffer.rxData[0];
        tData[1] = Device->Buffer.rxData[1];
        tData[2] = Device->Buffer.rxData[4];
        for (i = 0; i < Device->Buffer.rxData[4]; i++)
            tData[3 + i] = Device->Buffer.rxData[9 + i];
        crcR = cal_crc_loop_CCITT_A(Device->Buffer.rxBufferLength - 3, &tData[1], CRC_SSP_SEED, CRC_SSP_POLY);
        tData[3 + Device->Buffer.rxData[4]] = (uint8_t)(crcR & 0xFF);
        tData[4 + Device->Buffer.rxData[4]] = (uint8_t)((crcR >> 8) & 0xFF);
        for (i = 0; i < Device->Buffer.rxBufferLength; i++)
            Device->Buffer.rxData[i] = tData[i];
        debugPrintHex("RX Decrypt Data :", Device->Buffer.rxData, Device->Buffer.rxBufferLength);

        /* for decrypted resonse with encrypted command, increment the counter here  */
        //	if(!cmd->EncryptionStatus)
        // encPktCount[cmd->SSPAddress]++;
    }

    return true;
}

void essp::SSPDataIn(uint8_t RxChar, SSP_Device *Device)
{
    unsigned short crc;

    if (RxChar == SSP_STX && Device->Buffer.rxPtr == 0)
    {
        // packet start
        Device->Buffer.rxData[Device->Buffer.rxPtr++] = RxChar;
    }
    else
    {
        // if last byte was start byte, and next is not then
        // restart the packet
        if (Device->Buffer.CheckStuff == 1)
        {
            if (RxChar != SSP_STX)
            {
                Device->Buffer.rxData[0] = SSP_STX;
                Device->Buffer.rxData[1] = RxChar;
                Device->Buffer.rxPtr = 2;
            }
            else
                Device->Buffer.rxData[Device->Buffer.rxPtr++] = RxChar;
            // reset stuff check flag
            Device->Buffer.CheckStuff = 0;
        }
        else
        {
            // set flag for stuffed byte check
            if (RxChar == SSP_STX)
                Device->Buffer.CheckStuff = 1;
            else
            {
                // add data to packet
                Device->Buffer.rxData[Device->Buffer.rxPtr++] = RxChar;
                // get the packet length
                if (Device->Buffer.rxPtr == 3)
                    Device->Buffer.rxBufferLength = Device->Buffer.rxData[2] + 5;
            }
        }
        // are we at the end of the packet
        if (Device->Buffer.rxPtr == Device->Buffer.rxBufferLength)
        {
            // is this packet for us ??
            if ((Device->Buffer.rxData[1] & SSP_STX) == Device->ID)
            {
                // is the checksum correct
                crc = cal_crc_loop_CCITT_A(Device->Buffer.rxBufferLength - 3, &Device->Buffer.rxData[1], CRC_SSP_SEED, CRC_SSP_POLY);
                if ((unsigned char)(crc & 0xFF) == Device->Buffer.rxData[Device->Buffer.rxBufferLength - 2] && (unsigned char)((crc >> 8) & 0xFF) == Device->Buffer.rxData[Device->Buffer.rxBufferLength - 1])
                    Device->Buffer.NewResponse = 1; /* we have a new response so set flag  */
            }
            // reset packet
            Device->Buffer.rxPtr = 0;
            Device->Buffer.CheckStuff = 0;
        }
    }
}

bool essp::complie_packed(SSP_Device *Device, SSP_PACKET *Data)
{
    int i, j;
    uint16_t crc;
    uint8_t tBuffer[255];
    Device->Buffer.rxPtr = 0;
    Device->Buffer.NewResponse = 0;
    for (i = 0; i < 255; i++)
        Device->Buffer.rxData[i] = 0x00;
    if (Data->PacketData[0] == SSP_CMD_SYNC)
        Device->Sync = true;

    debugPrintHex("TX Data :", Data->PacketData, Data->PacketLength);

    if (Data->isEncrypt == ESSP_ENCRYPTION_LEVER_1)
    {
        if (!encrypt_packed(Device, Data))
            return false;
    }

    Device->Buffer.CheckStuff = 0;
    Device->Buffer.SSPAddress = Device->ID;
    if (Device->Sync)
        Device->Buffer.SSPAddress = Device->Buffer.SSPAddress | 0x80;
    else
        Device->Buffer.SSPAddress = Device->Buffer.SSPAddress & 0x7F;
    Device->Buffer.rxPtr = 0;
    Device->Buffer.txPtr = 0;
    Device->Buffer.rxBufferLength = 3;
    Device->Buffer.txBufferLength = Data->PacketLength + 5; /* the full ssp packet length   */
    Device->Buffer.txData[0] = 0x7F;                        /* ssp packet start   */
    Device->Buffer.txData[1] = Device->Buffer.SSPAddress;   /* the address/seq bit */
    Device->Buffer.txData[2] = Data->PacketLength;          /* the data length only (always > 0)  */
    for (i = 0; i < Data->PacketLength; i++)                /* add the command data  */
        Device->Buffer.txData[3 + i] = Data->PacketData[i];
    crc = cal_crc_loop_CCITT_A(Device->Buffer.txBufferLength - 3, &Device->Buffer.txData[1], CRC_SSP_SEED, CRC_SSP_POLY);
    Device->Buffer.txData[3 + Data->PacketLength] = (uint8_t)(crc & 0xFF);
    Device->Buffer.txData[4 + Data->PacketLength] = (uint8_t)((crc >> 8) & 0xFF);
    j = 0;
    tBuffer[j++] = Device->Buffer.txData[0];
    for (i = 1; i < Device->Buffer.txBufferLength; i++)
    {
        tBuffer[j] = Device->Buffer.txData[i];
        if (Device->Buffer.txData[i] == SSP_STX)
        {
            tBuffer[++j] = SSP_STX; /* SSP_STX found in data so add another to 'stuff it'  */
        }
        j++;
    }
    for (i = 0; i < j; i++)
        Device->Buffer.txData[i] = tBuffer[i];
    Device->Buffer.txBufferLength = j;
    Device->Sync = !Device->Sync;
    return true;
}

unsigned short cal_crc_loop_CCITT_A(short l, unsigned char *p, unsigned short seed, unsigned short cd)
{
    int i, j;
    unsigned short crc = seed;

    for (i = 0; i < l; ++i)
    {
        crc ^= (p[i] << 8);
        for (j = 0; j < 8; ++j)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ cd;
            else
                crc <<= 1;
        }
    }
    return crc;
}
