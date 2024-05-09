#include "server_conn.h"
server_conn::server_conn(SIM800L *sim)
{
#if (!defined(USE_HMI)) && (!defined(USE_RFID)) && (!defined(MAY_DD)) && (!defined(KIOSK_ORDER))
    // delay(10000);
    // Serial.println(F("USE_HMI"));
#endif
    this->sim800l = sim;
}

int server_conn::getSimSignal()
{
    return simSignal;
}

bool server_conn::setupGPRS()
{
#if (!defined(USE_HMI)) && (!defined(USE_RFID)) && (!defined(MAY_DD)) && (!defined(KIOSK_ORDER))
    NetworkRegistration network = sim800l->getRegistrationStatus();
    uint32_t tick = millis();
    while (network != REGISTERED_HOME && network != REGISTERED_ROAMING)
    {
        delay(500);
        network = sim800l->getRegistrationStatus();
        if ((uint32_t)(millis() - tick) > 1500)
            return false;
    }
    // Serial.println(F("Network registration OK"));
    //  delay(1000);

    bool success = sim800l->setupGPRS("m-wap", "mms", "mms");
    tick = millis();
    while (!success)
    {
        success = sim800l->setupGPRS("m-wap", "mms", "mms");
        delay(500);
        if ((uint32_t)(millis() - tick) > 1500)
            return false;
    }
#endif
    return true;
}

int server_conn::postJsonData(char *url, char *data)
{
    int status = -1;
    static uint8_t errorCount = 0;
#ifdef USE_HMI
    Serial.println(F("USE_HMI"));
#endif
    if (WiFi.isConnected())
    {
        HTTPClient http;
        http.begin(url);
        http.setTimeout(500);
        http.addHeader("Content-Type", "application/json");
        int status = http.POST(data);
        // Serial.println(data);
        if (status > 0)
        {
            if (status != 200)
            {
                Serial.print("HTTP Response code: ");
                Serial.println(status);
                String payload = http.getString();
                Serial.println(payload);
            }
            else
            {
                // Serial.println("update status success");
            }
        }
        http.end();
        return status;
    }
    else
    {
#if (!defined(USE_HMI)) && (!defined(USE_RFID)) && (!defined(MAY_DD)) && (!defined(KIOSK_ORDER))
        if (sim800l->isReady())
        {
            if (setupGPRS() == false)
            {
                sim800l->disconnectGPRS();
                return -1;
            }
            bool connected = false;
            for (uint8_t i = 0; i < 5 && !connected; i++)
            {
                delay(200);
                connected = sim800l->connectGPRS();
            }
            status = sim800l->doPost(url, "application/json", data, 10000, 10000);
            if (status == 200)
            {
                // Success, output the data received on the serial
                // Serial.print(F("HTTP POST successful ("));
                // //Serial.print(sim800l->getDataSizeReceived());
                // //Serial.println(F(" bytes)"));
                // //Serial.print(F("Received : "));
                // //Serial.println(sim800l->getDataReceived());
            }
            else
            {
                sim800l->sendCommand_P("AT+HTTPTERM");
                sim800l->readResponseCheckAnswer_P(DEFAULT_TIMEOUT, "OK");
                // Failed...
                Serial.print(F("HTTP POST error "));
                Serial.println(status);
            }
            bool disconnected = sim800l->disconnectGPRS();
            for (uint8_t i = 0; i < 5; i++)
            {
                delay(200);
                disconnected = sim800l->disconnectGPRS();
                if (disconnected)
                    break;
            }
            return status;
        }
#endif
    }
    return -1;
}
