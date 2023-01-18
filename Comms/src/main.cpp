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
    if (server.hasArg("ssid") && server.hasArg("password") && server.hasArg("tubeCurrent") && server.hasArg("timezone"))
    {
        if (server.arg("ssid").length() <= 32 && server.arg("password").length() <= 64 && server.arg("tubeCurrent").toFloat() != 0.0f && server.arg("tubeCurrent").toFloat() <= 10.6f)
        {
            memoryData.ssid = server.arg("ssid");
            memoryData.password = server.arg("password");
            memoryData.timezone = server.arg("timezone");
            memoryData.tubeCurrent = (uint8_t)(server.arg("tubeCurrent").toFloat() / 10.6f * 255);
            writeMemory(&EEPROM, &memoryData);

#ifdef LOGGING
            Logger.println("[SERVER] New settings accepted");
#endif

            WiFi.begin(memoryData.ssid.c_str(), memoryData.password.c_str());
        }
    }
    server.send(200, "text/html", INDEX_HTML);
}
