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

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

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


ILI9341TypeDef display;

volatile uint16_t adc_data[3] = { 0 };
volatile uint32_t adc0_length = 0;
volatile uint32_t adc1_length = 0;
volatile uint8_t  adc0_filled = 0;
volatile uint8_t  adc1_filled = 1;
volatile uint8_t  adc_available = 1;
volatile uint8_t  adc_reset_cyccnt = 1;

volatile uint16_t adc_max[2]    = {  0 };
volatile uint16_t adc_min[2]    = { -1 };
volatile uint32_t adc_period[2] = {  0 };
volatile uint8_t  adc_period0_detected = 0;
volatile uint8_t  adc_period1_detected = 0;
#define ADC_BUFFER_SIZE 2048//512
#define ADC_CHANNEL0_SCALE 1//11
#define ADC_CHANNEL1_SCALE 1
uint32_t adc0_time[ADC_BUFFER_SIZE];
uint32_t adc1_time[ADC_BUFFER_SIZE];
uint16_t adc0[ADC_BUFFER_SIZE];
uint16_t adc1[ADC_BUFFER_SIZE];
uint8_t  adc_immediate = 1;

uint32_t xlim_us = 200;
uint32_t ylim_uV = 1000000;

uint16_t cursor0 = 120;
uint16_t cursor1 = 196;

uint16_t trigger0 = 70;
uint16_t trigger1 = 146;

uint8_t  trigger_mode   = 0;
uint16_t trigger0_value = 2048;
uint16_t trigger1_value = 2048;

uint8_t event_adc      = 0;
uint8_t event_axis     = 1;
uint8_t event_mode     = 1;
uint8_t event_cursor   = 1;
uint8_t event_trigger  = 1;
uint8_t event_channel  = 1;
uint8_t event_seconds  = 1;
uint8_t event_voltage  = 1;
uint8_t event_button0  = 0;
uint8_t event_button1  = 0;
uint8_t event_button2  = 0;
uint8_t event_selector = 1;
uint8_t event_trigger_mode = 1;
uint8_t event_trigger0_detected = 1;
uint8_t event_trigger1_detected = 1;
static void drawAxis(ILI9341TypeDef *display);
static void clearCursor(ILI9341TypeDef *display, uint16_t pos);
static void clearTrigger(ILI9341TypeDef *display, uint16_t pos);
static void drawCursor(ILI9341TypeDef *display, uint16_t pos, char *name, uint16_t color);
static void drawTrigger(ILI9341TypeDef *display, uint16_t pos, char *name, uint16_t color);
static void drawSignal(ILI9341TypeDef *display, uint32_t *adc_time, uint16_t *adc0, uint32_t adc_length, uint16_t pixel_dirty[280][2], uint16_t cursor, uint16_t color);
static void clearSignal(ILI9341TypeDef *display, uint16_t pixel_dirty[280][2]);
static void drawSignalParam(ILI9341TypeDef *display, char *string, size_t size, uint16_t adc_max, uint16_t adc_min, uint32_t adc_period);
volatile uint32_t adc0_time_delta = 0;
volatile uint32_t adc1_time_delta = 0;
volatile uint16_t adc0_prev       = 0;
volatile uint16_t adc1_prev       = 0;




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


static void drawAxis(ILI9341TypeDef *display)
{
	for (uint16_t i = 0; i < 9; i++) {
		uint16_t y = 20 + 25 * i;

		if (i == 0 || i == 8) {
			ILI9341_FillRectangle(display, 20, y, 276, 1, ILI9341_WHITE);
			continue;
		}

		for (uint16_t j = 0; j < 276; j += 2)
			ILI9341_DrawPixel(display, 20 + j, y, ILI9341_GRAY);
	}

	for (uint16_t i = 0; i < 12; i++) {
		uint16_t x = 20 + 25 * i;

		if (i == 0 || i == 11) {
			ILI9341_FillRectangle(display, x, 20, 1, 200, ILI9341_WHITE);
			continue;
		}

		for (uint16_t j = 0; j < 200; j += 2)
			ILI9341_DrawPixel(display, x, 20 + j, ILI9341_GRAY);
	}
}

static void clearCursor(ILI9341TypeDef *display, uint16_t pos)
{
	ILI9341_FillRectangle(display, 0, pos - 6, 20, 11, ILI9341_BLACK);
}

static void clearTrigger(ILI9341TypeDef *display, uint16_t pos)
{
	ILI9341_FillRectangle(display, 296, pos - 6, 20, 11, ILI9341_BLACK);
}

static void drawCursor(ILI9341TypeDef *display, uint16_t pos, char *name, uint16_t color)
{
	ILI9341_FillRectangle(display, 0, pos - 6, 7 * 2, 1, color);
	ILI9341_WriteString(display, 0, pos - 5, name, Font_7x10, ILI9341_BLACK, color);

	for (uint8_t i = 0; i < 6; i++) {
		for (uint8_t j = i; j < 11 - i; j++)
			ILI9341_DrawPixel(display, 14 + i, pos - 6 + j, color);
	}
}

static void drawTrigger(ILI9341TypeDef *display, uint16_t pos, char *name, uint16_t color)
{
	ILI9341_FillRectangle(display, 302, pos - 6, 7 * 2, 1, color);
	ILI9341_WriteString(display, 302, pos - 5, name, Font_7x10, ILI9341_BLACK, color);

	for (uint8_t i = 0; i < 6; i++) {
		for (uint8_t j = i; j < 11 - i; j++)
			ILI9341_DrawPixel(display, 301 - i, pos - 6 + j, color);
	}
}

static void drawSignal(ILI9341TypeDef *display, uint32_t *adc_time, uint16_t *adc0, uint32_t adc_length, uint16_t pixel_dirty[280][2], uint16_t cursor, uint16_t color)
{
	uint16_t point[280];
	for (uint16_t i = 0; i < 280; i++)
		point[i] = 0;

	for (uint16_t i = 0; i < adc_length; i++) {

		float uV = (float)(adc0[i]) * 3300000.0f / 4096.0f;
		uint16_t x = (float)(adc_time[i]) * 280.0f / (float)(12.0f * xlim_us);
		uint16_t y = cursor - ((uV / (float)(ylim_uV)) * 200.0f / 8.0f);

		if (x < 0)
			x = 0;

		if (x > 274)
			x = 274;

		if (y < 21)
			y = 21;

		if (y > 219)
			y = 219;

		point[x] += (float)(y - point[x]) * 1.0f;
	}

	uint16_t pixel[280][2];
	for (uint16_t i = 0; i < 280; i++) {
		pixel[i][0] = 220;
		pixel[i][1] = 20;
	}

	for (uint16_t i = 1; i <= 279; i++) {

		if (point[i] == 0)
			continue;

		int16_t x1 = i;
		int16_t x0 = x1 - 1;

		for (; x0 >= 0; x0--) {
			if (point[x0] != 0)
				break;
		}

		if (x0 == 0 && point[x0] == 0)
			return;

		int16_t y0 = point[x0];
		int16_t y1 = point[x1];

		int16_t dx = (x1 - x0) > 0 ? (x1 - x0) : -(x1 - x0);
		int16_t sx = x0 < x1 ? 1 : -1;
		int16_t dy = (y1 - y0) > 0 ? -(y1 - y0) : (y1 - y0);
		int16_t sy = y0 < y1 ? 1 : -1;
		int16_t error = dx + dy;

		while (1) {

			if (pixel[x0][0] > y0)
				pixel[x0][0] = y0;

			if (pixel[x0][1] < y0)
				pixel[x0][1] = y0;

			if (x0 == x1 && y0 == y1)
				break;

			int16_t e2 = 2 * error;

			if (e2 >= dy) {
				if (x0 == x1)
					break;

				error = error + dy;
				x0 = x0 + sx;
			}

			if (e2 <= dx) {
				if (y0 == y1)
					break;

				error = error + dx;
				y0 = y0 + sy;
			}
		}
	}

	for (uint16_t i = 1; i <= 279; i++) {
		uint16_t min = pixel[i][0] < pixel_dirty[i][0] ? pixel[i][0] : pixel_dirty[i][0];
		uint16_t max = pixel[i][1] > pixel_dirty[i][1] ? pixel[i][1] : pixel_dirty[i][1];

		for (uint16_t j = min; j <= max; j++) {
			uint8_t draw = 0;
			if (j >= pixel[i][0] && j <= pixel[i][1])
				draw = 1;

			uint8_t clear = 0;
			if (j >= pixel_dirty[i][0] && j <= pixel_dirty[i][1])
				clear = 1;

			if (draw && !clear && j > 21)
				ILI9341_DrawPixel(display, i + 20, j, color);

			if (!draw && clear) {
				if (((i % 25) == 0 && (j % 2) == 0) || ((i % 2) == 0 && ((j - 20) % 25) == 0))
					ILI9341_DrawPixel(display, i + 20, j,  ILI9341_GRAY);
				else
					ILI9341_DrawPixel(display, i + 20, j, ILI9341_BLACK);
			}
		}

		pixel_dirty[i][0] = pixel[i][0];
		pixel_dirty[i][1] = pixel[i][1];
	}
}

static void clearSignal(ILI9341TypeDef *display, uint16_t pixel_dirty[280][2])
{
	for (uint16_t i = 1; i <= 279; i++) {
		for (uint16_t j = pixel_dirty[i][0]; j <= pixel_dirty[i][1]; j++) {
			if (((i % 25) == 0 && (j % 2) == 0) || ((i % 2) == 0 && ((j - 20) % 25) == 0))
				ILI9341_DrawPixel(display, i + 20, j,  ILI9341_GRAY);
			else
				ILI9341_DrawPixel(display, i + 20, j, ILI9341_BLACK);
		}

		pixel_dirty[i][0] = 0;
		pixel_dirty[i][1] = 0;
	}
}

static void drawSignalParam(ILI9341TypeDef *display, char *string, size_t size, uint16_t adc_max, uint16_t adc_min, uint32_t adc_period)
{
	float max  = (float)(adc_max) * 3300000.0f / 4096.0f;
	float min  = (float)(adc_min) * 3300000.0f / 4096.0f;
	float freq = 1.0f / ((float)(adc_period) / 1000000.0f);
	char *max_postfix  = "";
	char *min_postfix  = "";
	char *freq_postfix = "";

	if (max >= 1000000.0f) {
		max /= 1000000.0f;
		max_postfix = " V";
	} else if (max >= 1000.0f) {
		max /= 1000.0f;
		max_postfix = "mV";
	} else
		max_postfix = "uV";

	if (min >= 1000000.0f) {
		min /= 1000000.0f;
		min_postfix = " V";
	} else if (min >= 1000.0f) {
		min /= 1000.0f;
		min_postfix = "mV";
	} else
		min_postfix = "uV";

	if (freq >= 1000000.0f) {
		freq /= 1000000.0f;
		freq_postfix = "MHz";
	} else if (freq >= 1000.0f) {
		freq /= 1000.0f;
		freq_postfix = "kHz";
	} else
		freq_postfix = " Hz";

	if (adc_period != 0)
		snprintf(string, size, "ampl:%3.1f%s~%3.1f%s freq:%3.0f%s  ", min, min_postfix, max, max_postfix, freq, freq_postfix);
	else
		snprintf(string, size, "ampl:%3.1f%s~%3.1f%s freq:??? Hz  ", min, min_postfix, max, max_postfix);

	ILI9341_WriteString(display, 98, 225, string, Font_7x10, ILI9341_WHITE, ILI9341_BLACK);
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
  // 直接使用 HAL_UART_Transmit 測試
  //const char *test_msg = "UART Test\r\n";
  //HAL_UART_Transmit(&huart6, (uint8_t*)test_msg, strlen(test_msg), 100);

  // 啟動DMA接收
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

  // Display init
  display.spi             = &hspi5;
  display.cs_gpio_port    = ILI9341_CS_GPIO_Port;
  display.dc_gpio_port    = ILI9341_DC_GPIO_Port;
  display.reset_gpio_port = ILI9341_RESET_GPIO_Port;
  display.cs_pin          = ILI9341_CS_Pin;
  display.dc_pin          = ILI9341_DC_Pin;
  display.reset_pin       = ILI9341_RESET_Pin;
  display.width           = 320;
  display.height          = 240;
  display.orientation     = ILI9341_ORIENTATION_ROTATE_RIGHT;

  ILI9341_UNSELECT(&display);
  ILI9341_Init(&display);

  // Local vars
  char string[255];

  uint8_t  menu_extended         = 0;
  uint8_t  menu_channel0_enabled = 1;
  uint8_t  menu_channel1_enabled = 1;//0;
  int8_t   menu_selected_item    = 2;
  uint16_t menu_selector_x       = 96;
  uint16_t menu_selector_y       = 3;

  uint8_t  mode = 0;
  uint8_t  mode_seconds = 5;
  uint8_t  mode_voltage = 6;

  uint16_t encoder0_prev = 0;
  uint16_t encoder1_prev = 0;

  uint8_t  frames = 0;
  uint32_t frames_ticks = HAL_GetTick();

  uint16_t pixel_dirty0[280][2];
  uint16_t pixel_dirty1[280][2];
  for (uint16_t i = 0; i < 280; i++) {
	  pixel_dirty0[i][0] = 0;
	  pixel_dirty0[i][1] = 0;
	  pixel_dirty1[i][0] = 0;
	  pixel_dirty1[i][1] = 0;
  }

  // Dispaly freq. (for debug)
  ILI9341_FillScreen(&display, ILI9341_BLACK);

  snprintf(string, 255, "Oscilloscope");
  ILI9341_WriteString(&display, 0, 18 * 0, string, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

  snprintf(string, 255, "SYCLK = %ldMHz", HAL_RCC_GetSysClockFreq()/1000000);
  ILI9341_WriteString(&display, 0, 18 * 1, string, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

  snprintf(string, 255, "HCLK  = %ldMHz", HAL_RCC_GetHCLKFreq()/1000000);
  ILI9341_WriteString(&display, 0, 18 * 2, string, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

  snprintf(string, 255, "APB1  = %ldMHz", HAL_RCC_GetPCLK1Freq()/1000000);
  ILI9341_WriteString(&display, 0, 18 * 3, string, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

  snprintf(string, 255, "APB2  = %ldMHz", HAL_RCC_GetPCLK2Freq()/1000000);
  ILI9341_WriteString(&display, 0, 18 * 4, string, Font_11x18, ILI9341_BLACK, ILI9341_WHITE);

  HAL_Delay(1000);
  ILI9341_FillScreen(&display, ILI9341_BLACK);

  adc_reset_cyccnt = 1;
  if (adc_immediate) {
	  // The ADC starts immediately after the previous measurement is handled
	  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_data, 2);
  } else {
	  // ADC starts by timer
	  HAL_TIM_Base_Start_IT(&htim10);
  }

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
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

		// Draw axis
		if (event_axis) {
			drawAxis(&display);
			event_axis = 0;
		}

		uint8_t local_event_adc = 0;
		if (event_adc) {
			local_event_adc = 1;
			event_adc = 0;
		}

		// Draw signals
		if (local_event_adc) {

			if (menu_channel0_enabled)
				drawSignal(&display, adc0_time, adc0, adc0_length, pixel_dirty0, cursor0, ILI9341_YELLOW);

			if (menu_channel1_enabled)
				drawSignal(&display, adc1_time, adc1, adc1_length, pixel_dirty1, cursor1, ILI9341_CYAN);

		}

		// Draw FPS
		if (frames > 60) {
			snprintf(string, 255, "FPS: %5i", (int)(1000.0f / ((float)(HAL_GetTick() - frames_ticks) / 60.0f)));
			ILI9341_WriteString(&display, 225, 5, string, Font_7x10, ILI9341_WHITE, ILI9341_BLACK);

			frames = 0;
			frames_ticks = HAL_GetTick();
		}

		// Handle button events
		if (event_button0) {

			if (!menu_extended) {
				ILI9341_Rectangle(&display, menu_selector_x, menu_selector_y, 38, 13, ILI9341_BLACK);

				menu_selected_item++;

				if (menu_selected_item > 3)
					menu_selected_item = 2;

				menu_selector_x = 20 + 38 * menu_selected_item;
				menu_selector_y = 3;

				event_selector = 1;
			}

			event_button0 = 0;
		}

		if (event_button1) {
			menu_extended = !menu_extended;

			if (!menu_extended) {
				ILI9341_Rectangle(&display, menu_selector_x, menu_selector_y, 38, 13, ILI9341_BLACK);

				menu_selected_item = 2;

				menu_selector_x = 20 + 38 * menu_selected_item;
				menu_selector_y = 3;

				mode = 0;
				event_selector = 1;
			} else
				mode = 1;

			event_mode = 1;
			event_button1 = 0;
		}

		if (event_button2) {
			if (menu_selected_item == 2) {
				menu_channel0_enabled = !menu_channel0_enabled;

				if (!menu_channel0_enabled)
					clearSignal(&display, pixel_dirty0);
			}

			if (menu_selected_item == 3) {
				menu_channel1_enabled = !menu_channel1_enabled;

				if (!menu_channel1_enabled)
					clearSignal(&display, pixel_dirty1);
			}

			event_cursor  = 1;
			event_trigger = 1;
			event_channel = 1;
			event_button2 = 0;
		}

		// Handle UI redraw channel events
		if (event_channel) {
			uint16_t channel0_color = menu_channel0_enabled ? ILI9341_YELLOW  : ILI9341_COLOR565(60, 60, 0);
			uint16_t channel1_color = menu_channel1_enabled ? ILI9341_CYAN    : ILI9341_COLOR565(0, 60, 60);

			ILI9341_WriteString(&display, 110 - 12, 5, " CH1 ", Font_7x10, ILI9341_BLACK, channel0_color);
			ILI9341_WriteString(&display, 148 - 12, 5, " CH2 ", Font_7x10, ILI9341_BLACK, channel1_color);

			event_channel = 0;
		}

		// Handle encoder0
		int32_t encoder0_curr = __HAL_TIM_GET_COUNTER(&htim3);
		encoder0_curr = 32767 - ((encoder0_curr - 1) & 0xFFFF) / 2;

		if(encoder0_curr != encoder0_prev) {
			int32_t delta = encoder0_curr - encoder0_prev;

			if (delta > 10)
				delta = -1;

			if (delta < -10)
				delta = 1;

			if (menu_extended) {
				ILI9341_Rectangle(&display, menu_selector_x, menu_selector_y, 38, 13, ILI9341_BLACK);

				menu_selected_item += delta;

				if (menu_selected_item < 0)
					menu_selected_item = 0;

				if (menu_selected_item > 5)
					menu_selected_item = 5;

				if (menu_selected_item < 4) {
					menu_selector_x = 20 + 38 * menu_selected_item;
					menu_selector_y = 3;
				} else {
					menu_selector_x = 20 + 38 * (menu_selected_item - 4);
					menu_selector_y = 223;
				}

				event_selector = 1;

			} else {

				if (menu_selected_item == 2) {
					clearCursor(&display, cursor0);
					cursor0 += delta;

					if (cursor0 < 20)
						cursor0 = 20;

					if (cursor0 > 220)
						cursor0 = 220;
				}

				if (menu_selected_item == 3) {
					clearCursor(&display, cursor1);
					cursor1 += delta;

					if (cursor1 < 20)
						cursor1 = 20;

					if (cursor1 > 220)
						cursor1 = 220;
				}

			}

			event_cursor = 1;
			event_trigger = 1;
			encoder0_prev = encoder0_curr;
		}

		// Handle UI redraw cursor events
		if (event_cursor) {
			clearCursor(&display, cursor0);
			clearCursor(&display, cursor1);

			if (menu_channel0_enabled)
				drawCursor(&display, cursor0, "C1", ILI9341_YELLOW);

			if (menu_channel1_enabled)
				drawCursor(&display, cursor1, "C2", ILI9341_CYAN);

			event_cursor = 0;
		}

		// Handle encoder1
		int32_t encoder1_curr = __HAL_TIM_GET_COUNTER(&htim4);
		encoder1_curr = 32767 - ((encoder1_curr - 1) & 0xFFFF) / 2;

		if(encoder1_curr != encoder1_prev || event_trigger) {
			int32_t delta = encoder1_curr - encoder1_prev;

			if (delta > 10)
				delta = -1;

			if (delta < -10)
				delta = 1;

			if (menu_extended) {

				if (menu_selected_item == 1) {
					trigger_mode += delta;

					if (trigger_mode < 0)
						trigger_mode = 0;

					if (trigger_mode > 1)
						trigger_mode = 1;

					event_trigger_mode = 1;
				}

				if (menu_selected_item == 4) {
					mode_seconds += delta;

					if (mode_seconds < 0)
						mode_seconds = 0;

					if (mode_seconds > 16)
						mode_seconds = 16;

					uint32_t list_seconds[17] = {
						5,
						10,
						20,
						50,
						100,
						200,
						500,
						1000,
						2000,
						5000,
						10000,
						20000,
						50000,
						100000,
						200000,
						500000,
						1000000
					};

					xlim_us = list_seconds[mode_seconds];

					if (adc_immediate) {
						HAL_TIM_Base_Stop_IT(&htim10);
						adc_available = 1;
					}

					HAL_ADC_Stop_DMA(&hadc1);

					adc_reset_cyccnt = 1;
					adc0_length = 0;
					adc1_length = 0;
					adc_max[0] = 0;
					adc_max[1] = 0;
					adc_min[0] = -1;
					adc_min[1] = -1;
					adc_period[0] = 0;
					adc_period[1] = 0;
					adc_period0_detected = 0;
					adc_period1_detected = 0;
					event_trigger0_detected = 0;
					event_trigger1_detected = 0;

					if (menu_channel0_enabled)
						adc0_filled = 0;
					else
						adc0_filled = 1;

					if (menu_channel1_enabled)
						adc1_filled = 0;
					else
						adc1_filled = 1;

					adc_immediate = xlim_us <= 500;

					if (!adc_immediate) {
						uint32_t list_timer_settings[17][2] = {
						//  { Prescaler, Period }
							{     0,   0 },
							{     0,   0 },
							{     0,   0 },
							{     0,   0 },
							{     0,   0 },
							{     0,   0 },
							{     0,   0 },
							{    40, 100 },
							{    81, 100 },
							{   205, 100 },
							{   410, 100 },
							{   822, 100 },
							{  2056, 100 },
							{  4113, 100 },
							{  8228, 100 },
							{ 20570, 100 },
							{ 41142, 100 }
						};

						htim10.Init.Prescaler = list_timer_settings[mode_seconds][0];
						htim10.Init.Period    = list_timer_settings[mode_seconds][1];

						if (HAL_TIM_Base_Init(&htim10) != HAL_OK)
							Error_Handler();

						HAL_TIM_Base_Start_IT(&htim10);
					} else
						HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_data, 2);

					local_event_adc = 0;
					event_seconds = 1;
				}

				if (menu_selected_item == 5) {
					mode_voltage += delta;

					if (mode_voltage < 0)
						mode_voltage = 0;

					if (mode_voltage > 9)
						mode_voltage = 9;

					uint32_t list_voltage[10] = {
						10000,
						20000,
						50000,
						100000,
						200000,
						500000,
						1000000,
						2000000,
						5000000,
						10000000
					};

					ylim_uV = list_voltage[mode_voltage];

					event_voltage = 1;
				}

			} else {

				if (menu_selected_item == 2) {
					clearTrigger(&display, trigger0);
					trigger0 += delta;

					if (trigger0 < 20)
						trigger0 = 20;

					if (trigger0 > 220)
						trigger0 = 220;

					if (trigger0 > cursor0)
						trigger0 = cursor0;

					float uV = -((float)(ylim_uV) * ((8.0f * (trigger0 - cursor0)) / 200.0f));
					trigger0_value = uV * 4096.0f / 3300000.0f;
				}

				if (menu_selected_item == 3) {
					clearTrigger(&display, trigger1);
					trigger1 += delta;

					if (trigger1 < 20)
						trigger1 = 20;

					if (trigger1 > 220)
						trigger1 = 220;

					if (trigger1 > cursor1)
						trigger1 = cursor1;

					float uV = -((float)(ylim_uV) * ((8.0f * (trigger1 - cursor1)) / 200.0f));
					trigger1_value = uV * 4096.0f / 3300000.0f;
				}

			}

			event_trigger = 1;
			encoder1_prev = encoder1_curr;
		}

		// Handle UI redraw trigger events
		if (event_trigger) {
			clearTrigger(&display, trigger0);
			clearTrigger(&display, trigger1);

			if (menu_channel0_enabled)
				drawTrigger(&display, trigger0, "T1", ILI9341_YELLOW);

			if (menu_channel1_enabled)
				drawTrigger(&display, trigger1, "T2", ILI9341_CYAN);

			event_trigger = 0;
		}

		// Handle UI redraw selector events
		if (event_selector) {
			ILI9341_Rectangle(&display, menu_selector_x, menu_selector_y, 38, 13, ILI9341_WHITE);
			event_selector = 0;
		}

		// Handle UI redraw mode events
		if (event_mode) {
			if (mode == 0)
				ILI9341_WriteString(&display, 22, 5, " RUN ", Font_7x10, ILI9341_BLACK, ILI9341_GREEN);

			if (mode == 1)
				ILI9341_WriteString(&display, 22, 5, "MENU:", Font_7x10, ILI9341_BLACK, ILI9341_BLUE);

			if (mode == 2)
				ILI9341_WriteString(&display, 22, 5, "HOLD:", Font_7x10, ILI9341_BLACK, ILI9341_YELLOW);

			event_mode = 0;
		}

		// Handle UI redraw trigger mode events
		if (event_trigger_mode) {
			ILI9341_FillRectangle(&display, 61,  5, 33, 10, ILI9341_BLACK);
			ILI9341_FillRectangle(&display, 61, 14, 11, 1, ILI9341_WHITE);
			ILI9341_FillRectangle(&display, 72,  5, 11, 1, ILI9341_WHITE);
			ILI9341_FillRectangle(&display, 83, 14, 11, 1, ILI9341_WHITE);
			ILI9341_FillRectangle(&display, 72,  5, 1, 10, ILI9341_WHITE);
			ILI9341_FillRectangle(&display, 82,  5, 1, 10, ILI9341_WHITE);

			if (trigger_mode == 0) {
				for (uint8_t i = 0; i < 4; i++) {
					for (uint8_t j = i; j < (7 - i); j++)
						ILI9341_DrawPixel(&display, 69 + j, 11 - i, ILI9341_GREEN);
				}
			} else {
				for (uint8_t i = 0; i < 4; i++) {
				  for (uint8_t j = i; j < (7 - i); j++)
					  ILI9341_DrawPixel(&display, 79 + j, 8 + i, ILI9341_RED);
				}
			}

			event_trigger_mode = 0;
		}

		// Handle UI redraw seconds events
		if (event_seconds) {
			if (xlim_us >= 1000000)
				snprintf(string, 255, "%3li s",  xlim_us / 1000000);
			else if (xlim_us >= 1000)
				snprintf(string, 255, "%3lims", xlim_us / 1000);
			else
				snprintf(string, 255, "%3lius", xlim_us);

			ILI9341_WriteString(&display, 22, 225, string, Font_7x10, ILI9341_BLACK, ILI9341_WHITE);

			event_seconds = 0;
		}

		// Handle UI redraw voltage events
		if (event_voltage) {
			if (ylim_uV >= 1000000)
				snprintf(string, 255, "%3li V",  ylim_uV / 1000000);
			else if (ylim_uV >= 1000)
				snprintf(string, 255, "%3limV", ylim_uV / 1000);
			else
				snprintf(string, 255, "%3liuV", ylim_uV);

			ILI9341_WriteString(&display, 60, 225, string, Font_7x10, ILI9341_BLACK, ILI9341_WHITE);

			event_voltage = 0;
		}

		snprintf(string, 255, "T:%i%i", event_trigger0_detected, event_trigger1_detected);
		ILI9341_WriteString(&display, 195, 5, string, Font_7x10, ILI9341_WHITE, ILI9341_BLACK);

		if (menu_channel1_enabled && menu_selected_item == 3) {
			//if (adc_period1_detected)
				drawSignalParam(&display, string, 255, adc_max[1], adc_min[1], adc_period[1]);
		} else if (menu_channel0_enabled) {
			//if (adc_period0_detected)
				drawSignalParam(&display, string, 255, adc_max[0], adc_min[0], adc_period[0]);
		}

		// Restart ADC ...
		if (local_event_adc) {
			adc_reset_cyccnt = 1;
			adc0_length = 0;
			adc1_length = 0;
			adc_max[0] = 0;
			adc_max[1] = 0;
			adc_min[0] = -1;
			adc_min[1] = -1;
			adc_period[0] = 0;
			adc_period[1] = 0;
			adc_period0_detected = 0;
			adc_period1_detected = 0;
			event_trigger0_detected = 0;
			event_trigger1_detected = 0;

			if (menu_channel0_enabled)
				adc0_filled = 0;
			else
				adc0_filled = 1;

			if (menu_channel1_enabled)
				adc1_filled = 0;
			else
				adc1_filled = 1;

			if (adc_immediate) {
				// The ADC starts immediately after the previous measurement is handled
				HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_data, 2);
			} else {
				// ADC starts by timer
				HAL_TIM_Base_Start_IT(&htim10);
			}

			local_event_adc = 0;
		}

		frames++;


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
  __disable_irq();
  while (1)
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
