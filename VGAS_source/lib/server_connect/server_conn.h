#ifndef _SERVER_CONN_H
#define _SERVER_CONN_H
#include "Arduino.h"
#include "SIM800L.h"
#include <HTTPClient.h>
//#define KIOSK_ORDER
class server_conn
{
private:
    int simPostdata();
    int simGetData();
    SIM800L *sim800l;
    int simSignal;

public:
    server_conn(SIM800L *sim);
    int getSimSignal();
    bool setupGPRS();
    int postJsonData(char *url, char *data);
    String getData();
};
#endif