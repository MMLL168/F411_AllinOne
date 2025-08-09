/*
 * oscilloscope.h
 *
 *  Created on: Aug 9, 2025
 *      Author: digno
 */

#ifndef INC_OSCILLOSCOPE_H_
#define INC_OSCILLOSCOPE_H_

#include "main.h"
#include "ili9341.h"

#define ADC_BUFFER_SIZE 2048//512
#define ADC_CHANNEL0_SCALE 1//11
#define ADC_CHANNEL1_SCALE 1


#ifdef _OSCILLOSCOPE_C
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
uint32_t frames_ticks =0;//HAL_GetTick();

uint16_t pixel_dirty0[280][2];
uint16_t pixel_dirty1[280][2];

volatile uint32_t adc0_time_delta = 0;
volatile uint32_t adc1_time_delta = 0;
volatile uint16_t adc0_prev       = 0;
volatile uint16_t adc1_prev       = 0;

#else
extern volatile uint16_t adc_data[3];
extern volatile uint32_t adc0_length;
extern volatile uint32_t adc1_length;
extern volatile uint8_t  adc0_filled;
extern volatile uint8_t  adc1_filled;
extern volatile uint8_t  adc_available;
extern volatile uint8_t  adc_reset_cyccnt;

extern volatile uint16_t adc_max[2];
extern volatile uint16_t adc_min[2];
extern volatile uint32_t adc_period[2];
extern volatile uint8_t  adc_period0_detected;
extern volatile uint8_t  adc_period1_detected;

extern uint32_t adc0_time[ADC_BUFFER_SIZE];
extern uint32_t adc1_time[ADC_BUFFER_SIZE];
extern uint16_t adc0[ADC_BUFFER_SIZE];
extern uint16_t adc1[ADC_BUFFER_SIZE];
extern uint8_t  adc_immediate;

extern uint32_t xlim_us;
extern uint32_t ylim_uV;

extern uint16_t cursor0;
extern uint16_t cursor1;

extern uint16_t trigger0;
extern uint16_t trigger1;

extern uint8_t  trigger_mode;
extern uint16_t trigger0_value;
extern uint16_t trigger1_value;

extern uint8_t event_adc;
extern uint8_t event_axis;
extern uint8_t event_mode;
extern uint8_t event_cursor;
extern uint8_t event_trigger;
extern uint8_t event_channel;
extern uint8_t event_seconds;
extern uint8_t event_voltage;
extern uint8_t event_button0;
extern uint8_t event_button1;
extern uint8_t event_button2;
extern uint8_t event_selector;
extern uint8_t event_trigger_mode;
extern uint8_t event_trigger0_detected;
extern uint8_t event_trigger1_detected;

extern uint8_t  menu_extended;
extern uint8_t  menu_channel0_enabled;
extern uint8_t  menu_channel1_enabled;
extern int8_t   menu_selected_item;
extern uint16_t menu_selector_x;
extern uint16_t menu_selector_y;

extern uint8_t  mode;
extern uint8_t  mode_seconds;
extern uint8_t  mode_voltage;

extern uint16_t encoder0_prev;
extern uint16_t encoder1_prev;

extern uint8_t  frames;
extern uint32_t frames_ticks;//HAL_GetTick();

extern uint16_t pixel_dirty0[280][2];
extern uint16_t pixel_dirty1[280][2];

extern volatile uint32_t adc0_time_delta;
extern volatile uint32_t adc1_time_delta;
extern volatile uint16_t adc0_prev;
extern volatile uint16_t adc1_prev;
#endif// /* _OSCILLOSCOPE_C */



void InitOscState(void);
static void drawAxis(ILI9341TypeDef *display);
static void clearCursor(ILI9341TypeDef *display, uint16_t pos);
static void clearTrigger(ILI9341TypeDef *display, uint16_t pos);
static void drawCursor(ILI9341TypeDef *display, uint16_t pos, char *name, uint16_t color);
static void drawTrigger(ILI9341TypeDef *display, uint16_t pos, char *name, uint16_t color);
static void drawSignal(ILI9341TypeDef *display, uint32_t *adc_time, uint16_t *adc0, uint32_t adc_length, uint16_t pixel_dirty[280][2], uint16_t cursor, uint16_t color);
static void clearSignal(ILI9341TypeDef *display, uint16_t pixel_dirty[280][2]);
static void drawSignalParam(ILI9341TypeDef *display, char *string, size_t size, uint16_t adc_max, uint16_t adc_min, uint32_t adc_period);
void Oscilloscope_Process(void);

#endif /* INC_OSCILLOSCOPE_H_ */
