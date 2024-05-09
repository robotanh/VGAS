#include "VHITEK_Var.h"
namespace VhitekVending
{
    namespace Transactions
    {
        ExternalEEPROM vmTransactionEeprom;
        uint32_t TransactionID = 0;
        uint32_t lastTransactionEEPromIdx = 0;
        uint32_t maxTransactionEEPromIdx = 64000 / sizeof(VgasTransactionType) - sizeof(VgasTransactionType) * 2;
        uint32_t totalTransUnsync = 0;

        uint16_t getMaxTransactionIdx()
        {
            return maxTransactionEEPromIdx;
        }

        uint32_t getCurrentTransID()
        {
            return TransactionID;
        }

        uint16_t getCurrentTransIdx()
        {
            return lastTransactionEEPromIdx;
        }

        uint32_t getTotalTransUnSync()
        {
            return totalTransUnsync;
        }

        void setTotalTransUnSync(uint32_t totalUnsync)
        {
            totalTransUnsync = totalUnsync;
        }

        void minusTotalTransUnSync()
        {
            totalTransUnsync--;
        }

        String toTransJson(VgasTransactionType data)
        {
            DynamicJsonDocument doc(10000);
            char rData[2048];
            char mId[20];
            char dateBuf[25];
            sprintf(dateBuf, "%04d-%02d-%02d %02d:%02d:%02d", data.date.year + 1900, data.date.month, data.date.day, data.date.hour, data.date.min, data.date.seconds);
            sprintf(mId, "%s_%d", apSSID, data.IdVoiBom);
            doc["mid"] = mId;
            doc["sessionid"] = data.partId;
            doc["transactionid"] = data.ID;
            doc["typepayment"] = data.paymentTerm;
            doc["amount"] = data.SoLitBomDuoc;
            doc["fueltype"] = data.LoaiXangDau;
            doc["price"] = data.GiaXangDau;
            doc["totalmoney"] = data.ThanhTien;
            doc["total"] = data.total;
            doc["totalsale"] = data.IdLanBom;
            doc["date"] = dateBuf;
            doc["cardpayment"] = data.cardId;
            serializeJson(doc, rData);
            return String(rData);
        }

        int16_t sumCalc(VgasTransactionType data)
        {
            int len = sizeof(data);
            uint8_t buffer[100];
            uint8_t *ptr = (uint8_t *)&data;
            uint8_t dataSize = 0;
            for (int i = 0; i < len; i++)
            {
                if ((uint32_t)&data.checkSum <= (uint32_t)&ptr[i])
                {
                    break;
                }
                buffer[i] = ptr[i];
                dataSize++;
            }
            uint16_t sum = cal_crc_loop_CCITT_A(dataSize, buffer);
            return sum;
        }

        VgasTransactionType readEeprom(uint16_t idx, bool *dataVaid)
        {
            uint16_t eepromPtr = idx * sizeof(VgasTransactionType);
            VgasTransactionType tmpData;
            vmTransactionEeprom.get(eepromPtr, tmpData);
            uint16_t sum = sumCalc(tmpData);
            *dataVaid = false;
            if (sum == tmpData.checkSum)
                *dataVaid = true;
            return tmpData;
        }

        bool writeToEpprom(VgasTransactionType data, uint16_t idx)
        {
            uint16_t eepromPtr = idx * sizeof(VgasTransactionType);
            uint16_t sum = sumCalc(data);
            data.checkSum = sum;
            vmTransactionEeprom.put(eepromPtr, data);
            bool dataVaid;
            readEeprom(idx, &dataVaid);
            return dataVaid;
        }

        bool addTransaction(VgasTransactionType data)
        {
            uint32_t tick = millis();
            bool success = false;
            auto addTrans = [&]()
            {
                if (writeToEpprom(data, lastTransactionEEPromIdx + 1))
                {
                    lastTransactionEEPromIdx += 1;
                    if (lastTransactionEEPromIdx >= maxTransactionEEPromIdx)
                        lastTransactionEEPromIdx = 0;
                    success = true;
                    totalTransUnsync++;
                    TransactionID += 1;
                }
                else
                {
                    success = false;
                }
            };
            accessI2C1Bus(addTrans, 1000);
            return success;
        }

        void clearVmTransactionEeprom()
        {
            VgasTransactionType tmp;
            uint8_t *ptr = (uint8_t *)&tmp;
            for (int j = 0; j < sizeof(tmp); j++)
            {
                ptr[j] = 0;
            }

            for (int i = 0; i <= 64000 / sizeof(tmp); i++)
            {
                accessSPIBus([&]()
                             {     
                                        u8g2.clearBuffer(); // clear the internal memory
                                        u8g2.drawFrame(0, 0, 128, 64);
                                        u8g2.setFont(u8g2_font_profont10_mf); // Courier New Bold 10,12,14,18,24
                                        u8g2.setCursor(2, 10);
                                        u8g2.printf("RESTORY DEFAULT");
                                        u8g2.setCursor(2, 19);
                                        u8g2.printf(" RESTORY DEFAULT : YES");
                                        u8g2.setCursor(2, 30);
                                        u8g2.printf("CLEAR TRANS %.02f",(((float)i / 1454) * 100));
                                        u8g2.sendBuffer(); },
                             3000);
                vmTransactionEeprom.put(i * sizeof(tmp), ptr);
            }
        }

        void begin()
        {
            bool vaid = false;
            uint32_t lastID = 0;
            VgasTransactionType tmpData;
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_profont10_mf);
            u8g2.setCursor(1, 10);
            u8g2.printf("LOADING DATA ...");
            u8g2.sendBuffer();
            if (vmTransactionEeprom.begin(0x51, Wire1) == false)
            {

                u8g2.setCursor(1, 20);
                u8g2.printf("vmTransactionEeprom not found");
                while (1)
                {
                    delay(100);
                }
            }
            vmTransactionEeprom.setMemorySize(65536);
            vmTransactionEeprom.setPageSize(64);
            uint32_t startTick = millis();
            for (uint32_t i = 0; i < maxTransactionEEPromIdx; i++)
            {
                if ((uint32_t)(millis() - startTick) > 200)
                {
                    u8g2.setCursor(1, 30);
                    u8g2.printf("Loading %.2f ", (float)i / maxTransactionEEPromIdx * 100);
                    u8g2.sendBuffer();
                    startTick = millis();
                }
                tmpData = readEeprom(i, &vaid);
                if (vaid)
                {
                    if (lastID < tmpData.ID)
                    {
                        lastID = tmpData.ID;
                        TransactionID = tmpData.ID;
                        lastTransactionEEPromIdx = i;
                        Process::lastTranctionData = tmpData;
                    }
                    if (tmpData.syncByte == 0)
                    {
                        totalTransUnsync++;
                    }
                }
            }
            TransactionID = TransactionID + 1;
            Serial.printf("Last trans: %s\n", toTransJson(Process::lastTranctionData).c_str());
            Serial.printf("transactionID = %d, lastTransactionEEPromIdx = %d, totalTransUnsync = %d\n\n", TransactionID, lastTransactionEEPromIdx, totalTransUnsync);
        }
    }
}