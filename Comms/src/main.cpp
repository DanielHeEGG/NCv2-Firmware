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

int COUNTER = 0;

void setup()
{
    // Initialize UART
    Logger.begin(115200, SERIAL_8N1, 3, 1);
    Comms.begin(115200, SERIAL_8N1, 16, 17);

    // Initialize flash memory
    EEPROM.begin(512);

    // Set WiFi mode to SoftAP + Station Mode
    WiFi.mode(WIFI_MODE_APSTA);

#ifdef LOGGING
    Logger.println("[BOOT] Reading flash memory");
#endif
    readMemory(&EEPROM, &memoryData);

    if (memoryData.enableWifi)
    {
#ifdef LOGGING
        Logger.println("[BOOT] Connecting to WiFi");
#endif
        // Attempt to connect
        WiFi.begin(memoryData.ssid.c_str(), memoryData.password.c_str());
    }

#ifdef LOGGING
    Logger.println("[BOOT] Configuring SoftAP");
#endif
    // Configure SoftAP
    String ssid = String(ESP.getEfuseMac(), HEX);
    ssid.toUpperCase();
    ssid = "NixieController-" + ssid;
    WiFi.softAP(ssid.c_str(), NULL);
    WiFi.softAPConfig(LOCAL_IP, GATEWAY_IP, SUBNET_MASK);
    WiFi.setHostname(ssid.c_str());

    // Configure web server
    server.on("/", serverRootHandler);
    server.on("/wifi", serverWifiHandler);
    server.on("/time", serverTimeHandler);
    server.on("/tube", serverTubeHandler);

#ifdef LOGGING
    Logger.println("[BOOT] Starting server");
#endif
    server.begin();
}

void loop()
{
    server.handleClient();

    if (COUNTER == 0)
    {
        COUNTER = 6000;

        // Buffer Format:
        // 0[0b00:Empty 0b01:Time 0b10:TubeCurrent 0b11:Both] 1-2[Hour] 3-4[Minute] 5-6[Second] 7-9[TubeCurrent] 10['\n']
        String buffer;

        if (memoryData.enableNetTime && memoryData.enableWifi && WiFi.status() == WL_CONNECTED)
        {
#ifdef LOGGING
            Logger.println("[MAIN] Requesting internet time");
#endif
            HTTPClient client;
            client.begin(("https://www.timeapi.io/api/Time/current/zone?timeZone=" + memoryData.timezone).c_str());

            if (client.GET() == 200)
            {
                buffer = parsePacket(client.getString(), memoryData.tubeCurrent);
#ifdef LOGGING
                Logger.println("[MAIN] Time updated");
#endif
            }
            else
            {
                buffer = parsePacket(memoryData.tubeCurrent);
#ifdef LOGGING
                Logger.println("[MAIN] Internet time request failed");
#endif
            }
        }
        else
        {
            buffer = parsePacket(memoryData.tubeCurrent);
        }

        Comms.print(buffer + "\n");
    }
    COUNTER--;

    delay(100);
}

void serverRootHandler()
{
    // Compile webpage
    String webpage = String(INDEX_HTML);

    if (memoryData.enableWifi)
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            webpage.replace("{{WIFI_STATUS}}", "<div style=\"color:green\">WiFi Connected (SSID: " + memoryData.ssid + ")</div>");
        }
        else
        {
            webpage.replace("{{WIFI_STATUS}}", "<div style=\"color:red\">WiFi Disconnected</div>");
        }
    }
    else
    {
        webpage.replace("{{WIFI_STATUS}}", "<div style=\"color:red\">WiFi Disabled</div>");
    }

    if (memoryData.enableNetTime)
    {
        webpage.replace("{{TIME_STATUS}}", "<div style=\"color:green\"> Active Time Zone: " + memoryData.timezone + "</div>");
    }
    else
    {
        webpage.replace("{{TIME_STATUS}}", "<div style=\"color:red\">Internet Time Disabled</div>");
    }

    webpage.replace("{{TUBE_STATUS}}", "<div style=\"color:green\">Active Tube Current: " + String((float)memoryData.tubeCurrent / 255.0f * 10.6f, 1) + " / 10.6mA</div>");

    server.send(200, "text/html", webpage);
}

void serverWifiHandler()
{
    if (server.hasArg("enableWifi") && server.arg("enableWifi") == "true")
    {
        memoryData.enableWifi = true;

        if (server.hasArg("ssid") && server.arg("ssid").length() != 0 && server.arg("ssid").length() <= 32)
        {
            memoryData.ssid = server.arg("ssid");
#ifdef LOGGING
            Logger.println("[SERVER] New WiFi SSID settings accepted");
#endif
        }
        if (server.hasArg("password") && server.arg("password").length() != 0 && server.arg("password").length() <= 64)
        {
            memoryData.password = server.arg("password");
#ifdef LOGGING
            Logger.println("[SERVER] New WiFi password settings accepted");
#endif
        }
        writeMemory(&EEPROM, &memoryData);

#ifdef LOGGING
        Logger.println("[SERVER] Reconnecting WiFi");
#endif
        WiFi.begin(memoryData.ssid.c_str(), memoryData.password.c_str());
    }

    if (server.hasArg("disableWifi") && server.arg("disableWifi") == "true")
    {
        memoryData.enableWifi = false;
    }

    serverRootHandler();
}

void serverTimeHandler()
{
    if (server.hasArg("enableNetTime") && server.arg("enableNetTime") == "true")
    {
        memoryData.enableNetTime = true;

        if (server.hasArg("timezone") && server.arg("timezone").length() <= 64)
        {
            memoryData.timezone = server.arg("timezone");
#ifdef LOGGING
            Logger.println("[SERVER] New timezone settings accepted");
#endif
        }
        writeMemory(&EEPROM, &memoryData);

        COUNTER = 0;
    }

    if (server.hasArg("enableManualTime") && server.arg("enableManualTime") == "true")
    {
        memoryData.enableNetTime = false;

        if (server.hasArg("manualTimeHour") && server.arg("manualTimeHour").length() <= 2 && server.hasArg("manualTimeMinute") && server.arg("manualTimeMinute").length() <= 2 && server.hasArg("manualTimeSecond") && server.arg("manualTimeSecond").length() <= 2)
        {
            String buffer = parsePacket(server.arg("manualTimeHour"), server.arg("manualTimeMinute"), server.arg("manualTimeSecond"), memoryData.tubeCurrent);
            Comms.print(buffer + "\n");
#ifdef LOGGING
            Logger.println("[SERVER] New manual time settings accepted");
#endif
        }
        writeMemory(&EEPROM, &memoryData);
    }

    serverRootHandler();
}

void serverTubeHandler()
{
    if (server.hasArg("tubeCurrent") && server.arg("tubeCurrent").toFloat() != 0.0f && server.arg("tubeCurrent").toFloat() <= 10.6f)
    {
        memoryData.tubeCurrent = (uint8_t)(server.arg("tubeCurrent").toFloat() / 10.6f * 255);
#ifdef LOGGING
        Logger.println("[SERVER] New tube current settings accepted");
#endif
    }
    writeMemory(&EEPROM, &memoryData);

    COUNTER = 0;

    serverRootHandler();
}
