
#pragma once
#ifndef __CO2_SENSOR_H_
#define __CO2_SENSOR_H_

#include <sys/_stdint.h>
#include <stm32f1xx_hal.h>

#define MHZ19_TIMEOUT 500

class CO2Sensor {
public:
	CO2Sensor();
	~CO2Sensor();
	bool initSensor(UART_HandleTypeDef* usart);
	uint16_t GetMaxCO2Level(void);
	uint16_t GetMaxPerHourCO2Level(void);
	uint16_t GetMaxPerDayCO2Level(void);
	uint16_t GetCurrentCO2Level(void);
	int8_t GetCurrentTemperature(void);
	uint16_t GetErrorsCount(void);
	void ResetErrorsCount(void);
	uint16_t* GetPerSecondCo2();
	uint16_t* GetPerMinuteCo2();
	uint16_t* GetPerHourCo2();
	uint32_t GetUptime();
	void TickSeconds(uint8_t interval);
	void resetSensor(void);
protected:
	void USART_SendBlock(uint8_t* data, uint8_t size);
	bool USART_ReadBlock(uint8_t* data, uint8_t size, uint16_t timeout);
	uint8_t USART_ReadByte(bool* isOk, uint16_t timeout);
	bool USART_ReadFixedBlock(uint8_t* data, uint8_t size, uint16_t timeout);
	uint8_t calculateChecksum(uint8_t* command);
	void measureCO2Level(void);
	void setAutoCalibration(bool enabled);
	void setRange(uint16_t range);
	char* search_buffer(char* haystack, uint16_t haystacklen, char* needle, uint16_t needlelen);
private:
	UART_HandleTypeDef* usart;
	uint16_t errorsCount;
	uint16_t counts[17];
	int8_t temp[17];
	uint16_t co2PerMinutes[62];
	uint16_t co2PerHours[26];
	uint16_t maxLevel;
	uint16_t maxLevelToday;
	uint64_t sumForInterval;
	uint8_t elapsedSeconds;
	uint8_t elapsedMinutes;
	uint32_t elapsedHours;
	uint32_t uptime;
};

#endif
