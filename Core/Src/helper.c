/**
 ******************************************************************************
 * @file           : helper.c
 * @brief          : Helper functions
 ******************************************************************************
 */

#include <stdlib.h>
#include <string.h>

#include "helper.h"

/**
 * @brief  Gets current time stored in RTC
 * @param  hi2c I2C handle of RTC
 * @param  dateTime DateTime struct to receive data from RTC
 * @retval None
 */
void RTC_getTime(I2C_HandleTypeDef *hi2c, DateTime *dateTime)
{
    uint8_t buffer[7];

    if (HAL_I2C_Mem_Read(hi2c, 0xD0, 0, 1, buffer, 7, 1000) != HAL_OK) Error_Handler();

    dateTime->second = (buffer[0] & 0x0F) + 10 * ((buffer[0] & 0x70) >> 4);
    dateTime->minute = (buffer[1] & 0x0F) + 10 * ((buffer[1] & 0x70) >> 4);
    dateTime->hour = (buffer[2] & 0x0F) + 10 * ((buffer[2] & 0x30) >> 4);
    dateTime->weekday = buffer[3] & 0x07;
    dateTime->day = (buffer[4] & 0x0F) + 10 * ((buffer[4] & 0x30) >> 4);
    dateTime->month = (buffer[5] & 0x0F) + 10 * ((buffer[5] & 0x10) >> 4);
    dateTime->year = (buffer[6] & 0x0F) + 10 * ((buffer[6] & 0xF0) >> 4);

    return;
}

/**
 * @brief  Sets RTC with given time
 * @param  hi2c I2C handle of RTC
 * @param  dateTime DateTime struct containing data to write to RTC
 * @retval None
 */
void RTC_setTime(I2C_HandleTypeDef *hi2c, const DateTime *dateTime)
{
    uint8_t buffer[7];
    buffer[0] = ((dateTime->second / 10) << 4) | (dateTime->second % 10);
    buffer[1] = ((dateTime->minute / 10) << 4) | (dateTime->minute % 10);
    buffer[2] = ((dateTime->hour / 10) << 4) | (dateTime->hour % 10);
    buffer[3] = dateTime->weekday;
    buffer[4] = ((dateTime->day / 10) << 4) | (dateTime->day % 10);
    buffer[5] = ((dateTime->month / 10) << 4) | (dateTime->month % 10);
    buffer[6] = ((dateTime->year / 10) << 4) | (dateTime->year % 10);

    if (HAL_I2C_Mem_Write(hi2c, 0xD0, 0, 1, buffer, 7, 1000) != HAL_OK) Error_Handler();

    return;
}

/**
 * @brief  Initializes DAC
 * @param  hspi SPI handle of DAC
 * @param  nCS_Port SPI nCS pin port
 * @param  nCS_Pin SPI nCS pin
 * @retval None
 */
void DAC_init(SPI_HandleTypeDef *hspi, GPIO_TypeDef *nCS_Port, uint16_t nCS_Pin)
{
    uint8_t buffer[2];

    // Power down release, 0b1001 XXXXXXXXX
    buffer[0] = 0x09;
    buffer[1] = 0xFF;
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 0);
    if (HAL_SPI_Transmit(hspi, buffer, 2, 1000) != HAL_OK) Error_Handler();
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 1);

    // Set all IO to DAC mode, 0b0011 11111111
    buffer[0] = 0x03;
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 0);
    if (HAL_SPI_Transmit(hspi, buffer, 2, 1000) != HAL_OK) Error_Handler();
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 1);

    // Set all IO to output mode, 0b1111 11111111
    buffer[0] = 0x0F;
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 0);
    if (HAL_SPI_Transmit(hspi, buffer, 2, 1000) != HAL_OK) Error_Handler();
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 1);

    return;
}

/**
 * @brief  Sets one DAC channel
 * @param  hspi SPI handle of DAC
 * @param  nCS_Port SPI nCS pin port
 * @param  nCS_Pin SPI nCS pin
 * @param  channel DAC output channel (1 ~ 6, 1 -> rightmost digit)
 * @param  value DAC output value (0 ~ 255)
 * @retval None
 */
void DAC_setChannel(SPI_HandleTypeDef *hspi, GPIO_TypeDef *nCS_Port, uint16_t nCS_Pin, uint8_t channel, uint8_t value)
{
    channel = channel & 0x0F;
    channel = (channel & 0xC) >> 2 | (channel & 0x3) << 2;
    channel = (channel & 0xA) >> 1 | (channel & 0x5) << 1;

    uint8_t buffer[2];
    buffer[0] = channel;
    buffer[1] = value;

    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 0);
    if (HAL_SPI_Transmit(hspi, buffer, 2, 1000) != HAL_OK) Error_Handler();
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 1);

    return;
}

/**
 * @brief  Sets all DAC channels
 * @param  hspi SPI handle of DAC
 * @param  nCS_Port SPI nCS pin port
 * @param  nCS_Pin SPI nCS pin
 * @param  value DAC output value (0 ~ 255)
 * @retval None
 */
void DAC_setAll(SPI_HandleTypeDef *hspi, GPIO_TypeDef *nCS_Port, uint16_t nCS_Pin, uint8_t value)
{
    DAC_setChannel(hspi, nCS_Port, nCS_Pin, 1, value);
    DAC_setChannel(hspi, nCS_Port, nCS_Pin, 2, value);
    DAC_setChannel(hspi, nCS_Port, nCS_Pin, 3, value);
    DAC_setChannel(hspi, nCS_Port, nCS_Pin, 4, value);
    DAC_setChannel(hspi, nCS_Port, nCS_Pin, 5, value);
    DAC_setChannel(hspi, nCS_Port, nCS_Pin, 6, value);

    return;
}

/**
 * @brief  Clears all digits (sets analog mux to channel 15)
 * @param  hspi SPI handle of shift register
 * @param  nCS_Port SPI nCS pin port
 * @param  nCS_Pin SPI nCS pin
 * @retval None
 */
void SR_clearDigits(SPI_HandleTypeDef *hspi, GPIO_TypeDef *nCS_Port, uint16_t nCS_Pin)
{
    uint8_t buffer[3] = {0xFF, 0xFF, 0xFF};

    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 0);
    if (HAL_SPI_Transmit(hspi, buffer, 3, 1000) != HAL_OK) Error_Handler();
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 1);

    return;
}

/**
 * @brief  Sets digits
 * @param  hspi SPI handle of shift register
 * @param  nCS_Port SPI nCS pin port
 * @param  nCS_Pin SPI nCS pin
 * @param  digits Array of digits to be set, must be size 6 (digits[0] -> rightmost digit)
 * @retval None
 */
void SR_setDigits(SPI_HandleTypeDef *hspi, GPIO_TypeDef *nCS_Port, uint16_t nCS_Pin, uint8_t *digits)
{
    uint8_t buffer[3];
    buffer[0] = (digits[0] << 4) | digits[1];
    buffer[1] = (digits[2] << 4) | digits[3];
    buffer[2] = (digits[4] << 4) | digits[5];

    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 0);
    if (HAL_SPI_Transmit(hspi, buffer, 3, 1000) != HAL_OK) Error_Handler();
    HAL_GPIO_WritePin(nCS_Port, nCS_Pin, 1);

    return;
}

/**
 * @brief  Generates a DateTime object from a HTTP response packet
 * @param  huart UART handle of WiFi module
 * @param  dateTime DateTime object
 * @param  api Time API URL
 * @param  length Length of api string
 * @retval 0 for successful parse, -1 for error
 */
int getInternetTime(UART_HandleTypeDef *huart, DateTime *dateTime, const char *api, int length)
{
    dateTime->year = 0;

    // Get data packet
    // AT+HTTPCLIENT=2,3,"{TIME_API}",,,1
    if (HAL_UART_Transmit(huart, (uint8_t *)"AT+HTTPCLIENT=2,3,\"", 19, 1000) != HAL_OK) Error_Handler();
    if (HAL_UART_Transmit(huart, (uint8_t *)api, length - 1, 1000) != HAL_OK) Error_Handler();
    if (HAL_UART_Transmit(huart, (uint8_t *)"\",,,1\r\n", 7, 1000) != HAL_OK) Error_Handler();

    char res[128];
    memset(res, 0, 128);
    HAL_UART_Receive(huart, (uint8_t *)res, 128, 5000);

    // Parse data packet
    int ptr = 0;
    char temp[8];
    for (int i = 0; i < 128; i++)
    {
        if (res[i] == 0) return -1;
        if (res[i] == ',')
        {
            ptr = i + 1;
            break;
        }
    }
    for (int i = ptr; i < 128; i++)
    {
        if (res[i] == 0) return -1;
        if (res[i] == '\n')
        {
            ptr = i + 1;
            break;
        }
    }
    for (int i = ptr; i < 128; i++)
    {
        if (res[i] == 0) return -1;
        if (res[i] == '\n')
        {
            ptr = i + 1;
            break;
        }
    }
    for (int i = ptr; i < 128; i++)
    {
        if (res[i] == 0) return -1;
        if (res[i] == ' ')
        {
            ptr = i + 1;
            break;
        }
    }

    if (ptr == 0) return -1;

    ptr += 2;
    memset(temp, 0, 8);
    strncpy(temp, res + ptr, 2);
    dateTime->year = atoi(temp);

    ptr += 3;
    memset(temp, 0, 8);
    strncpy(temp, res + ptr, 2);
    dateTime->month = atoi(temp);

    ptr += 3;
    memset(temp, 0, 8);
    strncpy(temp, res + ptr, 2);
    dateTime->day = atoi(temp);

    ptr += 3;
    memset(temp, 0, 8);
    strncpy(temp, res + ptr, 2);
    dateTime->hour = atoi(temp);

    ptr += 3;
    memset(temp, 0, 8);
    strncpy(temp, res + ptr, 2);
    dateTime->minute = atoi(temp);

    ptr += 3;
    memset(temp, 0, 8);
    strncpy(temp, res + ptr, 2);
    dateTime->second = atoi(temp);

    for (int i = ptr; i < 128; i++)
    {
        if (res[i] == 0) return -1;
        if (res[i] == '\n')
        {
            ptr = i + 1;
            break;
        }
    }
    for (int i = ptr; i < 128; i++)
    {
        if (res[i] == 0) return -1;
        if (res[i] == ' ')
        {
            ptr = i + 1;
            break;
        }
    }

    memset(temp, 0, 8);
    strncpy(temp, res + ptr, 1);
    dateTime->weekday = atoi(temp);

    if (dateTime->year != 0) return 0;

    return -1;
}
