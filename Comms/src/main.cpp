/**
 ******************************************************************************
 * @file           : main.cpp
 * @brief          : Main program body
 ******************************************************************************
 */

#include "main.hpp"

#include "helper.hpp"

// #define LOGGING

// Embedded HTML in memory, null terminated
extern const char INDEX_HTML[] asm("_binary_src_web_index_html_start");
extern const char INDEX_HTML_END[] asm("_binary_src_web_index_html_end");

IPAddress LOCAL_IP(192, 168, 0, 2);
IPAddress GATEWAY_IP(192, 168, 0, 1);
IPAddress SUBNET_MASK(255, 255, 255, 0);

WebServer server(LOCAL_IP, 80);

HardwareSerial Logger(1);
HardwareSerial Comms(2);

MemoryData memoryData;

int SYSMODE = 0;

void setup()
{
    Logger.begin(115200, SERIAL_8N1, 3, 1);
    Comms.begin(115200, SERIAL_8N1, 16, 17);

    EEPROM.begin(512);

    WiFi.mode(WIFI_MODE_APSTA);

#ifdef LOGGING
    Logger.println("[BOOT] Reading flash memory");
#endif
    readMemory(&EEPROM, &memoryData);

#ifdef LOGGING
    Logger.println("[BOOT] Connecting to WiFi");
#endif
    WiFi.begin(memoryData.ssid.c_str(), memoryData.password.c_str());

    runWebserver();
}

void loop()
{
    server.handleClient();

    if (SYSMODE == 0)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
#ifdef LOGGING
            Logger.println("[MAIN] WiFi connected");
#endif
            Comms.println("OK");

            SYSMODE = 1;
        }
    }
    if (SYSMODE == 1)
    {
    }

    delay(100);
}

void runWebserver()
{
#ifdef LOGGING
    Logger.println("[SERVER] Configuring SoftAP");
#endif
    String ssid = String(ESP.getEfuseMac(), HEX);
    ssid.toUpperCase();
    ssid = "NixieController-" + ssid;
    WiFi.softAP(ssid.c_str(), NULL);
    WiFi.softAPConfig(LOCAL_IP, GATEWAY_IP, SUBNET_MASK);

    server.on("/", serverHandler);

#ifdef LOGGING
    Logger.println("[SERVER] Starting server");
#endif
    server.begin();
}

void serverHandler()
{
    bool FLAG0 = false;
    bool FLAG1 = false;

    if (server.hasArg("ssid") && server.arg("ssid").length() != 0 && server.arg("ssid").length() <= 32)
    {
        memoryData.ssid = server.arg("ssid");
        FLAG0 = true;
    }
    if (server.hasArg("password") && server.arg("password").length() != 0 && server.arg("password").length() <= 64)
    {
        memoryData.password = server.arg("password");
        FLAG0 = true;
    }
    if (server.hasArg("timezone") && server.arg("timezone").length() <= 64)
    {
        memoryData.timezone = server.arg("timezone");
        FLAG1 = true;
    }
    if (server.hasArg("tubeCurrent") && server.arg("tubeCurrent").toFloat() != 0.0f && server.arg("tubeCurrent").toFloat() <= 10.6f)
    {
        memoryData.tubeCurrent = (uint8_t)(server.arg("tubeCurrent").toFloat() / 10.6f * 255);
        FLAG1 = true;
    }

    if (FLAG0 || FLAG1)
    {
#ifdef LOGGING
        Logger.println("[SERVER] New settings accepted");
#endif
        writeMemory(&EEPROM, &memoryData);
    }

    if (FLAG0)
    {
#ifdef LOGGING
        Logger.println("[SERVER] Reconnecting WiFi");
#endif
        WiFi.begin(memoryData.ssid.c_str(), memoryData.password.c_str());
    }

    server.send(200, "text/html", INDEX_HTML);
}
