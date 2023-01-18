/**
 ******************************************************************************
 * @file           : helper.cpp
 * @brief          : Helper functions
 ******************************************************************************
 */

#include "helper.hpp"

void UARTRecvPoll(HardwareSerial *interface, char *buffer)
{
    int i = 0;
    for (;;)
    {
        if (interface->available())
        {
            buffer[i] = interface->read();
            if (buffer[i] == '\n')
            {
                buffer[i] = '\0';
                break;
            }
            i++;
        }
    }
    return;
}

void readMemory(EEPROMClass *memory, MemoryData *data)
{
    int index = 0;
    int i = 0;

    for (i = 0; i < 32; i++)
    {
        data->ssid[i] = memory->read(index + i);
    }
    index += i;

    for (i = 0; i < 64; i++)
    {
        data->password[i] = memory->read(index + i);
    }
    index += i;

    for (i = 0; i < 64; i++)
    {
        data->timezone[i] = memory->read(index + i);
    }
    index += i;

    data->tubeCurrent = memory->read(index);
}

void writeMemory(EEPROMClass *memory, const MemoryData *data)
{
    int index = 0;
    int i = 0;

    for (i = 0; i < 32; i++)
    {
        memory->write(index + i, data->ssid[i]);
    }
    index += i;

    for (i = 0; i < 64; i++)
    {
        memory->write(index + i, data->password[i]);
    }
    index += i;

    for (i = 0; i < 64; i++)
    {
        memory->write(index + i, data->timezone[i]);
    }
    index += i;

    memory->write(index, data->tubeCurrent);

    memory->commit();
}
