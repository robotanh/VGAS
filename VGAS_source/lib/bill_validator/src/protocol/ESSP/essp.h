#ifndef _ITL_ESSP_H_
#define _ITL_ESSP_H_
#include "Arduino.h"
#ifndef DebugSerial
#define DebugSerial Serial
#endif

#define CRC_SSP_SEED 0xFFFF
#define CRC_SSP_POLY 0x8005

#define SSP_STX 0x7F
#define SSP_STEX 0x7E
#define SSP_CMD_SET_ROUTING 0x3B
#define SSP_CMD_SYNC 0x11
#define SSP_CMD_HOST_PROTOCOL 0x06
#define SSP_CMD_SETUP_REQUEST 0x05
#define SSP_CMD_ENABLE 0x0A
#define SSP_CMD_ENABLE_PAYOUT_DEVICE 0x5C
#define SSP_CMD_SET_INHIBITS 0x02
#define SSP_CMD_POLL 0x07
#define SSP_CMD_RESET 0x01
#define SSP_CMD_DISABLE_PAYOUT_DEVICE 0x5B
#define SSP_CMD_DISABLE 0x09
#define SSP_CMD_SET_GENERATOR 0x4A
#define SSP_CMD_SET_MODULUS 0x4B
#define SSP_CMD_KEY_EXCHANGE 0x4C

enum SSP_CMD_RESPONSE_STATUS
{
    SSP_CMD_RESPONSE_STATUS_TIMEOUT = 0x00,
    SSP_CMD_RESPONSE_STATUS_OK = 0xF0,
    SSP_CMD_RESPONSE_STATUS_UNKNOW = 0xF2,
    SSP_CMD_RESPONSE_STATUS_NO_PARAMS,
    SSP_CMD_RESPONSE_STATUS_PARAMS_OUT_OF_RANGER,
    SSP_CMD_RESPONSE_STATUS_CAN_NOT_PROCESS,
    SSP_CMD_RESPONSE_STATUS_SW_ERROR,
    SSP_CMD_RESPONSE_STATUS_FAIL,
    SSP_CMD_RESPONSE_STATUS_KEY_NOT_SET = 0xFA
};

enum SSP_MODE
{
    Master = 1,
    Slave = 2
};

enum ESSP_ENCRYPTION
{
    ESSP_ENCRYPTION_UNSET = 0,
    ESSP_ENCRYPTION_SET = 1,
    ESSP_ENCRYPTION_PAIR,
};

enum ESSP_ENCRYPTION_LEVER
{
    ESSP_ENCRYPTION_LEVER_0 = 0,
    ESSP_ENCRYPTION_LEVER_1 = 1
};

typedef struct
{
    uint64_t FixedKey;
    uint8_t EncryptKey[16];
} SSP_FULL_KEY;

typedef struct
{
    uint16_t packetTime;
    uint8_t PacketLength;
    uint8_t PacketData[255];
    ESSP_ENCRYPTION_LEVER isEncrypt;
} SSP_PACKET;

typedef struct
{
    uint8_t txData[255];
    uint8_t txPtr;
    uint8_t rxData[255];
    uint8_t rxPtr;
    uint8_t txBufferLength;
    uint8_t rxBufferLength;
    uint8_t SSPAddress;
    uint8_t NewResponse;
    uint8_t CheckStuff;
} SSP_TX_RX_Data;

typedef struct
{
    uint16_t ID;
    uint16_t Poll_Time;
    uint16_t PackedTimeout;
    uint32_t ePackedCount = 0;
    bool Sync;
    ESSP_ENCRYPTION_LEVER Encypt_Lever;
    ESSP_ENCRYPTION Encypt_Status;
    SSP_FULL_KEY Encypt_Key;
    SSP_TX_RX_Data Buffer;
} SSP_Device;
unsigned short cal_crc_loop_CCITT_A(short l, unsigned char *p, unsigned short seed, unsigned short cd);
unsigned long long GeneratePrime(void);
unsigned char MillerRabin(long long n, long long trials);
unsigned char IsItPrime(long long n, long long a);
long long XpowYmodN(long long x, long long y, long long N);
unsigned long long GenerateRandomNumber(void);

class essp
{
private:
    Stream *SSP_Serial;
    bool Transmit(SSP_Device *Device);
    bool Receive(SSP_Device *Device);
    bool encrypt_set_modulus(SSP_Device *Device, uint64_t modulus);
    bool encrypt_set_generator(SSP_Device *Device, uint64_t generator);
    uint64_t encrypt_key_exchange(SSP_Device *Device, uint64_t hostInterKey);
    bool complie_packed(SSP_Device *Device, SSP_PACKET *Data);
    bool encrypt_packed(SSP_Device *Device, SSP_PACKET *Data);
    void SSPDataIn(uint8_t RxChar, SSP_Device *Device);

public:
    essp(HardwareSerial *uart, SSP_MODE MODE);
    void begin();
    void debugPrintHex(String dbgName, uint8_t *Data, uint8_t len);
    bool isDone(SSP_Device *Device);
    void TransmitData(SSP_Device *Device, SSP_PACKET *Data);
    bool encrypt_init(SSP_Device *Device);
    
};
#endif