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
