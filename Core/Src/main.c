/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os2.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>

#include "helper.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* Definitions for taskDisplay */
osThreadId_t taskDisplayHandle;
const osThreadAttr_t taskDisplay_attributes = {
    .name = "taskDisplay",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityHigh,
};
/* Definitions for taskGetTime */
osThreadId_t taskGetTimeHandle;
const osThreadAttr_t taskGetTime_attributes = {
    .name = "taskGetTime",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityNormal,
};
/* USER CODE BEGIN PV */

// SET THIS BEFORE FLASHING
const char WIFI_SSID[] = "";

// SET THIS BEFORE FLASHING
const char WIFI_PASSWORD[] = "";

// SET THIS BEFORE FLASHING
// See http://worldtimeapi.org/timezones for full list of timezones
const char TIMEZONE[] = "";

// SET THIS BEFORE FLASHING
// 0mA -> 0, 10.6mA -> 255, linear
const uint8_t TUBE_CURRENT = 0;

const char API_URL[] = "http://worldtimeapi.org/api/timezone/";
char TIME_API[sizeof(API_URL) + sizeof(TIMEZONE) - 1 + 4];

DateTime currentTime;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
static void MX_USART2_UART_Init(void);
void startTaskDisplay(void *argument);
void startTaskGetTime(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */

    // Format API URL string
    memset(TIME_API, 0, sizeof(TIME_API));
    strcpy(TIME_API, API_URL);
    strcat(TIME_API, TIMEZONE);
    strcat(TIME_API, ".txt");

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    MX_SPI2_Init();
    MX_USART2_UART_Init();
    /* USER CODE BEGIN 2 */

    // Power on setup
    HAL_GPIO_WritePin(HV_SHDN_GPIO_Port, HV_SHDN_Pin, 0);
    DAC_init(&DAC_SPI, DAC_nCS_GPIO_Port, DAC_nCS_Pin);
    DAC_setAll(&DAC_SPI, DAC_nCS_GPIO_Port, DAC_nCS_Pin, TUBE_CURRENT);
    SR_clearDigits(&SR_SPI, SR_nCS_GPIO_Port, SR_nCS_Pin);
    HAL_GPIO_WritePin(ESP_RUN_GPIO_Port, ESP_RUN_Pin, 1);

    // Reset WiFi module
    HAL_GPIO_WritePin(ESP_nRST_GPIO_Port, ESP_nRST_Pin, 0);
    HAL_Delay(100);
    HAL_GPIO_WritePin(ESP_nRST_GPIO_Port, ESP_nRST_Pin, 1);

    // Cycle all digits while waiting for WiFi module startup
    for (int i = 0; i < 10; i++)
    {
        uint8_t digits[] = {i, i, i, i, i, i};
        SR_setDigits(&SR_SPI, SR_nCS_GPIO_Port, SR_nCS_Pin, digits);
        HAL_Delay(1000);
    }

    // Check factory reset jumper
    if (HAL_GPIO_ReadPin(JMP_nRST_GPIO_Port, JMP_nRST_Pin) == 0)
    {
        // Display all zeros
        uint8_t digits[] = {0, 0, 0, 0, 0, 0};
        SR_setDigits(&SR_SPI, SR_nCS_GPIO_Port, SR_nCS_Pin, digits);

        // Restore WiFi module default
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)"AT+RESTORE\r\n", 12, 1000) != HAL_OK) Error_Handler();
        HAL_Delay(10000);

        // Station mode
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)"AT+CWMODE=1\r\n", 13, 1000) != HAL_OK) Error_Handler();
        HAL_Delay(1000);

        // Connect to WiFi
        // AT+CWJAP="{WIFI_SSID}","{WIFI_PASSWORD}"
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)"AT+CWJAP=\"", 10, 1000) != HAL_OK) Error_Handler();
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)WIFI_SSID, sizeof(WIFI_SSID) - 1, 1000) != HAL_OK) Error_Handler();
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)"\",\"", 3, 1000) != HAL_OK) Error_Handler();
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)WIFI_PASSWORD, sizeof(WIFI_PASSWORD) - 1, 1000) != HAL_OK) Error_Handler();
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)"\"\r\n", 3, 1000) != HAL_OK) Error_Handler();
        HAL_Delay(10000);

        // Reconnect on startup
        if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)"AT+CWAUTOCONN=1\r\n", 15, 1000) != HAL_OK) Error_Handler();
        HAL_Delay(1000);
    }

    // Disable echo
    if (HAL_UART_Transmit(&ESP_UART, (uint8_t *)"ATE0\r\n", 6, 1000) != HAL_OK) Error_Handler();
    HAL_Delay(1000);

    // Load internet time to RTC & currentTime
    if (getInternetTime(&ESP_UART, &currentTime, TIME_API, sizeof(TIME_API)) == 0)
    {
        RTC_setTime(&RTC_I2C, &currentTime);
    }
    RTC_getTime(&RTC_I2C, &currentTime);

    /* USER CODE END 2 */

    /* Init scheduler */
    osKernelInitialize();

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* creation of taskDisplay */
    taskDisplayHandle = osThreadNew(startTaskDisplay, NULL, &taskDisplay_attributes);

    /* creation of taskGetTime */
    taskGetTimeHandle = osThreadNew(startTaskGetTime, NULL, &taskGetTime_attributes);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    /* USER CODE END RTOS_EVENTS */

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C1_Init 2 */

    /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void)
{

    /* USER CODE BEGIN SPI1_Init 0 */

    /* USER CODE END SPI1_Init 0 */

    /* USER CODE BEGIN SPI1_Init 1 */

    /* USER CODE END SPI1_Init 1 */
    /* SPI1 parameter configuration*/
    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI1_Init 2 */

    /* USER CODE END SPI1_Init 2 */
}

/**
 * @brief SPI2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI2_Init(void)
{

    /* USER CODE BEGIN SPI2_Init 0 */

    /* USER CODE END SPI2_Init 0 */

    /* USER CODE BEGIN SPI2_Init 1 */

    /* USER CODE END SPI2_Init 1 */
    /* SPI2 parameter configuration*/
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN SPI2_Init 2 */

    /* USER CODE END SPI2_Init 2 */
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

    /* USER CODE BEGIN USART2_Init 0 */

    /* USER CODE END USART2_Init 0 */

    /* USER CODE BEGIN USART2_Init 1 */

    /* USER CODE END USART2_Init 1 */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        Error_Handler();
    }
    /* USER CODE BEGIN USART2_Init 2 */

    /* USER CODE END USART2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOA, ESP_nRST_Pin | ESP_RUN_Pin | DAC_nCS_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOB, HV_SHDN_Pin | SR_nCS_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pins : ESP_nRST_Pin ESP_RUN_Pin DAC_nCS_Pin */
    GPIO_InitStruct.Pin = ESP_nRST_Pin | ESP_RUN_Pin | DAC_nCS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /*Configure GPIO pin : JMP_nRST_Pin */
    GPIO_InitStruct.Pin = JMP_nRST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(JMP_nRST_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : HV_SHDN_Pin SR_nCS_Pin */
    GPIO_InitStruct.Pin = HV_SHDN_Pin | SR_nCS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_startTaskDisplay */
/**
 * @brief  Function implementing the taskDisplay thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_startTaskDisplay */
void startTaskDisplay(void *argument)
{
    /* USER CODE BEGIN 5 */
    DateTime pastTime = currentTime;
    /* Infinite loop */
    for (;;)
    {
        if (currentTime.second != pastTime.second)
        {
            pastTime = currentTime;

            uint8_t digits[6];
            for (int i = 0; i < 10; i++)
            {
                digits[0] = i;
                digits[1] = i;
                digits[2] = i;
                digits[3] = i;
                digits[4] = i;
                digits[5] = i;
                SR_setDigits(&SR_SPI, SR_nCS_GPIO_Port, SR_nCS_Pin, digits);
                osDelay(5);
            }

            digits[0] = currentTime.second % 10;
            digits[1] = currentTime.second / 10;
            digits[2] = currentTime.minute % 10;
            digits[3] = currentTime.minute / 10;
            digits[4] = currentTime.hour % 10;
            digits[5] = currentTime.hour / 10;
            SR_setDigits(&SR_SPI, SR_nCS_GPIO_Port, SR_nCS_Pin, digits);
        }
        else
        {
            osDelay(100);
        }
    }
    /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_startTaskGetTime */
/**
 * @brief Function implementing the taskGetTime thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_startTaskGetTime */
void startTaskGetTime(void *argument)
{
    /* USER CODE BEGIN startTaskGetTime */
    /* Infinite loop */
    for (;;)
    {
        // Get time from RTC
        RTC_getTime(&RTC_I2C, &currentTime);

        // Attempt to get internet time every day
        if (currentTime.hour == 0)
        {
            DateTime dateTime;
            if (getInternetTime(&ESP_UART, &dateTime, TIME_API, sizeof(TIME_API)) == 0)
            {
                RTC_setTime(&RTC_I2C, &dateTime);
            }
        }
        osDelay(100);
    }
    /* USER CODE END startTaskGetTime */
}

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    /* USER CODE BEGIN Callback 0 */

    /* USER CODE END Callback 0 */
    if (htim->Instance == TIM1)
    {
        HAL_IncTick();
    }
    /* USER CODE BEGIN Callback 1 */

    /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
