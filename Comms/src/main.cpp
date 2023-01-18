/**
 ******************************************************************************
 * @file           : main.cpp
 * @brief          : Main program body
 ******************************************************************************
 */

#include "main.hpp"

#include "helper.hpp"

#define LOGGING

// Embedded HTML in memory, null terminated
extern const char INDEX_HTML[] asm("_binary_src_web_index_html_start");
extern const char INDEX_HTML_END[] asm("_binary_src_web_index_html_end");

IPAddress LOCAL_IP(192, 168, 0, 2);
IPAddress GATEWAY_IP(192, 168, 0, 1);
IPAddress SUBNET_MASK(255, 255, 255, 0);

WebServer server(LOCAL_IP, 80);

HardwareSerial Logger(1);
HardwareSerial Comms(2);

MemoryData memoryData = {0};

int SYSMODE = 0;

void setup()
{
    Logger.begin(115200, SERIAL_8N1, 3, 1);
    Comms.begin(115200, SERIAL_8N1, 16, 17);

    EEPROM.begin(512);

    WiFi.mode(WIFI_MODE_APSTA);

#ifdef LOGGING
    Logger.println("Waiting for command");
#endif

    char buffer[100] = {0};
    UARTRecvPoll(&Comms, buffer);

    if (strcmp(buffer, "BOOT") == 0)
    {
        Comms.println("OK");

#ifdef LOGGING
        Logger.println("[BOOT] Reading flash memory");
#endif
        readMemory(&EEPROM, &memoryData);

#ifdef LOGGING
        Logger.println("[BOOT] Connecting to WiFi");
#endif
        WiFi.begin(memoryData.ssid, memoryData.password);
    }
    if (strcmp(buffer, "CONFIG") == 0)
    {
        Comms.println("OK");
    }

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
    char ssid[25] = {0};
    snprintf(ssid, 25, "NixieController-%llX", ESP.getEfuseMac());
    WiFi.softAP(ssid, NULL);
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
            strcpy(memoryData.ssid, server.arg("ssid").c_str());
            strcpy(memoryData.password, server.arg("password").c_str());
            strcpy(memoryData.timezone, server.arg("timezone").c_str());
            memoryData.tubeCurrent = (uint8_t)(server.arg("tubeCurrent").toFloat() / 10.6f * 255);
            writeMemory(&EEPROM, &memoryData);

#ifdef LOGGING
            Logger.println("[SERVER] New settings accepted");
#endif

            WiFi.begin(memoryData.ssid, memoryData.password);
        }
    }
    server.send(200, "text/html", INDEX_HTML);
}
