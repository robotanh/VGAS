#ifndef _NV11_H_
#define _NV11_H_
#include "BillValidator.h"
#include "validatorHelper.h"
#include "protocol/ESSP/essp.h"
#include <vector>

enum SSP_POLL_TYPE
{
    SSP_POLL_INFO = 0,
    SSP_POLL_WARNING,
    SSP_POLL_ERROR
};

#define SSP_POLL_RESET 0xF1
#define SSP_POLL_READ 0xEF   // next byte is channel (0 for unknown)
#define SSP_POLL_CREDIT 0xEE // next byte is channel
#define SSP_POLL_REJECTING 0xED
#define SSP_POLL_REJECTED 0xEC
#define SSP_POLL_STACKING 0xCC
#define SSP_POLL_STACKED 0xEB
#define SSP_POLL_SAFE_JAM 0xEA
#define SSP_POLL_UNSAFE_JAM 0xE9
#define SSP_POLL_DISABLED 0xE8
#define SSP_POLL_FRAUD_ATTEMPT 0xE6 // next byte is channel
#define SSP_POLL_STACKER_FULL 0xE7
#define SSP_POLL_CLEARED_FROM_FRONT 0xE1
#define SSP_POLL_CLEARED_INTO_CASHBOX 0xE2
#define SSP_POLL_BARCODE_VALIDATE 0xE5
#define SSP_POLL_BARCODE_ACK 0xD1
#define SSP_POLL_CASH_BOX_REMOVED 0xE3
#define SSP_POLL_CASH_BOX_REPLACED 0xE4
#define SSP_POLL_DISPENSING 0xDA
#define SSP_POLL_DISPENSED 0xD2
#define SSP_POLL_JAMMED 0xD5
#define SSP_POLL_HALTED 0xD6
#define SSP_POLL_FLOATING 0xD7
#define SSP_POLL_FLOATED 0xD8
#define SSP_POLL_TIMEOUT 0xD9
#define SSP_POLL_INCOMPLETE_PAYOUT 0xDC
#define SSP_POLL_INCOMPLETE_FLOAT 0xDD
#define SSP_POLL_CASHBOX_PAID 0xDE
#define SSP_POLL_COIN_CREDIT 0xDF
#define SSP_POLL_EMPTYING 0xC2
#define SSP_POLL_SMART_EMPTYING 0xB3
#define SSP_POLL_EMPTY 0xC3
#define SSP_POLL_COINS_LOW 0xD3
#define SSP_POLL_COINS_EMPTY 0xD4
#define SSP_POLL_PAYOUT_OUT_OF_SERVICE 0xC6

typedef struct
{
    uint8_t event;
    int8_t size;
    SSP_POLL_TYPE type;
    String name;
    String description;
} SSP_POLL_EVENT_DEF;

typedef struct
{
    unsigned char event;
    std::vector<uint8_t> data;
} SSP_POLL_EVENT_DATA;

typedef struct
{
    SSP_POLL_EVENT_DATA events[20];
    uint8_t event_count;
} SSP_POLL_DATA6;

typedef std::function<void(SSP_POLL_EVENT_DATA)> _OnEventCallbackHandler;

typedef struct
{
    _OnEventCallbackHandler fn;
    unsigned char event;
} _SSP_POLL_HANDLER;

#define ICTBC_ERROR_PACKET_LENGHT -1
#define ICTBC_ERROR_PACKET_CHECKSUM -2
#define ICTBC_ERROR_PACKET_TIMEOUT -3
#define ICTBC_ERROR_NACK -4

#define ICTBC_RECEIVE_PACKET_DATA_OFFSET 5
#define ICTBC_RECEIVE_PACKET_DATA_ACK 1
#define ICTBC_RECEIVE_PACKET_DATA_REPLYCMD 0
#define ICTBC_RECEIVE_PACKET_DATA_NACK 2

#define ICTBC_POLL_EVENT_BILL_ESCROW 0
#define ICTBC_POLL_EVENT_BILL_STACKED_CASHBOX 1
#define ICTBC_POLL_EVENT_BILL_STACKED_RECYCLER 2
#define ICTBC_POLL_EVENT_BILL_WAITING 3
#define ICTBC_POLL_EVENT_BILL_RETURNED 4
#define ICTBC_POLL_EVENT_BILL_TO_CASHBOX 5
#define ICTBC_POLL_EVENT_FAULT 0xff
class NV11 : public BillValidatorBaseClass
{
private:
    essp *dataBus;
    bool isInitDone;
    int lastMessageCnt;
    unsigned int _featureLevel;
    unsigned int _countryCode;
    unsigned int _billScaleFactor;
    unsigned int _decimalPlaces;
    unsigned int _stackerCapacity;
    unsigned int _payoutCapacity;
    unsigned int _payoutBillPostion;
    unsigned int _billSecurityLevels;
    unsigned int _totalBillInPayout;
    bool _escrowEnabled;
    int32_t _min = 0;
    int32_t _max = 0;
    int32_t _billHoldingVal = 0;
    int _billCountValue;
    int32_t _billTypeCredit[16];
    int32_t _billsAccept[16];
    _BV_CONFIG_TYPE config;
    SSP_Device Device;
    bool initDone = false;
    void _poll();
    void _setAcceptNotes(int32_t *notes, uint8_t totalNotes);
    bool _sync();
    bool _setHostProtocolVersion(uint8_t version);
    bool _enable();
    bool _getNoteData();
    bool _disable();
    bool isDispening = false;
    bool isFillReclyer = false;
    bool _setInhibits(int Data);
    bool _encrypt_init();
    void _SetEncryptKey(uint64_t FixedKey);
    void _addPollEvent(uint8_t event, SSP_POLL_TYPE pollType, String name, int8_t size, String description);
    void _on(uint8_t pollEvent, _OnEventCallbackHandler fn);
    bool _setNoteReportByChannel(uint8_t version);
    bool _setNoteRouteToCashBox(uint32_t value);
    bool _setNoteRouteToPayout(uint32_t value);
    bool _enablePayout();
    bool _payoutOneNote();
    void begin();
    std::vector<SSP_POLL_EVENT_DEF> _pollEventList;
    std::vector<_SSP_POLL_HANDLER> _PollHandlers;

public:
    NV11(essp *bus);
    virtual ~NV11();
    virtual bool init();
    virtual void loop();
    virtual void setBillsMinMax(uint32_t min, uint32_t max);
    virtual int countBillInPayout(int16_t *billCount);
    virtual _BV_CONFIG_TYPE getBVConfig();
    virtual int emptyReclyer();
    virtual int getBillCountValue();
    // virtual bool init();
    virtual int payoutValue(int32_t amount, int32_t *totalPayout);
};
#endif