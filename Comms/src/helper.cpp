/**
 ******************************************************************************
 * @file           : helper.cpp
 * @brief          : Helper functions
 ******************************************************************************
 */

#include "helper.hpp"

/**
 * @brief  Poll UART for data ending in newline
 * @param  interface Hardware UART interface
 * @retval Data
 */
String UARTRecvPoll(HardwareSerial *interface)
{
    String buffer = "";
    for (;;)
    {
        if (interface->available())
        {
            char c = interface->read();
            if (c == '\n')
            {
                buffer += String('\0');
                break;
            }
            buffer += String(c);
        }
    }
    return buffer;
}

/**
 * @brief  Read from flash memory
 * @param  memory Hardware memory interface
 * @param  data Data object to write to
 * @retval None
 */
void readMemory(EEPROMClass *memory, MemoryData *data)
{
    int index = 0;
    int i = 0;

    data->enableWifi = false;
    if (memory->read(index) != 0)
    {
        data->enableWifi = true;
    }
    index++;

    data->ssid = "";
    for (i = 0; i < 32; i++)
    {
        char c = memory->read(index + i);
        if (c == '\0') continue;
        data->ssid += String(c);
    }
    index += i;

    data->password = "";
    for (i = 0; i < 64; i++)
    {
        char c = memory->read(index + i);
        if (c == '\0') continue;
        data->password += String(c);
    }
    index += i;

    data->enableNetTime = false;
    if (memory->read(index) != 0)
    {
        data->enableNetTime = true;
    }
    index++;

    data->timezone = "";
    for (i = 0; i < 64; i++)
    {
        char c = memory->read(index + i);
        if (c == '\0') continue;
        data->timezone += String(c);
    }
    index += i;

    data->tubeCurrent = memory->read(index);
}

/**
 * @brief  Write to flash memory
 * @param  memory Hardware memory interface
 * @param  data Data object to read from
 * @retval None
 */
void writeMemory(EEPROMClass *memory, const MemoryData *data)
{
    int index = 0;
    int i = 0;

    if (data->enableWifi)
    {
        memory->write(index, 1);
    }
    else
    {
        memory->write(index, 0);
    }
    index++;

    for (i = 0; i < 32; i++)
    {
        if (i < data->ssid.length())
            memory->write(index + i, data->ssid.charAt(i));
        else
            memory->write(index + i, '\0');
    }
    index += i;

    for (i = 0; i < 64; i++)
    {
        if (i < data->password.length())
            memory->write(index + i, data->password.charAt(i));
        else
            memory->write(index + i, '\0');
    }
    index += i;

    if (data->enableNetTime)
    {
        memory->write(index, 1);
    }
    else
    {
        memory->write(index, 0);
    }
    index++;

    for (i = 0; i < 64; i++)
    {
        if (i < data->timezone.length())
            memory->write(index + i, data->timezone.charAt(i));
        else
            memory->write(index + i, '\0');
    }
    index += i;

    memory->write(index, data->tubeCurrent);

    memory->commit();
}

String parsePacket(uint8_t tubeCurrent)
{
    String buffer = "2000000";
    if (tubeCurrent < 100) buffer += "0";
    if (tubeCurrent < 10) buffer += "0";
    buffer += String((int)tubeCurrent);

    return buffer;
}

String parsePacket(String resp, uint8_t tubeCurrent)
{
    nlohmann::json res = nlohmann::json::parse(resp);

    String buffer = "3";

    if (res["hour"].get<int>() < 10) buffer += "0";
    buffer += String(res["hour"].get<int>());

    if (res["minute"].get<int>() < 10) buffer += "0";
    buffer += String(res["minute"].get<int>());

    if (res["seconds"].get<int>() < 10) buffer += "0";
    buffer += String(res["seconds"].get<int>());

    if (tubeCurrent < 100) buffer += "0";
    if (tubeCurrent < 10) buffer += "0";
    buffer += String((int)tubeCurrent);

    return buffer;
}

String parsePacket(String hour, String minute, String second, uint8_t tubeCurrent)
{
    String buffer = "3";

    if (hour.length() < 2) buffer += "0";
    buffer += hour;

    if (minute.length() < 2) buffer += "0";
    buffer += minute;

    if (second.length() < 2) buffer += "0";
    buffer += second;

    if (tubeCurrent < 100) buffer += "0";
    if (tubeCurrent < 10) buffer += "0";
    buffer += String((int)tubeCurrent);

    return buffer;
}
