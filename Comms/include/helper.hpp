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
    bool enableWifi;
    String ssid;
    String password;

    bool enableNetTime;
    String timezone;

    uint8_t tubeCurrent;
};

String UARTRecvPoll(HardwareSerial *interface);

void readMemory(EEPROMClass *memory, MemoryData *data);
void writeMemory(EEPROMClass *memory, const MemoryData *data);

String parsePacket(uint8_t tubeCurrnet);
String parsePacket(String resp, uint8_t tubeCurrent);

#endif
