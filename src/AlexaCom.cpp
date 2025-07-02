#ifdef ALEXA
#include "AlexaCom.h"
#include "deviceinfo.h"
#include "systemstate.h"
#include "ESPAsyncWebServer.h"

#ifdef ALEXA
#include "fauxmoESP.h"
fauxmoESP alexa;
#endif

AlexaCom alexaCom;

void AlexaCom::aliveOffLineAlexa()
{
    DeviceData data;
    for (auto &dev : alexaDevices)
    {
#ifdef DEBUG_ON
        Logger::debug("Checking device with tid: %d", dev.tid);
#endif
        int idx = deviceInfo.indexOf(dev.tid);
        int secs = 60;
        if (idx >= 0)
        {
            data = deviceInfo.getDevices()[idx];
            secs = deviceInfo.diffSeconds(data.lastSeen);
#ifdef ALEXA
            if (secs >= 60 * 5)
            {
#ifdef DEBUG_ON
                Logger::debug("Device %s is offline for: %d seconds", dev.uniqueName().c_str(), secs);
#endif
                alexa.setState(dev.uniqueName().c_str(), false, 0);
            }
            else
            {
                alexa.setState(dev.uniqueName().c_str(), data.state, 100);
            }
#endif
        }
    }
}

void AlexaCom::DoCallback(unsigned char device_id, const char *device_name, bool state, unsigned char value)
{
#ifdef DEBUG_ON
    Logger::debug("Executing callback for device_id: %d, device_name: %s", device_id, device_name);
#endif
    int idx = findByAlexaId(device_id);
    if (idx < 0)
        return;
    if (alexaDeviceCallback)
        alexaDeviceCallback(alexaDevices[idx].tid, device_name, state, value);
}

void AlexaCom::DoGetCallback(unsigned char device_id, const char *device_name)
{
    if (onGetCallbackFn)
    {
        onGetCallbackFn(device_name);
    }
}

void AlexaCom::setup(AsyncWebServer *server, AlexaCallbackType callback)
{
    alexaDeviceCallback = callback;
#ifdef ALEXA
    Logger::info("Alexa Init");

#ifdef DEBUG_ON
    Logger::debug("Creating Alexa Server");
#endif
    alexa.createServer((server == NULL) ? true : false);
    alexa.setPort(80);
    alexa.enable(true);

    for (size_t i = 0; i < deviceInfo.size(); i++)
    {
        DeviceData reg = deviceInfo.getDevices()[i];
        if (reg.tid == 0)
            continue;

#ifdef DEBUG_ON
        Logger::debug("Setting up device: %s with tid: %d", reg.name.c_str(), reg.tid);
#endif
        AlexaDeviceMap map;
        map.tid = reg.tid;
        map.name = reg.name;
        addDevice(reg.tid, map.uniqueName().c_str());
    }

    alexa.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value)
                     { alexaCom.DoCallback(device_id, device_name, state, value); });
    alexa.onGetState([](unsigned char device_id, const char *device_name)
                     { alexaCom.DoGetCallback(device_id, device_name); });

#ifdef WS
    server->onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                          {
                              if (alexa.process(request->client(), request->method() == HTTP_GET, request->url(), String((char *)data)))
                                  return; });
    server->onNotFound([](AsyncWebServerRequest *request)
                       {
                           String body = (request->hasParam("body", true)) ? request->getParam("body", true)->value() : String();
                           if (alexa.process(request->client(), request->method() == HTTP_GET, request->url(), body))
                               return; });
#endif
    // alexa.addThermostat("termostato");
    // alexa.setPort(80);
    // alexa.triggerDiscovery();
    Logger::log(LogLevel::INFO, "Alexa Enable(true)");
#endif
}

void AlexaCom::renameDevice(const uint8_t tid, String name)
{
    size_t i = indexOf(tid);
    if (i >= 0)
    {
        String oldname = alexaDevices[i].name;
        alexaDevices[i].name = name;
        alexa.renameDevice(alexaDevices[i].alexaId, name.c_str());
    }
}

void AlexaCom::loop()
{
#ifdef ALEXA
    alexa.handle();
    static long updateDiscovery = 0;
    if (millis() - updateDiscovery > 60000)
    {
        // alexa.triggerDiscovery();
        updateDiscovery = millis();
        // alexa.setThermostatStateByName("termostato", true, temperatureRead(), "heat");
    }
#endif
}

void AlexaCom::updateStateAlexa(const uint8_t tid, const bool value)
{
#ifdef DEBUG_ON
    Logger::debug("Updating state for device with tid %d to %s", tid, value ? "on" : "off");
#endif
    uint8_t id = indexOf(tid);
    if (id < 0)
        return;
    uint8_t alexaId = alexaDevices[id].alexaId;

#ifdef ALEXA
    String rsp = value ? "100" : "0";
    Logger::warn("Alexa::SetState(%d,%s,%s)", alexaId, String(value), rsp);
    alexa.setState(alexaId, value, value ? 100 : 0);
    Logger::info("Send to Alexa %d %s", alexaId, rsp);
#endif
}

void AlexaCom::addTermostat(String name)
{
    // alexa.addThermostat(name.c_str());
    // alexa.onSetThermostat([](unsigned char a, const char *b, bool c, unsigned char d, const char *e)
    //{
    //    Serial.printf("Alexa diz termostato: %s: %s,%s, %s \n", a, b, d, e);
    //});
    // isTermostat = true;
}

void AlexaCom::addTemperature(String name)
{
    // alexa.addTemperatureSensor(name.c_str());
    // alexa.onSetTemperature([](unsigned char id, const char *name, float temp)
    //                        { Serial.printf("Alexa diz temperatura %d, %s, %.1f", id, name, temp); });
}
void AlexaCom::setTemperature(String name, float temp)
{
    // alexa.setTemperature(name.c_str(), temp);
}

void AlexaCom::addDevice(uint8_t tid, const char *name)
{
#ifdef DEBUG_ON
    Logger::debug("Adding device with tid: %d and name: %s", tid, name);
#endif
    AlexaDeviceMap map;
    map.tid = tid;
    map.name = name;
    String aname = map.uniqueName();
#ifdef ALEXA
    if (alexa.getDeviceId(aname.c_str()) < 0)
    {
        alexa.addDevice(aname.c_str());
        map.alexaId = alexa.getDeviceId(aname.c_str());

        alexa.setDeviceUniqueId((char)map.alexaId, map.uniqueId().c_str());

        alexaDevices.push_back(map);
    }
    Logger::info("Adicionou Alexa: %s", aname);
#endif
}
#endif
