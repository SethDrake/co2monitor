#pragma once
#ifndef __OBJECTS_H_
#define __OBJECTS_H_
#include "drivers/inc/co2sensor.h"
#include "drivers/inc/ssd1306.h"


template<class ForwardIt>
	ForwardIt max_element(ForwardIt first, ForwardIt last)
	{
		if (first == last) {
			return last;
		}
		ForwardIt largest = first;
		++first;
		for (; first != last; ++first) {
			if (*largest < *first) {
				largest = first;
			}
		}
		return largest;
	}

typedef enum
{ 
	LOADING = 0,
	ACTIVE,
	SLEEP
} SYSTEM_MODE;

typedef enum
{ 
	SECONDS = 0,
	MINUTES,
	HOURS,
	STATISTICS
} GRPAH_MODE;

extern SYSTEM_MODE systemMode;
extern GRPAH_MODE graphMode;

extern CO2Sensor co2sensor;
extern SSD1306 display;

extern DMA_HandleTypeDef i2cDmaTx;
extern TIM_HandleTypeDef htim1;
extern PCD_HandleTypeDef hpcd_USB_FS;

#endif //__OBJECTS_H_