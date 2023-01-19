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

#ifdef LOGGING
    Logger.println("[BOOT] Connecting to WiFi");
#endif
    // Attempt to connect
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

            SYSMODE = 1;
        }
    }
    if (SYSMODE == 1)
    {
        if (WiFi.status() != WL_CONNECTED)
        {
#ifdef LOGGING
            Logger.println("[MAIN] WiFi disconnected");
#endif
            SYSMODE = 0;
        }
        else
        {
            // Get internet time every 600s
            if (COUNTER == 0)
            {
                COUNTER = 6000;

#ifdef LOGGING
                Logger.println("[MAIN] Requesting internet time");
#endif

                HTTPClient client;
                client.begin(("https://www.timeapi.io/api/Time/current/zone?timeZone=" + memoryData.timezone).c_str());

                // Buffer Format:
                // 0[0b00:Empty 0b01:Time 0b10:TubeCurrent 0b11:Both] 1-4[Year] 5-6[Month] 7-8[Day] 9-10[Hour] 11-12[Minute] 13-14[Second] 15[DayOfWeek] 16-18[TubeCurrent] 19['\n']
                String buffer;

                if (client.GET() == 200)
                {
                    buffer = parsePacket(client.getString(), memoryData.tubeCurrent);
#ifdef LOGGING
                    Logger.println("[MAIN] Time updated");
#endif
                }
                else
                {
                    buffer = "2000000000000000";
                    if (memoryData.tubeCurrent < 100) buffer += "0";
                    if (memoryData.tubeCurrent < 10) buffer += "0";
                    buffer += String((int)memoryData.tubeCurrent);
#ifdef LOGGING
                    Logger.println("[MAIN] Internet time request failed");
#endif
                }

                Comms.print(buffer + "\n");
            }
            COUNTER--;
        }
    }

    delay(100);
}

void runWebserver()
{
#ifdef LOGGING
    Logger.println("[SERVER] Configuring SoftAP");
#endif
    // Configure SoftAP
    String ssid = String(ESP.getEfuseMac(), HEX);
    ssid.toUpperCase();
    ssid = "NixieController-" + ssid;
    WiFi.softAP(ssid.c_str(), NULL);
    WiFi.softAPConfig(LOCAL_IP, GATEWAY_IP, SUBNET_MASK);
    WiFi.setHostname(ssid.c_str());

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

    if (FLAG1)
    {
        // Get new internet time immediately
        COUNTER = 0;
    }

    // Compile webpage
    String webpage = String(INDEX_HTML);

    if (SYSMODE == 0)
    {
        webpage.replace("{{STATUS}}", "<div style=\"color:red\">WiFi Disconnected</div>");
    }
    if (SYSMODE == 1)
    {
        webpage.replace("{{STATUS}}", "<div style=\"color:green\">WiFi Connected (SSID: " + memoryData.ssid + ")</div>");
    }

    webpage.replace("{{SETTINGS}}", "<div>Time Zone: " + memoryData.timezone + "</div><div>Tube Current: " + String((float)memoryData.tubeCurrent / 255.0f * 10.6f, 1) + " / 10.6mA</div>");

    server.send(200, "text/html", webpage);
}
