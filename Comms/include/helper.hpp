/**
 ******************************************************************************
 * @file           : helper.hpp
 * @brief          : Header for helper.cpp
 ******************************************************************************
 */

#ifndef HELPER_HPP_
#define HELPER_HPP_

#include "main.hpp"

struct MemoryData
{
    char ssid[32];
    char password[64];
    char timezone[64];
    uint8_t tubeCurrent;
};

void UARTRecvPoll(HardwareSerial *interface, char *buffer);

void readMemory(EEPROMClass *memory, MemoryData *data);
void writeMemory(EEPROMClass *memory, const MemoryData *data);

#endif
