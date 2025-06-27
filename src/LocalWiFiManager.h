#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "WiFiManager.h"

class LocalWiFiManager
{
protected:
    WiFiManager wm;

public:
    LocalWiFiManager()
    {
    }

    void autoConnect(String ssid = "", String pass = "")
    {
    }

    void process()
    {
    }

private:
};

#endif // WIFI_MANAGER_H