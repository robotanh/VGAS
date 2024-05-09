#include "VHITEK_Var.h"
#include "validatorHelper.h"
#include <ESPConnect.h>
#include <AsyncElegantOTA.h>
namespace VhitekVending
{
    Adafruit_Thermal printer(&Serial1, 255);
    TaskHandle_t TaskKeypadHandle;
    TaskHandle_t TaskMDBHandle;
    TaskHandle_t TaskSyncHandle;
    TaskHandle_t TaskTCPHandle;
    TaskHandle_t taskVgasUpdateRealTime;
    TaskHandle_t taskLEDHandle;
    TaskHandle_t taskVgasHandle;
    uint32_t currentSyncIDX = 0;
    uint32_t userIdeTick = 0;
    esp32FOTA fota(BOARD_TYPE, TOTAL_CHANGE, false);
    MatrixKeyboardManager keyboard;
    MultiIoAbstractionRef ExternalIO = multiIoExpander(0);
    BillValidator Validator;
    ICTBC_BUS dataICTBus(&Serial2, 1);
    essp dataESSPBus(&Serial2, Master);
    ICTPULSE_BUS dataPulseBus(5, 18, &Serial2);
    ICT104_BUS dataICT104Bus(&Serial2);

    SemaphoreHandle_t i2cSemaphoreHandle = NULL;
    uint32_t i2cSemaphoreHandleTakenTick = 0;

    SemaphoreHandle_t i2c1SemaphoreHandle = NULL;
    uint32_t i2c1SemaphoreHandleTakenTick = 0;

    SemaphoreHandle_t SPISemaphoreHandle = NULL;
    uint32_t SPISemaphoreHandleTakenTick = 0;

    SemaphoreHandle_t MDBSemaphoreHandle = NULL;
    uint32_t MDBSemaphoreHandleTakenTick = 0;

    SemaphoreHandle_t Serial1SemaphoreHandle = NULL;
    uint32_t Serial1SemaphoreHandleTakenTick = 0;

    int32_t soTienDaNhan = 0, soTienChuaNhan = 0, selectedColPrice = 0;
    char wifiSignal = -1, nhietDoKhoangLanh = -100, gsmSignal = 0, serverConnected = 0;
    char apSSID[50];
    QueueHandle_t KeypadQueue;
    QueueHandle_t TransactionDataQueue;
    bool startBeep = false;
    uint32_t beepTick = 0;
    WiFiClient mqtt;
    PubSubClient mqttClient(mqtt);

    void beep_buzz(int t)
    {
        ExternalIO->writeValue(13, HIGH);
        beepTick = millis() + t;
        startBeep = true;
    }

    class MyKeyboardListener : public KeyboardListener
    {
    public:
        void keyPressed(char key, bool held) override
        {
            if ((key != 'A') && (key != 'B') && (key != 'C') && (key != 'D'))
            {
                beep_buzz(200);
                xQueueSend(KeypadQueue, &key, 10);
            }
        }

        void keyReleased(char key) override
        {
        }
    } myListener;

    bool accessI2CBus(std::function<void()> &&Callback, uint32_t timeout)
    {
        if (xSemaphoreTake(i2cSemaphoreHandle, timeout) == pdTRUE)
        {
            i2cSemaphoreHandleTakenTick = millis();
            if (Wire.begin(25, 26, 100000))
                Callback();
            Wire.endTransmission(true);
            i2cSemaphoreHandleTakenTick = 0;
            xSemaphoreGive(i2cSemaphoreHandle);
            return true;
        }
        i2cSemaphoreHandleTakenTick = 0;
        return false;
    }

    bool accessI2C1Bus(std::function<void()> &&Callback, uint32_t timeout)
    {
        if (xSemaphoreTake(i2c1SemaphoreHandle, timeout) == pdTRUE)
        {
            i2c1SemaphoreHandleTakenTick = millis();
            if (Wire1.begin(16, 17, 400000))
                Callback();
            Wire1.endTransmission(true);
            i2c1SemaphoreHandleTakenTick = 0;
            xSemaphoreGive(i2c1SemaphoreHandle);
            return true;
        }
        i2cSemaphoreHandleTakenTick = 0;
        return false;
    }

    bool accessSPIBus(std::function<void()> &&Callback, uint32_t timeout)
    {
        if (xSemaphoreTake(SPISemaphoreHandle, timeout) == pdTRUE)
        {
            SPISemaphoreHandleTakenTick = millis();
            Callback();
            SPISemaphoreHandleTakenTick = 0;
            xSemaphoreGive(SPISemaphoreHandle);
            return true;
        }
        SPISemaphoreHandleTakenTick = 0;
        return false;
    }

    unsigned short cal_crc_loop_CCITT_A(short l, unsigned char *p)
    {
        int i, j;
        unsigned short seed = 0xFFFF;
        unsigned short cd = 0x8005;
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

    void begin()
    {
        Serial.begin(115200);
        Serial2.begin(9600, SERIAL_8N1, 18, 5, false);
        pinMode(33, INPUT);
        pinMode(15, INPUT);
        pinMode(PIN_PUMP, OUTPUT);
        pinMode(PIN_VAL1, OUTPUT);
        pinMode(PIN_VAL2, OUTPUT);
        pinMode(PIN_TOTAL, OUTPUT);
        Wire.begin(25, 26, 100000);
        Wire1.begin(16, 17, 400000);
        pinMode(13, OUTPUT);
        digitalWrite(13, HIGH);
        vSemaphoreCreateBinary(i2cSemaphoreHandle);
        vSemaphoreCreateBinary(i2c1SemaphoreHandle);
        vSemaphoreCreateBinary(SPISemaphoreHandle);
        vSemaphoreCreateBinary(MDBSemaphoreHandle);
        vSemaphoreCreateBinary(Serial1SemaphoreHandle);
        SPI.begin(LCD_CLOCK, -1, LCD_DATA, LCD_CS);
        Display::begin();
        Config::begin();
        Transactions::begin();
        mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
        mqttClient.setCallback(Process::callback);
        Serial.println();
        Serial.print("Connecting to ");
        Serial.println("VHITEK_4G");

        WiFi.begin("VHITEK_4G", "23122312");

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }

        xTaskCreateUniversal(Process::taskVgas, "taskVgas", 10000, NULL, 3, &taskVgasHandle, CONFIG_ARDUINO_RUNNING_CORE);
        xTaskCreateUniversal(Process::taskSyncData, "taskSyncData", 30000, NULL, 3, &TaskSyncHandle, CONFIG_ARDUINO_RUNNING_CORE);
        xTaskCreateUniversal(Process::VgasUpdateRealTime, "taskVgasUpdateRealTime", 30000, NULL, 3, &taskVgasUpdateRealTime, CONFIG_ARDUINO_RUNNING_CORE);
    }
}

namespace VhitekVending
{
    namespace Process
    {
        AsyncWebServer server(80);
        bool startSync = true;
        bool isSync = false;
        static int mode = 0;
        VgasTransactionType currentData;
        uint32_t lastReadTick = 0;
        uint32_t lastTag = 0;
        VgasTransactionType lastTranctionData;
        VgasSettingPriceType updatesPrice;

        /*------------------------------ MQTT PROCCESS ------------------------------*/
        void sendCmd(char *data)
        {

            for (int i = 0; i < 3; i++)
            {
                if (mqttClient.publish("/VGAS/BO", data))
                {
                    Serial.printf("Board send: %s\r\n", data);
                    break;
                }
            }
        }

        void resCmd(uint8_t ID, char *cmd, char *message)
        {
            DynamicJsonDocument doc(1000);
            char data[2048];
            doc["MID"] = apSSID;
            doc["ID"] = ID;
            doc["CMD"] = cmd;
            doc["MES"] = message;
            serializeJson(doc, data);
            sendCmd(data);
        }

        void updateStatus(VgasTransactionType data)
        {
            DynamicJsonDocument doc(10000);
            char rData[2048];
            char dateBuf[25];
            sprintf(dateBuf, "%04d-%02d-%02d %02d:%02d:%02d",
                    data.date.year + 1900, data.date.month, data.date.day,
                    data.date.hour, data.date.min, data.date.seconds);
            doc["CMD"] = "STATUS";
            doc["MID"] = apSSID;

            doc["INFO"]["fw"] = BOARD_TYPE;
            doc["INFO"]["time"] = dateBuf;
            if (WiFi.isConnected())
            {
                float RssI = WiFi.RSSI();
                RssI = isnan(RssI) ? -100.0 : RssI;
                RssI = min(max(2 * (RssI + 100.0), 0.0), 100.0);
                doc["INFO"]["wifi"] = RssI;
            }
            else
            {
                doc["INFO"]["wifi"] = 0;
            }
            doc["INFO"]["Total"] = data.total;
            doc["INFO"]["pType"] = data.paymentTerm;
            doc["INFO"]["pumpId"] = data.IdVoiBom;
            doc["INFO"]["IdLanBom"] = data.IdLanBom;
            doc["INFO"]["ThanhTien"] = data.ThanhTien;
            doc["INFO"]["sessionId"] = data.partId;
            doc["INFO"]["GiaXangDau"] = data.GiaXangDau;
            doc["INFO"]["SoLitBomDuoc"] = data.SoLitBomDuoc;
            doc["INFO"]["LoaiNhienLieu"] = data.LoaiXangDau;
            doc["INFO"]["TrangThaiCoBom"] = data.TrangThaiCoBom;
            doc["INFO"]["ID"] = data.ID;
            serializeJson(doc, rData);
            sendCmd(rData);
        }

        void reconnect()
        {
            if (!mqttClient.connected())
            {
                if (mqttClient.connect(apSSID))
                {
                    char topicSub[25];
                    sprintf(topicSub, "/%s/VGAS/SERVER", apSSID);
                    mqttClient.subscribe(topicSub);
                    Serial.printf("Subscribe %s\n", topicSub);
                    mqttClient.setBufferSize(2048);
                }
            }
        }

        void callback(char *topic, byte *payload, unsigned int length)
        {
            StaticJsonDocument<2000> MQTTDoc;
            char buffer[200];
            char saveBuffer[200];
            auto clear_buffer = [&]()
            {
                for (int i = 0; i < 200; i++)
                {
                    buffer[i] = 0;
                    saveBuffer[i] = 0;
                }
            };
            clear_buffer();
            for (int i = 0; i < length; i++)
            {
                int data = payload[i];
                buffer[i] = data;
                saveBuffer[i] = data;
            }

            MQTTDoc.clear();
            Serial.printf("MQTT: %s\r\n\n", buffer);
            DeserializationError error = deserializeJson(MQTTDoc, buffer);
            if (error == error.Ok)
            {
                // xử lý
            }
            else
            {
                Serial.printf("parser error %d\r\n", error.code());
            }
            clear_buffer();
            delay(10);
        }
        /*------------------------------ MQTT PROCCESS ------------------------------*/
        void WifiBegin()
        {
            ESPConnect.begin();
        }

        VgasTransactionType decodeKPL95Byte(char *rData)
        {
            VgasTransactionType decodedData;
            char dData[20];
            decodedData.LoaiXangDau = Config::getFuelType();
            decodedData.partId = Config::getPartId();
            decodedData.ID = Transactions::getCurrentTransID();
            decodedData.paymentTerm = 6;
            decodedData.TrangThaiCoBom = rData[2] - '0';
            Serial.printf("Trang thai co bom: %d\n", decodedData.TrangThaiCoBom);
            decodedData.IdVoiBom = rData[3];
            Serial.printf("ID voi bom: %d\n", decodedData.IdVoiBom);
            decodedData.trangThaiIn = (rData[4] == 'N') ? 0 : 1;
            Serial.printf("Trang thai in: %c\n", rData[4]);
            decodedData.trangThaiSS = (rData[4] == 'N') ? 0 : 1;
            Serial.printf("Trang thai SS: %c\n", rData[5]);
            decodedData.trangThaiXoaToTal = (rData[4] == 'N') ? 0 : 1;
            Serial.printf("Trang thai xoa total: %c\n", rData[6]);
            int offset = 7;
            for (int i = 0; i < 6; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[6] = 0;
            decodedData.IdLanBom = atol(dData);
            Serial.printf("ID lan bom: %d\n", decodedData.IdLanBom);
            offset = 13;
            for (int i = 0; i < 9; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[9] = 0;
            decodedData.SoLitBomDuoc = atol(dData);
            Serial.printf("So lit bom duoc: %d\n", decodedData.SoLitBomDuoc);
            offset = 22;
            for (int i = 0; i < 6; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[6] = 0;
            decodedData.GiaXangDau = atol(dData);
            Serial.printf("Gia xang dau: %d\n", decodedData.GiaXangDau);
            offset = 28;
            for (int i = 0; i < 9; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[9] = 0;
            decodedData.total = atol(dData);
            Serial.printf("Total: %d\n", decodedData.total);
            offset = 37;
            for (int i = 0; i < 9; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[9] = 0;
            decodedData.ThanhTien = atol(dData);
            Serial.printf("Thanh tien: %d\n", decodedData.ThanhTien);
            offset = 46;
            for (int i = 0; i < 6; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[6] = 0;
            Serial.printf("Ma lan bom cuoi: %d\n", atol(dData));
            offset = 52;
            for (int i = 0; i < 6; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[6] = 0;
            Serial.printf("Gia lan bom cuoi: %d\n", atol(dData));
            offset = 58;
            for (int i = 0; i < 9; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[9] = 0;
            Serial.printf("So lit ban lan bom cuoi: %d\n", atol(dData));
            offset = 67;
            for (int i = 0; i < 9; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[9] = 0;
            Serial.printf("So tien ban lan bom cuoi: %d\n", atol(dData));
            offset = 76;
            for (int i = 0; i < 8; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[8] = 0;
            Serial.printf("Han muc theo tien: %d\n", atol(dData));
            offset = 84;
            for (int i = 0; i < 8; i++)
            {
                dData[i] = rData[offset + i];
            }
            dData[8] = 0;
            Serial.printf("Han muc theo lit: %d\n", atol(dData));
            return decodedData;
        }

        VgasTransactionType KPL_ReadData95Bytes(int ID)
        {
            char rData[200];
            int rPtr = 0;
            delay(20);
            VgasTransactionType decodeData;
            decodeData.IdVoiBom = -1;

            while (Serial2.available())
            {
                Serial2.read();
                delay(5);
            }
            Serial2.write(8);
            Serial2.write(ID);
            Serial2.write(7);
            uint32_t startTime = millis();
            while (1)
            {
                if ((uint32_t)(millis() - startTime) > 500)
                    return decodeData;
                if (Serial2.available())
                {
                    char data = Serial2.read();
                    if (rPtr < 200)
                    {
                        rData[rPtr] = data;
                        rPtr++;
                    }
                    else
                    {
                        break;
                    }
                    if (rPtr >= 95)
                        break;
                }
                delay(3);
            }
            if ((rData[rPtr - 1] == 4) && (rData[rPtr - 3] == 3))
            {
                char checksum = 0x5A;
                for (int i = 2; i < rPtr - 3; i++)
                {
                    checksum = checksum ^ rData[i];
                }

                if (checksum != rData[rPtr - 2])
                {
                    Serial.printf("Checksum Error\r\n");
                    return decodeData;
                }

                if (rPtr == 95)
                {
                    decodeData = decodeKPL95Byte(rData);
                }

                if (Transactions::getTotalTransUnSync() < Transactions::getMaxTransactionIdx())
                {
                    if (decodeData.TrangThaiCoBom == 0)
                    {
                        // Serial2.write(7);
                        // Serial2.write(10 + ID);
                        // Serial2.write(8);
                    }
                }
            }
            return decodeData;
        }

        void updatePumpData(VgasTransactionType data)
        {
            currentData = data;
        }

        void loopVgas()
        {
            static uint8_t lastPumpStatus = 0;
            VgasTransactionType data = KPL_ReadData95Bytes(23);
            if (data.IdVoiBom == -1)
            {
                delay(500);
            }
            else
            {
                updatePumpData(data);
            }
        }

        void VgasUpdateRealTime(void *parameter)
        {
            VgasTransactionType lastData;
            uint32_t lastUpdateTick = 0;
            while (1)
            {
                if ((currentData.SoLitBomDuoc != lastData.SoLitBomDuoc) || (currentData.TrangThaiCoBom != lastData.TrangThaiCoBom) || ((uint32_t)millis() - lastUpdateTick > 10000))
                {
                    if (mqttClient.connected())
                    {
                        time_t now = time(nullptr);
                        struct tm *p_tm = localtime(&now);
                        currentData.date.day = p_tm->tm_mday;
                        currentData.date.month = p_tm->tm_mon + 1;
                        currentData.date.year = p_tm->tm_year;
                        currentData.date.hour = p_tm->tm_hour;
                        currentData.date.min = p_tm->tm_min;
                        currentData.date.seconds = p_tm->tm_sec;
                        updateStatus(currentData);
                        lastUpdateTick = millis();
                        lastData = currentData;
                    }
                }
                delay(100);
            }
        }

        void taskVgas(void *parameter)
        {
            while (1)
            {
                loopVgas();
                delay(50);
            }
        }

        void taskSyncData(void *parameter)
        {
            while (1)
            {
                static uint32_t tickReconnectWiFi = 0;
                static uint32_t tickReconnectMQTT = 0;
                delay(500);
                if (startSync)
                {
                    if (!WiFi.isConnected())
                    {
                        if ((uint32_t)(millis() - tickReconnectWiFi) > 10000)
                        {
                            WiFi.disconnect();
                            delay(2000);
                            WifiBegin();
                            tickReconnectWiFi = millis();
                        }
                    }

                    if ((!mqttClient.connected()) && (WiFi.isConnected()))
                    {
                        if ((uint32_t)(millis() - tickReconnectMQTT) > 10000)
                        {
                            reconnect();
                            tickReconnectMQTT = millis();
                        }
                    }
                    else
                    {
                        mqttClient.loop();
                    }
                }
                delay(100);
            }
        }
    }
}
