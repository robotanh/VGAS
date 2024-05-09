#include "VHITEK_Var.h"

namespace VhitekVending
{
    namespace Config
    {
        ExternalEEPROM vmConfigEeprom;
        SettingsType settings;
        struct_vmSaleInfo saleInfo;
        struct_vmDateSaleInfo dateSaleInfo;
        uint32_t lastHistoryCashBoxEEPromIdx = 0;
        uint32_t maxHistoryCashBoxEEPromIdx = (64000 - 50000) / sizeof(CashBoxType) - 10;

        RTC_DS1307 rtc;
    
        /*------------------------------ Settings ------------------------------*/
        int16_t sumCalcSettingsType(SettingsType data)
        {
            int len = sizeof(data);
            uint8_t buffer[200];
            uint8_t *ptr = (uint8_t *)&data;
            uint8_t dataSize = 0;
            for (int i = 0; i < len; i++)
            {
                if ((uint32_t)&data.cs <= (uint32_t)&ptr[i])
                {
                    break;
                }
                buffer[i] = ptr[i];
                dataSize++;
            }
            uint16_t sum = cal_crc_loop_CCITT_A(dataSize, buffer);
            return sum;
        }

        SettingsType readSettingsType(bool *dataVaid)
        {
            uint16_t eepromPtr = 2;
            SettingsType tmpData;
            vmConfigEeprom.get(eepromPtr, tmpData);
            uint16_t sum = sumCalcSettingsType(tmpData);
            *dataVaid = false;
            if (sum == tmpData.cs)
            {
                *dataVaid = true;
            }
            return tmpData;
        }

        bool writeSettingsType(SettingsType data)
        {
            SettingsType tmp;
            uint16_t eepromPtr = 2;
            data.cs = sumCalcSettingsType(data);
            for (int i = 0; i < 5; i++)
            {
                vmConfigEeprom.put(eepromPtr, data);
                bool dataVaid = false;
                tmp = readSettingsType(&dataVaid);
                if (dataVaid)
                {
                    settings = tmp;
                    return dataVaid;
                }
            }
            return false;
        }

        bool saveSettings()
        {
            return writeSettingsType(settings);
        }

        bool loadSettings()
        {
            int settingsLoadRetry = 0;
            while (1)
            {
                SettingsType tmp;
                bool dataVail = false;
                tmp = readSettingsType(&dataVail);
                if (dataVail)
                {
                    settings = tmp;
                    if (settings.extraConfig.timerQR == 0)
                    {
                        settings.extraConfig.timerQR = 3;
                        saveSettings();
                    }
                    break;
                }
                else
                {
                    settingsLoadRetry++;
                }
                if (settingsLoadRetry >= 3)
                {
                    uint8_t *ptr1 = (uint8_t *)&settings;
                    for (int i = 0; i < sizeof(settings); i++)
                    {
                        ptr1[i] = 0;
                    }
                    u8g2.clearBuffer();
                    u8g2.setCursor(1, 20);
                    u8g2.print("LOAD SETTINGS Error");
                    u8g2.sendBuffer();
                    delay(2000);
                    saveSettings();
                    break;
                }
                delay(100);
            }
        }

        SettingsType getSettings()
        {
            return settings;
        }

        bool setBillType(char usingPulse)
        {
            settings.typeBill = usingPulse;
            return saveSettings();
        }

        char getBillType()
        {
            return settings.typeBill;
        }

        bool setPayoutValue(uint32_t billValue)
        {
            settings.extraConfig.payoutValue = billValue;
            return saveSettings();
        }

        bool toggleSingleVendConfig()
        {
            settings.singleVend = !settings.singleVend;
            return saveSettings();
        }

        bool toggleEnableEscrowConfig()
        {
            settings.extraConfig.enableEscrow = !settings.extraConfig.enableEscrow;
            return saveSettings();
        }

        bool SetTimerQR(uint8_t timeQR)
        {
            settings.extraConfig.timerQR = timeQR;
            return saveSettings();
        }

        bool isEscrowDisable()
        {
            return !settings.extraConfig.enableEscrow;
        }

        bool initPartId()
        {
            time_t now;
            struct tm timeinfo;
            if (!getLocalTime(&timeinfo))
            {
                now = 0;
            }
            time(&now);
            settings.vgasInfo.partId = now + 1;
            Serial.printf("initPartId %d \r\n", settings.vgasInfo.partId);
            return saveSettings();
        }

        uint32_t getPartId()
        {
            return settings.vgasInfo.partId;
        }

        bool setPartId(uint32_t partID)
        {
            settings.vgasInfo.partId = partID;
            return saveSettings;
        }

        bool setFuelType(uint8_t fuelType, char *name)
        {
            settings.vgasInfo.fuelType = fuelType;
            strcpy(settings.vgasInfo.fuelName, name);
            return saveSettings();
        }

        uint8_t getFuelType()
        {
            return settings.vgasInfo.fuelType;
        }

        String getFuelName()
        {
            return settings.vgasInfo.fuelName;
        }

        VgasInfoType getVgasInfo()
        {
            return settings.vgasInfo;
        }

        bool setVgasInfo(VgasInfoType vgasInfo)
        {
            settings.vgasInfo = vgasInfo;
            return saveSettings();
        }

        bool restoreDefault()
        {
            uint8_t *ptr = (uint8_t *)&settings;
            for (int i = 0; i < sizeof(settings); i++)
            {
                ptr[i] = 0;
            }
            saveSettings();
        }

        /*------------------------------ Settings ------------------------------*/

        String loadChipID()
        {
            uint32_t chipId = 0;
            for (int i = 0; i < 17; i = i + 8)
            {
                chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
            }
            sprintf(apSSID, "VGA%d", chipId);
            Serial.printf("\r\n\r\n %s\r\n\r\n", apSSID);
            return String(apSSID);
        }

        void begin()
        {
            Wire1.begin(16, 17, 400000);
            loadChipID();
            if (vmConfigEeprom.begin(0x53, Wire1) == false)
            {
                while (true)
                {
                    delay(10);
                }
            }
            loadSettings();
            loadRTC();
        }

        bool loadRTC()
        {
            const char *ntpServer = "pool.ntp.org";
            const long gmtOffset_sec = 25200;
            const int daylightOffset_sec = 0;
            configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
            if (rtc.begin())
            {
                DateTime now = rtc.now();
                struct tm time;
                time.tm_hour = now.hour();
                time.tm_min = now.minute();
                time.tm_sec = now.second();
                time.tm_mday = now.day();
                time.tm_mon = now.month() - 1;
                time.tm_year = now.year() - 1900;
                time_t t = mktime(&time);
                struct tm timeinfo;
                if (!getLocalTime(&timeinfo))
                {
                    struct timeval now_time = {.tv_sec = t};
                    settimeofday(&now_time, 0);
                    getLocalTime(&timeinfo);
                }
            }
        }

        void loop()
        {
            static uint32_t tick = 0;
            struct tm timeinfo;
            if ((unsigned long)(millis() - tick) >= 10000)
            {

                if (getLocalTime(&timeinfo, 500))
                {
                    struct tm timeinfo;
                    if (getLocalTime(&timeinfo, 200))
                    {
                        auto timeFunction = [&]
                        {
                            time_t now = time(nullptr);
                            struct tm *p_tm = localtime(&now);
                            rtc.adjust(DateTime(p_tm->tm_year + 1900, p_tm->tm_mon + 1, p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, 0));
                        };
                        accessI2CBus(timeFunction, 100);
                    }
                }
                tick = millis();
            }
        }
    }
}