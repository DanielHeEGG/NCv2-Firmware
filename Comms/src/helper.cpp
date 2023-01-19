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

String parsePacket(String resp, uint8_t tubeCurrent)
{
    nlohmann::json res = nlohmann::json::parse(resp);

    String buffer = "3";

    buffer += String(res["year"].get<int>());

    if (res["month"].get<int>() < 10) buffer += "0";
    buffer += String(res["month"].get<int>());

    if (res["day"].get<int>() < 10) buffer += "0";
    buffer += String(res["day"].get<int>());

    if (res["hour"].get<int>() < 10) buffer += "0";
    buffer += String(res["hour"].get<int>());

    if (res["minute"].get<int>() < 10) buffer += "0";
    buffer += String(res["minute"].get<int>());

    if (res["seconds"].get<int>() < 10) buffer += "0";
    buffer += String(res["seconds"].get<int>());

    if (res["dayOfWeek"].get<std::string>().compare("Monday") == 0)
        buffer += "1";
    else if (res["dayOfWeek"].get<std::string>().compare("Tuesday") == 0)
        buffer += "2";
    else if (res["dayOfWeek"].get<std::string>().compare("Wednesday") == 0)
        buffer += "3";
    else if (res["dayOfWeek"].get<std::string>().compare("Thursday") == 0)
        buffer += "4";
    else if (res["dayOfWeek"].get<std::string>().compare("Friday") == 0)
        buffer += "5";
    else if (res["dayOfWeek"].get<std::string>().compare("Saturday") == 0)
        buffer += "6";
    else if (res["dayOfWeek"].get<std::string>().compare("Sunday") == 0)
        buffer += "7";
    else
        buffer += "0";

    if (tubeCurrent < 100) buffer += "0";
    if (tubeCurrent < 10) buffer += "0";
    buffer += String((int)tubeCurrent);

    return buffer;
}
