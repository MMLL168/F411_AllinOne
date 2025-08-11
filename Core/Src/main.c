/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "crc.h"
#include "dma.h"
#include "fatfs.h"
#include "i2s.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>  // 用於 rand() 隨機數
#include <sys/unistd.h>
//#include "app_x-cube-ai.h"
#include <stdbool.h> // <-- 請在這裡加入這一行
#include "helloworld.h"

#include "ili9341.h"
#include "oscilloscope.h"
#include "fatfs.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define DWT_CONTROL *(volatile unsigned long *)0xE0001000
#define SCB_DEMCR   *(volatile unsigned long *)0xE000EDFC
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
FATFS fs;
FATFS *pfs;
FIL fil;
FRESULT fres;
DWORD fre_clust;
uint32_t totalSpace, freeSpace;
char buffer[100];
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
ILI9341TypeDef display;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 讓 printf 可以透過 UART2 輸出，方便除錯
//#ifdef __GNUC__
//#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
//#else
//#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
//#endif
//
//PUTCHAR_PROTOTYPE
//{
//  HAL_UART_Transmit(&huart6, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
//  return ch;
//}
//
//int _write(int file, char *ptr, int len)
//{
//    HAL_UART_Transmit(&huart6, (uint8_t*)ptr, len, 100);
//    return len;
//}


#define I2S_BUFFER_SIZE 4//2048 // 緩衝區大小，需要根據模型要求調整

// 定義一個32位元的緩衝區來接收 I2S 資料 (24-bit data in 32-bit frame)
uint32_t i2s_rx_buffer[I2S_BUFFER_SIZE];
// 用於標示哪個緩衝區已滿可以處理
volatile int buffer_is_ready = 0;

unsigned cb_cnt=0;
uint32_t val24;
int val32;


/**
  * @brief  I2S 接收半滿回呼函式
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
  // DMA 已經填充完緩衝區的前半部分
  // 在這裡呼叫語音辨識函式，處理前半段的資料
  // speech_recognition_run(&i2s_rx_buffer[0], I2S_BUFFER_SIZE / 2);
  buffer_is_ready = 1; // 示意：設定旗標
	if(hi2s==&hi2s2){
		cb_cnt++;//回调次数计数
		//将两个32整型合并为一个
		//dat32 example: 0000fffb 00004f00
		//printf("%x\r\n",data_i2s[0]<<8);
		//printf("%x\r\n",data_i2s[1]>>8);

		val24=(i2s_rx_buffer[0]<<8)+(i2s_rx_buffer[1]>>8);
		//printf("%d\r\n",val24);
      //将24位有符号整型扩展到32位
		if(val24 & 0x800000)
		{//negative
			val32=0xff000000 | val24;
		}
		else
		{//positive
			val32=val24;
		}
		//以采样频率的十分之一，串口发送采样值
		if(cb_cnt%10==0)
			printf("%d\r\n",val32);
	}
}

/**
  * @brief  I2S 接收全滿回呼函式
  * @param  hi2s: I2S handle
  * @retval None
  */
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
  // DMA 已經填充完緩衝區的後半部分
  // 在這裡呼叫語音辨識函式，處理後半段的資料
  // speech_recognition_run(&i2s_rx_buffer[I2S_BUFFER_SIZE / 2], I2S_BUFFER_SIZE / 2);
  buffer_is_ready = 2; // 示意：設定旗標
	if(hi2s==&hi2s2){
		cb_cnt++;//回调次数计数
		//将两个32整型合并为一个
		//dat32 example: 0000fffb 00004f00
		//printf("%x\r\n",data_i2s[0]<<8);
		//printf("%x\r\n",data_i2s[1]>>8);

		val24=(i2s_rx_buffer[0]<<8)+(i2s_rx_buffer[1]>>8);
		//printf("%d\r\n",val24);
      //将24位有符号整型扩展到32位
		if(val24 & 0x800000)
		{//negative
			val32=0xff000000 | val24;
		}
		else
		{//positive
			val32=val24;
		}
		//以采样频率的十分之一，串口发送采样值
		if(cb_cnt%10==0)
			printf("%d\r\n",val32);
	}
}

// 假設這是一個讀取感測器數據的函式 (您需要自己實現)
float get_sensor_reading(void)
{
    // 在這裡填寫您獲取真實數據的程式碼
    // 例如：從 ADC 讀取數據並轉換成浮點數
    // 此處僅為範例，回傳一個隨機變化的值
    return 150.0f + (float)(rand() % 50);
}

// --- UART 接收相關變數 ---
#define UART_RX_BUFFER_SIZE 10
uint8_t uart_rx_buffer[UART_RX_BUFFER_SIZE]; // 用於儲存從 UART 收到的原始字元
uint8_t rx_byte; // 用於 HAL 函式，儲存單個接收位元組

volatile uint8_t uart_rx_index = 0; // 目前接收到緩衝區的位置
volatile bool new_data_received = false; // 新數據接收完成的旗標

// --- AI 推論相關變數 ---
// 這個變數將連接 main.c 和 app_x-cube-ai.c
int score_from_uart = 0;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  // 確保是我們想要的 UART (例如 USART2) 觸發的中斷
  if (huart->Instance == USART6)
  {
    // 檢查是否是換行符 (Enter 鍵)，或者緩衝區是否已滿
    if (rx_byte == '\r' || rx_byte == '\n' || uart_rx_index >= (UART_RX_BUFFER_SIZE - 1))
    {
      // 1. 在字串末尾加上結束符 '\0'
      uart_rx_buffer[uart_rx_index] = '\0';

      // 2. 設定新數據旗標，通知主迴圈處理
      if (uart_rx_index > 0) // 確保不是空指令
      {
        new_data_received = true;
      }

      // 3. 重置索引，準備下一次接收
      uart_rx_index = 0;
    }
    else
    {
      // 將收到的位元組存入緩衝區，並移動索引
      uart_rx_buffer[uart_rx_index++] = rx_byte;
    }

    // !!! 非常重要：重新啟動 UART 中斷接收，準備接收下一個位元組 !!!
    HAL_UART_Receive_IT(&huart6, &rx_byte, 1);
  }
}

uint16_t txData;
int txIndex;
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim->Instance == TIM2)
  {
    txData = ((uint16_t)helloworld[txIndex+1] << 8) | helloworld[txIndex];
    txIndex = txIndex + 2;
    if(txIndex>53456) txIndex = 0;
    HAL_I2S_Transmit(&hi2s3, &txData, 1, 10);
  }

	if(htim->Instance == TIM10)
	{
		if (adc_available) {
			adc_available = 0;
			HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_data, 2);
		}
	}
}


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
    	if (adc_reset_cyccnt) {
    		DWT->CYCCNT = 0U;
    		adc0_time_delta = 0;
    		adc1_time_delta = 0;
    		adc_reset_cyccnt = 0;
    	}

    	if (!adc0_filled) {
			adc0_time[adc0_length] = (DWT->CYCCNT - adc0_time_delta) / (SystemCoreClock / 1000000);
			adc0[adc0_length] = adc_data[0] * ADC_CHANNEL0_SCALE;

			if (adc_max[0] < adc0[adc0_length])
				adc_max[0] = adc0[adc0_length];

			if (adc_min[0] > adc0[adc0_length])
				adc_min[0] = adc0[adc0_length];

			if (adc0_length < (ADC_BUFFER_SIZE - 1)) {

				uint8_t trigger = 0;

				if (trigger_mode == 0)
					trigger = (adc0_prev < trigger0_value && adc0[adc0_length] > trigger0_value);
				else
					trigger = (adc0_prev > trigger0_value && adc0[adc0_length] < trigger0_value);

				if (trigger && adc0_length != 0) {
					if (!event_trigger0_detected) {
						adc0_length = 0;
						adc0_time_delta = DWT->CYCCNT;
						event_trigger0_detected = 1;
					} else if (!adc_period0_detected) {
						adc_period[0] = adc0_time[adc0_length];
						adc_period0_detected = 1;
					}
				}

				adc0_prev = adc_data[0] * ADC_CHANNEL0_SCALE;
				adc0_length++;

			} else
				adc0_filled = 1;
    	}

    	if (!adc1_filled) {
			adc1_time[adc1_length] = (DWT->CYCCNT - adc1_time_delta) / (SystemCoreClock / 1000000);
			adc1[adc1_length] = adc_data[1] * ADC_CHANNEL1_SCALE;

			if (adc_max[1] < adc1[adc1_length])
				adc_max[1] = adc1[adc1_length];

			if (adc_min[1] > adc1[adc1_length])
				adc_min[1] = adc1[adc1_length];

			if (adc1_length < (ADC_BUFFER_SIZE - 1)) {

				uint8_t trigger = 0;

				if (trigger_mode == 0)
					trigger = (adc1_prev < trigger1_value && adc1[adc1_length] > trigger1_value);
				else
					trigger = (adc1_prev > trigger1_value && adc1[adc1_length] < trigger1_value);

				if (trigger && adc1_length != 0) {
					if (!event_trigger1_detected) {
						adc1_length = 0;
						adc1_time_delta = DWT->CYCCNT;
						event_trigger1_detected = 1;
					} else if (!adc_period1_detected) {
						adc_period[1] = adc1_time[adc1_length];
						adc_period1_detected = 1;
					}
				}

				adc1_prev = adc_data[1] * ADC_CHANNEL1_SCALE;
				adc1_length++;

			} else
				adc1_filled = 1;
    	}

		if (adc0_filled && adc1_filled) {
			event_adc = 1;

	    	if (!adc_immediate) {
				HAL_TIM_Base_Stop_IT(&htim10);
				adc_available = 1;
	    	}

    		return;
		}

		if (adc_immediate)
			HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_data, 2);
		else
			adc_available = 1;
    }
}

/* 定義測試用變量 */
FATFS fs;           // FatFs檔案系統物件
FIL fil;           // 檔案物件
FRESULT fres;      // FatFs返回值
UINT br, bw;       // 檔案讀寫計數

/* 測試用的簡單字符串 */
char writeText[] = "Hello SD Card!";    // 要寫入的文字
char readText[20];                      // 讀取緩衝區
void SD_Test(void)
{
    char test_data[] = "SD Card Test Data";
    char read_data[30] = {0};

    // 1. 初始化並掛載文件系統
    printf("Initializing SD card...\r\n");
    fres = f_mount(&fs, "", 1);
    if(fres != FR_OK)
    {
        printf("f_mount error (%i)\r\n", fres);
        return;
    }
    printf("SD card mounted successfully!\r\n");

    // 2. 寫入測試
    printf("Writing test data...\r\n");
    fres = f_open(&fil, "test.txt", FA_WRITE | FA_CREATE_ALWAYS);
    if(fres != FR_OK)
    {
        printf("f_open error (%i)\r\n", fres);
        return;
    }

    fres = f_write(&fil, test_data, strlen(test_data), &bw);
    if(fres != FR_OK)
    {
        printf("f_write error (%i)\r\n", fres);
        f_close(&fil);
        return;
    }
    f_close(&fil);
    printf("Test data written successfully!\r\n");

    // 3. 讀取測試
    printf("Reading test data...\r\n");
    fres = f_open(&fil, "test.txt", FA_READ);
    if(fres != FR_OK)
    {
        printf("f_open error (%i)\r\n", fres);
        return;
    }

    f_read(&fil, read_data, strlen(test_data), &br);
    f_close(&fil);

    // 4. 驗證數據
    if(strcmp(test_data, read_data) == 0)
    {
        printf("Test passed! Read data: %s\r\n", read_data);
    }
    else
    {
        printf("Test failed!\r\n");
        printf("Expected: %s\r\n", test_data);
        printf("Got: %s\r\n", read_data);
    }
}

void _Error_Handler(char * file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_I2S2_Init();
  MX_USART6_UART_Init();
  MX_CRC_Init();
  MX_I2S3_Init();
  MX_TIM2_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_FATFS_Init();
  MX_SPI4_Init();
  MX_TIM10_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  //while(1)
  {
  printf("Starting SD card test...\r\n");
  SD_Test();
  HAL_Delay(1000); // 等待1秒鐘

  }
#if 1
	/* Mount SD Card */
	if(f_mount(&fs, "", 0) != FR_OK)
		_Error_Handler(__FILE__, __LINE__);

	/* Open file to write */
	if(f_open(&fil, "first.txt", FA_OPEN_ALWAYS | FA_READ | FA_WRITE) != FR_OK)
		_Error_Handler(__FILE__, __LINE__);

	/* Check freeSpace space */
	if(f_getfree("", &fre_clust, &pfs) != FR_OK)
		_Error_Handler(__FILE__, __LINE__);

	totalSpace = (uint32_t)((pfs->n_fatent - 2) * pfs->csize * 0.5);
	freeSpace = (uint32_t)(fre_clust * pfs->csize * 0.5);

	/* free space is less than 1kb */
	if(freeSpace < 1)
		_Error_Handler(__FILE__, __LINE__);

	/* Writing text */
	f_puts("STM32 SD Card I/O Example via SPI\n", &fil);
	f_puts("Save the world!!!", &fil);

	/* Close file */
	if(f_close(&fil) != FR_OK)
		_Error_Handler(__FILE__, __LINE__);

	/* Open file to read */
	if(f_open(&fil, "first.txt", FA_READ) != FR_OK)
		_Error_Handler(__FILE__, __LINE__);

	while(f_gets(buffer, sizeof(buffer), &fil))
	{
		/* SWV output */
		printf("%s", buffer);
		fflush(stdout);
	}

	/* Close file */
	if(f_close(&fil) != FR_OK)
		_Error_Handler(__FILE__, __LINE__);

	/* Unmount SDCARD */
	if(f_mount(NULL, "", 1) != FR_OK)
		_Error_Handler(__FILE__, __LINE__);

	//while(1)
	  {

	  }
#endif

#if 0
  if (HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)i2s_rx_buffer, I2S_BUFFER_SIZE) != HAL_OK)
  {
      Error_Handler();
  }
#endif

  if (HAL_UART_Receive_IT(&huart6, &rx_byte, 1) != HAL_OK)
  {
      Error_Handler();
  }

  printf("AI Model Application Started...\r\n");


  txIndex = 0;

  // For DWT->CYCCNT ...
  SCB_DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT_CONTROL |= DWT_CTRL_CYCCNTENA_Msk;

  adc_reset_cyccnt = 1;
  if (adc_immediate)
  {
	  // The ADC starts immediately after the previous measurement is handled
	  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_data, 2);
  }
  else
  {
	  // ADC starts by timer
	  HAL_TIM_Base_Start_IT(&htim10);
  }

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

  InitOscState();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
	 if(HAL_GPIO_ReadPin(B1_GPIO_Port, B1_Pin) == GPIO_PIN_RESET)
	 {

		  HAL_TIM_Base_Start_IT(&htim2);
	 }
	 else
	 {
		  txIndex = 0;
		  HAL_TIM_Base_Stop_IT(&htim2);
	 }

	 Oscilloscope_Process();
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SM = 16;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
