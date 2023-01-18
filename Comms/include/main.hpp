/**
 ******************************************************************************
 * @file           : main.hpp
 * @brief          : Header for main.cpp
 ******************************************************************************
 */

#ifndef MAIN_HPP_
#define MAIN_HPP_

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP.h>
#include <String.h>
#include <WebServer.h>
#include <WiFi.h>

void runWebserver();
void serverHandler();

#endif
