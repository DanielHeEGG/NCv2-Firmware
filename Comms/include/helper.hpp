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
    String ssid;
    String password;
    String timezone;
    uint8_t tubeCurrent;
};

String UARTRecvPoll(HardwareSerial *interface);

void readMemory(EEPROMClass *memory, MemoryData *data);
void writeMemory(EEPROMClass *memory, const MemoryData *data);

#endif
