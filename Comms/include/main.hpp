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
#include <HTTPClient.h>
#include <String.h>
#include <WebServer.h>
#include <WiFi.h>

#include "extern/json.hpp"

void serverRootHandler();
void serverWifiHandler();
void serverTimeHandler();
void serverTubeHandler();

#endif
