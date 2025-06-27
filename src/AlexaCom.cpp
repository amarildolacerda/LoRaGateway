#if defined(ALEXA)
#include "AlexaCom.h"
#include "deviceinfo.h"
#include "systemstate.h"

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
            auto device = alexa.getDevice(dev.alexaIdx);

#ifdef ALEXA
            if (secs >= 60 * 5)
            {
#ifdef DEBUG_ON
                Logger::debug("Device %s is offline for: %d seconds", dev.name, secs);
#endif
                // alexa.setState(dev.uniqueName().c_str(), false, 0);
                device->setState(false);
                device->setValue(0);
            }
            else
            {
                //                alexa.setState(dev.uniqueName().c_str(), data.state, 100);
                device->setState(data.state);
                device->setValue(255);
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

void AlexaCom::setup(WEBSERVER *server, AlexaCallbackType callback)
{
    alexaDeviceCallback = callback;
#ifdef ALEXA
    Logger::info("Alexa Init");

#ifdef DEBUG_ON
    Logger::debug("Creating Alexa Server");
#endif
    alexa.begin(server);
    // alexa.createServer((server == NULL) ? true : false);
    // alexa.setPort(80);

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

        addDevice(reg.tid, map.name);
    }

    // alexa.onSetState([](unsigned char device_id, const char *device_name, bool state, unsigned char value)
    //                  { alexaCom.DoCallback(device_id, device_name, state, value); });
    // alexa.onGetState([](unsigned char device_id, const char *device_name)
    //                  { alexaCom.DoGetCallback(device_id, device_name); });

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
    // alexa.enable(true);
    // alexa.triggerDiscovery();
    Logger::log(LogLevel::INFO, "Alexa Enable(true)");
#endif
}

void AlexaCom::renameDevice(const uint8_t tid, String name)
{
    size_t i = indexOf(tid);
    if (i >= 0)
    {
        AlexaDeviceMap map = alexaDevices[i];
        String oldname = map.name;
        map.name = name;
        char aname[16];
        map.uniqueName(aname, 16);
        // alexa.renameDevice(alexaDevices[i].alexaIdx, aname);
    }
}

void AlexaCom::loop()
{
#ifdef ALEXA
    alexa.loop();
    static long updateDiscovery = 0;
    if (millis() - updateDiscovery > 60000)
    {
        // alexa.triggerDiscovery();
        updateDiscovery = millis();
        // alexa.setThermostatStateByName("termostato", true, temperatureRead(), "heat");
    }

    static long alexaShow = 0;
    if (millis() - alexaShow > 30000)
    {
        alexaShow = millis();
        Serial.println("Alexa devices: ");
        for (auto &dev : alexaDevices)
        {
            Serial.printf("tId: %d, id: %d, ", dev.tid, dev.alexaIdx);
            EspalexaDevice *p = alexa.getDevice(dev.alexaIdx);
            if (p)
            {
                Serial.printf("Nome: %s, State: %s, Value: %d \n", p->getName(), p->getState() ? "on" : "off", p->getValue());
            }
            else
                (Serial.println(" nope "));
        }
        Serial.println();
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
    uint8_t alexaId = alexaDevices[id].alexaIdx;

#ifdef ALEXA
    String rsp = value ? "255" : "0";
    Logger::warn("Alexa::SetState(%d,%s,%s)", alexaId, String(value), rsp);
    // alexa.setState(alexaId, value, value ? 100 : 0);
    auto device = alexa.getDevice(alexaId);
    if (device)
    {
        device->setState(value);
        device->setValue(value ? 255 : 0);
        Logger::info("Send to Alexa %d %s", alexaId, device->getState() ? "on" : "off");
    }
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

void AlexaCom::addDevice(uint8_t tid, const String name)
{
    int pos = indexOf(tid);
    if (pos >= 0)
        return;
#ifdef DEBUG_ON
    Logger::debug("Adding device with tid: %d and name: %s", tid, name);
#endif
    AlexaDeviceMap map;
    map.tid = tid;
    map.name = name;
    char aname[16];
    map.uniqueName(aname, 16);
#ifdef ALEXA
    EspalexaDevice *d = new EspalexaDevice(String(aname), [this](EspalexaDevice *d)
                                           { DoCallback(d->getId(), d->getName().c_str(), d->getState(), d->getPercent()); }, EspalexaDeviceType::onoff, 0);

    int idx = alexa.addDevice(d);
    if (idx > 0)
    {
        map.alexaIdx = idx - 1;

        alexaDevices.push_back(map);

        Logger::info("Adicionou Alexa: %s", aname);
    }
#endif
}
#endif
