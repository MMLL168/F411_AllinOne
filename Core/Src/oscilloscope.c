/*
 * oscilloscope.c
 *
 *  Created on: Aug 9, 2025
 *      Author: digno
 */
#define _OSCILLOSCOPE_C

#include "oscilloscope.h"


extern SPI_HandleTypeDef hspi5;
extern ILI9341TypeDef display;
extern ADC_HandleTypeDef hadc1;
extern DMA_HandleTypeDef hdma_adc1;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim10;

void InitOscState(void)
{

	  frames_ticks = HAL_GetTick();

	  for (uint16_t i = 0; i < 280; i++) {
		  pixel_dirty0[i][0] = 0;
		  pixel_dirty0[i][1] = 0;
		  pixel_dirty1[i][0] = 0;
		  pixel_dirty1[i][1] = 0;
	  }

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

void Oscilloscope_Process(void)
{
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

	  char string[255];

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

}

