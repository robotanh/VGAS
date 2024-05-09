#ifndef _VHITEK_VARIABLE_
#define _VHITEK_VARIABLE_

#define PIN_TOTAL 32
#define PIN_PUMP 12
#define PIN_VAL1 27
#define PIN_VAL2 14
#define PIN_ENC1 34
#define PIN_ENC2 33
#define PIN_MODE 15

#define MQTT_BROKER "159.223.48.4"
#define MQTT_PORT 1884

#define ARDUINOJSON_USE_LONG_LONG 1
#include <Arduino.h>
#include "ArduinoJson.h"
#include "WiFi.h"
#include "SPI.h"
#include <stdlib.h>
#include <qrcode.h>
#include <BillValidator.h>
#include <IoAbstraction.h>
#include <TaskManagerIO.h>
#include <KeyboardManager.h>
#include <IoAbstractionWire.h>
#include <U8g2lib.h>
#include "SparkFun_External_EEPROM.h"
#include "RTClib.h"
#include "time.h"
#include <sys/time.h>
#include "AsyncJson.h"
#include <ESPAsyncWebServer.h>
#include "ArduinoHttpClient.h"
#include <esp32fota.h>
#include "mbedtls/md.h"
#include "Adafruit_Thermal.h"
#include "PubSubClient.h"
#include "PN532_HSU.h"

#define API "159.223.48.4:83"

#define VHITEK_FW_VERSION "v3-1.1.VGA"
#define BOARD_TYPE "BASIC_MAIN_V3_VGAS"
#define TOTAL_CHANGE 1

#include <server_conn.h>
#define LCD_RESET 4	 // RST on LCD
#define LCD_CS 2	 // RS on LCD
#define LCD_CLOCK 19 // E on LCD
#define LCD_DATA 21	 // R/W on LCD
#define U8_Width 128
#define U8_Height 64

#define MENUMAINIDX 0
#define MENUSETPRICEIDX 1
#define MENUQUERYPRICEIDX 1
#define MENUTOTALSALESIDX 2
#define MENUTESTMOTORIDX 3
#define MENULINKAGESLOTIDX 4
#define MENUBILLINFOIDX 5
#define MENUAPPMODEIDX 6
#define MENUCLEARERRORIDX 7
#define MENUMACHINEIDIDX 8
#define MENUVALIDATORCONFIGIDX 9
#define MENUDROPSENSORIDX 10
#define MENUCLEARSALESDATAIDX 11
#define MENUFACTORYRESETIDX 12
#define MENUENTERPASSWORDIDX 13
#define MENUCLEARLINKAGEIDX 14
#define MENUQUERYLINKAGEIDX 15
#define MENUSINGLEVENDIDX 16
#define MENUSETDATETIMEIDX 17
#define MENUSETTEMPIDX 18
#define MENUTESTCARDRFIDIDX 19
#define MENUSETNOTEMINMAX 20
#define MENUSETSTACKERNOT 21
#define totalMenuELM 30
template <class T, size_t N>
char (&helper(T (&)[N]))[N];

#define arraysize(array) (sizeof(helper(array)))
namespace VhitekVending
{
	extern U8G2_ST7920_128X64_F_HW_SPI u8g2;
	extern MatrixKeyboardManager keyboard;
	extern MultiIoAbstractionRef ExternalIO;
	extern char apSSID[50];

	struct struct_vmSaleDate
	{
		uint8_t day;
		uint8_t month;
		uint8_t year;
		uint8_t hour;
		uint8_t min;
		uint8_t seconds;
	};

	struct struct_Date
	{
		uint8_t day;
		uint8_t month;
		uint16_t year;
		uint8_t hour;
		uint8_t min;
		uint8_t seconds;
	};

	typedef struct
	{
		uint32_t maxValue;
		uint32_t minValue;
		uint16_t StackerCapacity;
		uint16_t hotWaterTimer;
		uint16_t enableEscrow;
		uint32_t payoutValue;
		uint8_t timerQR;
		uint8_t CheckCam;
	} ValidatorConfigType;

	struct VgasInfoType
	{
		uint8_t ID;
		uint32_t price;
		uint32_t partId;
		uint8_t fuelType;
		char fuelName[20];
		uint64_t total;
		uint64_t totalMoneySale;
		uint32_t totalSale;
	};

	typedef struct
	{
		VgasInfoType vgasInfo;
		char typeBill;
		char adminPWD[9];
		char operatorPWD[9];
		uint32_t totalMoneysale;
		uint32_t totalMoneysale1;
		uint32_t totalMoneysale2;
		ValidatorConfigType extraConfig;
		char ssidName[20];
		char ssidPWD[20];
		char keyserver[9];
		char singleVend;
		uint16_t cs;
	} SettingsType;

	struct struct_vmSaleInfo
	{
		uint32_t totalSale;
		uint32_t totalMoneysale;
		struct_vmSaleDate saleStartDay;
		struct_vmSaleDate saleStartMonth;
		uint32_t totalSaleDayStart;
		uint32_t totalMoneySaleDayStart;
		uint32_t totalSaleMonthStart;
		uint32_t totalMoneySaleMonthStart;
	};

	typedef struct struct_DateSaleInfo
	{
		struct_vmSaleDate date;
		uint32_t totalSale;
		uint32_t TotalMoneySale;
	};

	struct struct_vmDateSaleInfo
	{
		struct_DateSaleInfo saleInfo[7];
		uint16_t checksum;
	};

	typedef struct
	{
		uint8_t SyncByte;
		struct_vmSaleDate date;
		uint64_t totalCashBox;
		uint16_t checkSum;
	} CashBoxType;

	typedef struct
	{
		bool syncByte;
		uint32_t ID;
		uint8_t TrangThaiCoBom;
		int8_t IdVoiBom;
		bool trangThaiIn;
		bool trangThaiSS;
		bool trangThaiXoaToTal;
		struct_vmSaleDate date;
		uint32_t partId; // total current < totalLast ==> partID++;
		char cardId[15];
		uint32_t IdLanBom;
		uint32_t SoLitBomDuoc;
		uint32_t GiaXangDau;
		uint32_t ThanhTien;
		uint32_t total;
		uint16_t LoaiXangDau;
		int8_t paymentTerm; // paymentType
		// uint32_t IdLanBomCuoi;
		// uint32_t giaLanBomCuoi;
		// uint32_t soLitBanLanCuoi;
		// uint64_t soTienBanLanCuoi;
		// uint32_t hanMucTheoTien;
		// uint32_t hanMucTheoLit;
		uint16_t checkSum;
	} VgasTransactionType;

	typedef struct
	{
		bool status;
		uint8_t ID;
		uint32_t price;
		struct_Date date;
		uint16_t cs;
	} VgasSettingPriceType;

	void begin();
	bool accessI2CBus(std::function<void()> &&Callback, uint32_t timeout);
	bool accessI2C1Bus(std::function<void()> &&Callback, uint32_t timeout);
	bool accessSPIBus(std::function<void()> &&Callback, uint32_t timeout);
	bool accessMDBBus(std::function<void()> &&Callback, uint32_t timeout);
	bool accessSerial1Bus(std::function<void()> &&Callback, uint32_t timeout);
	unsigned short cal_crc_loop_CCITT_A(short l, unsigned char *p);
	char *ultoa(unsigned long val, char *s);
	void beep_buzz(int t);

	namespace Display
	{
		void begin();
	}

	namespace Config
	{
		extern uint32_t lastHistoryCashBoxEEPromIdx;
		extern bool feePayer;
		extern RTC_DS1307 rtc;
		void begin();
		bool loadRTC();
		bool loadSettings();
		bool saveSettings();
		bool restoreDefault();
		bool toggleSingleVendConfig();
		bool toggleEnableEscrowConfig();
		bool isEscrowDisable();
		bool setPayoutValue(uint32_t billValue);
		SettingsType getSettings();
		struct_vmDateSaleInfo getDateSalesInfo();
		bool SetTimerQR(uint8_t timeQR);
		void loop();
		bool setBillType(char usingPulse);
		char getBillType();
		bool initPartId();
		uint32_t getPartId();
		bool setPartId(uint32_t partID);
		bool setFuelType(uint8_t fuelType, char *fuelName);
		String getFuelName();
		uint8_t getFuelType();
	}

	namespace Transactions
	{
		void begin();
		uint16_t getMaxTransactionIdx();
		uint32_t getCurrentTransID();
		uint16_t getCurrentTransIdx();
		uint32_t getTotalTransUnSync();
		void setTotalTransUnSync(uint32_t totalUnsync);
		void minusTotalTransUnSync();
		String toTransJson(VgasTransactionType data);
		bool addTransaction(VgasTransactionType data);
		VgasTransactionType readEeprom(uint16_t idx, bool *dataVaid);
		bool writeToEpprom(VgasTransactionType data, uint16_t idx);
		void clearVmTransactionEeprom();

	}

	namespace Process
	{
		extern int newCashBoxIdx;
		extern uint32_t count;
		extern uint32_t countKey;
		extern char currentTagStr[15];
		extern uint32_t currentTag;
		extern VgasTransactionType currentData;
		extern VgasSettingPriceType updatesPrice;
		extern VgasTransactionType lastTranctionData;
		void taskSyncData(void *parameter);
		void taskVgas(void *parameter);
		void VgasUpdateRealTime(void *parameter);
		void updatePumpData(VgasTransactionType data);
		void WifiBegin();
		void callback(char *topic, byte *payload, unsigned int length);
	}
} // namespace VhitekVending
#endif