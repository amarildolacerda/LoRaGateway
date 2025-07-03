

#include "LoRaCom.h"

// LoRaCom loraCom;

#ifdef BROADCAST
void broadcastCallbackFn(const NetworkInfo *info)
{

#ifdef RADIO_WIFI
    RadioInterface *rd = loraCom.radio;
    if (rd)
    {
        RadioWiFi *wifiRadio = static_cast<RadioWiFi *>(rd);
        if (wifiRadio)
        {
            wifiRadio->setClientToGateway(info->ip, info->port);
        }
    }
#endif
}
#endif