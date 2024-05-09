#include <Arduino.h>
#include "VHITEK_Var.h"

namespace VhitekVending
{

    namespace TCP
    {
#if defined(TOUCH_SCREEN) || defined(ADVERTISEMENT)
        StaticJsonDocument<12000> TCPDoc;
        int32_t slot = 0;
        uint8_t count = 0;

        typedef std::function<void(String)> _TCPOnEventCallback_t;

        typedef struct
        {
            _TCPOnEventCallback_t fn;
            String event;
        } _TCP_POLL_HANDLER;

        std::vector<_TCP_POLL_HANDLER> _PollHandlers;

        void pollEventCallback(String event, String data)
        {
            for (_TCP_POLL_HANDLER n : _PollHandlers)
            {
                if (n.event == event)
                {
                    n.fn(data);
                }
            }
        }

        void on(String event, _TCPOnEventCallback_t fn)
        {
            _TCP_POLL_HANDLER pollHander;
            pollHander.event = event;
            pollHander.fn = fn;
            _PollHandlers.push_back(pollHander);
        }

        namespace TouchScreen
        {
            WiFiClient touchClient;

            void sendCmd(char *cmd)
            {
                if (touchClient.connected())
                {
                    touchClient.write(0x7F);
                    touchClient.print(cmd);
                    touchClient.write(0x7E);
                    // Serial.printf("Board send: %s\n\n", cmd);
                }
                else
                {
                    // Serial.printf("Send failed TCP disconnected\n");
                }
            }

            void sendAdminMode()
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "ADMIN_MODE";
                TCPDoc["STATUS"] = digitalRead(15);
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void sendPI()
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "PI";
                serializeJson(TCPDoc, data);
                sendCmd(data);
                count++;
            }

            void setCount(String buffer)
            {
                count = 0;
            }

            void startPayment(String buffer)
            {
                TCPDoc.clear();
                DeserializationError error = deserializeJson(TCPDoc, buffer);
                if (error == error.Ok)
                {
                    int32_t tmpSlot = TCPDoc["COL"].as<int32_t>();
                    if (Process::Working::selectedCol == tmpSlot)
                    {
                        return;
                    }
                    if (Process::Working::keyPadIdx > 0)
                    {
                        Process::Working::clearKeypadData();
                    }
                    slot = tmpSlot;
                    Process::Working::processPayments();
                }
            }

            void endPayment(String buffer)
            {
                Process::Working::clearKeypadData();
            }

            void ping(String buffer)
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "PING";
                TCPDoc["STATUS"] = "PONG";
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void getSales(String buffer)
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "SALES";
                TCPDoc["MID"] = VhitekVending::apSSID;
                for (int i = 0; i < TOTAL_SLOTS; i++)
                {
                    TCPDoc["m"][i][0] = i + 1;
                    TCPDoc["m"][i][1] = Config::getSettings().colInfo[i].colPrice / 1000;
                    if ((Config::getSlotError().slot[i] != 0) && (Config::getSlotError().slot[i] != 1))
                        TCPDoc["m"][i][2] = 2;
                    else
                        TCPDoc["m"][i][2] = 1;
                    TCPDoc["m"][i][3] = Config::getProductIdConfig(i + 1);
                }
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void getQR(String buffer)
            {
                TCPDoc.clear();
                DeserializationError error = deserializeJson(TCPDoc, buffer);
                if (error == error.Ok)
                {
                    if ((Process::Working::selectedCol) && (Process::Working::selectedColPrice))
                    {
                        Process::Working::currentColQR = Process::Working::selectedCol;
                        Process::Working::vnpTransactionID = millis();
                        if (TCPDoc["STYLE"].as<String>() == "EWALLET")
                            Process::Working::VNPAYQR = Process::Working::getVNPAYCODE(Process::Working::selectedCol, Process::Working::selectedColPrice, Process::Working::vnpTransactionID, Transactions::TransactionID);
                        else if ((TCPDoc["STYLE"].as<String>() == "IWALLET"))
                            Process::Working::VNPAYQR = Process::Working::getQRUniApp();
                    }
                }
            }

            void sendIndentification(String buffer)
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "INDENTIFICATION";
                TCPDoc["MID"] = VhitekVending::apSSID;
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void sendQRVNPAY(String qr)
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "QR";
                TCPDoc["QR"] = qr;
                TCPDoc["STYLE"] = Process::Working::payCode;
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void sendStartPayment(int sl)
            {
                char data[2048];
                TCPDoc.clear();
                if (sl == 0)
                {
                    return;
                }
                TCPDoc["CMD"] = "START_PAYMENT";
                TCPDoc["COL"] = sl;
                TCPDoc["BALANCE"] = Process::Working::soTienDaNhan + Process::Working::soTienChuaNhan;
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void sendEndPayment()
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "END_PAYMENT";
                TCPDoc["BALANCE"] = Process::Working::soTienDaNhan + Process::Working::soTienChuaNhan;
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void sendChangeScreen(String name)
            {
                char data[2048];
                TCPDoc.clear();
                TCPDoc["CMD"] = "CHANGE_SCREEN";
                TCPDoc["MID"] = VhitekVending::apSSID;
                TCPDoc["SCREEN"] = name;
                TCPDoc["BALANCE"] = Process::Working::soTienDaNhan + Process::Working::soTienChuaNhan;
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }

            void sendErrorMotor()
            {
                char data[2048];
                TCPDoc.clear();
                int count = 0;
                TCPDoc["CMD"] = "LOCK";
                TCPDoc["MID"] = VhitekVending::apSSID;
                for (int i = 1; i <= TOTAL_SLOTS; i++)
                {
                    if (Config::slotError.slot[i - 1] != 0)
                    {
                        TCPDoc["ERR"][count] = i;
                        count++;
                    }
                }
                serializeJson(TCPDoc, data);
                sendCmd(data);
            }
        }

        void beginCallbackFn()
        {
#if defined(TOUCH_SCREEN)
            on("START_PAYMENT", TouchScreen::startPayment);
            on("END_PAYMENT", TouchScreen::endPayment);
            on("QR", TouchScreen::getQR);
            on("SALES", TouchScreen::getSales);
            on("INDENTIFICATION", TouchScreen::sendIndentification);
            on("PING", TouchScreen::ping);
            on("PO", TouchScreen::setCount);
#endif
        }
#endif
    }
}