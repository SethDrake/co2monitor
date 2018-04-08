
#include <algorithm>
#include <cstring>
#include <stm32f1xx_hal.h>
#include "src/drivers/inc/co2sensor.h"
#include "src/delay.h"
#include "stm32f1xx_hal_uart.h"

/* MH-Z19 CO2 SENSOR */

CO2Sensor::CO2Sensor() {
	maxLevel = 0;
	maxLevelToday = 0;
	sumForInterval = 0;
	errorsCount = 0;
	elapsedSeconds = 0;
	elapsedMinutes = 0;
	elapsedHours = 0;
	uptime = 0;
	std::fill_n(counts, 17, 0);
	std::fill_n(temp, 17, 0);
	std::fill_n(co2PerMinutes, 62, 0);
	std::fill_n(co2PerHours, 26, 0);
}

CO2Sensor::~CO2Sensor() {
}

bool CO2Sensor::initSensor(UART_HandleTypeDef* usart)
{
	this->usart = usart;
	//return true;

	uint8_t cmd[64] = { };
	resetSensor();
	DelayManager::DelayMs(3000);
	setRange(5000);
	DelayManager::DelayMs(9000);
	USART_ReadBlock(cmd, 64, 1000);
	cmd[0] = 0xff;
	char* str = search_buffer((char*)cmd, 64, "KB200", 5);
	if (!str)
	{
		return false;		
	}
	//setAutoCalibration(false);
	return true;
}

char* CO2Sensor::search_buffer(char* haystack, uint16_t haystacklen, char* needle, uint16_t needlelen)
{ 
	int searchlen = haystacklen - needlelen + 1;
	for (; searchlen-- > 0; haystack++)
		if (!memcmp(haystack, needle, needlelen))
			return haystack;
	return NULL;
}

void CO2Sensor::setAutoCalibration(bool enabled)
{
	uint8_t cmd[9] = { 0xFF, 0x01, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	cmd[3] = enabled ? 0xA0 : 0x00;
	cmd[8] = calculateChecksum(cmd);
	USART_SendBlock(cmd, 9);
	DelayManager::DelayUs(250);
}

void CO2Sensor::setRange(uint16_t range) //auto restart after execute command
{
	uint8_t cmd[9] = { 0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	cmd[6] = range / 256;
	cmd[7] = range % 256;
	cmd[8] = calculateChecksum(cmd);
	USART_SendBlock(cmd, 9);
}

void CO2Sensor::resetSensor(void)
{
	uint8_t cmd[9] = { 0xFF, 0x01, 0x8d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	cmd[8] = calculateChecksum(cmd);
	USART_SendBlock(cmd, 9);
}


void CO2Sensor::USART_SendBlock(uint8_t* data, uint8_t size) 
{
	HAL_UART_Transmit(usart, data, size, 5000);
}

uint8_t CO2Sensor::USART_ReadByte(bool* isOk, uint16_t timeout) 
{
	volatile uint32_t nCount;
	
	*isOk = true;
	nCount = ((uint64_t)HAL_RCC_GetHCLKFreq() * timeout / 10000);
	if (!(usart->gState == HAL_UART_STATE_READY) && !(usart->gState == HAL_UART_STATE_BUSY_TX))
	{
		*isOk = false;
		return 0x00;
	}
	if (usart->Lock == HAL_LOCKED)
	{
		*isOk = false;
		return 0x00;
	}
	__HAL_LOCK(usart);
	usart->ErrorCode = HAL_UART_ERROR_NONE;
	if (usart->gState == HAL_UART_STATE_BUSY_TX)
	{
		usart->gState = HAL_UART_STATE_BUSY_TX_RX;
	}
	else
	{
		usart->gState = HAL_UART_STATE_BUSY_RX;
	}
	while (!(usart->Instance->SR & USART_SR_RXNE))
	{
		nCount--;
		if (nCount == 0) //break if await longer timeout in ms
		{
			*isOk = false;
			break;
		}
	}
	uint8_t result = usart->Instance->DR;
	if (usart->gState == HAL_UART_STATE_BUSY_TX_RX) 
	{
		usart->gState = HAL_UART_STATE_BUSY_TX;
	}
	else
	{
		usart->gState = HAL_UART_STATE_READY;
	}
	__HAL_UNLOCK(usart);
	return result;
}

bool CO2Sensor::USART_ReadBlock(uint8_t* data, uint8_t size, uint16_t timeout) 
{
	bool isOk = true;
	for (int i = 0; i < size; i++)
	{
		data[i] = USART_ReadByte(&isOk, timeout);
		if (!isOk)
		{
			return false;
		}
	}
	return true;
}

bool CO2Sensor::USART_ReadFixedBlock(uint8_t* data, uint8_t size, uint16_t timeout) 
{
	return HAL_UART_Receive(usart, data, size, timeout) == HAL_OK;
}

uint8_t CO2Sensor::calculateChecksum(uint8_t* command)
{
	uint8_t checksum = 0;
	for (int i = 1; i < 8; i++)
	{
		checksum += command[i];
	}
	checksum = checksum ^ 0xFF;
	checksum += 1;
	return checksum;
}


void CO2Sensor::measureCO2Level()
{
	uint8_t cmd[9] = { 0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79 };
	
	if (!usart)
	{
		return;
	}

	USART_SendBlock(cmd, 9);
	if (!USART_ReadFixedBlock(cmd, 9, MHZ19_TIMEOUT))
	{
		counts[0] = 0;
		temp[0] = 0;
		errorsCount++;
		return;
	}
	
	uint8_t checksum = calculateChecksum(cmd);
	uint8_t checksumFromSensor = cmd[8];
	if (checksum != checksumFromSensor) { //checksum incorrect
		counts[0] = 0;
		temp[0] = 0;
		errorsCount++;
		return;
	}
	counts[0] = (cmd[2] << 8) + cmd[3];
	temp[0] = cmd[4] - 40;
	if (counts[0] > 34000) // invalid value
	{
		counts[0] = 0;
	}
}

uint16_t CO2Sensor::GetCurrentCO2Level(void) 
{
	return counts[1];
}


int8_t CO2Sensor::GetCurrentTemperature(void)
{
	return temp[1];
}

void CO2Sensor::TickSeconds(uint8_t interval)
{
	this->measureCO2Level();
	for (uint8_t i = 6; i > 0; i--)
	{
		sumForInterval += counts[i];
	}
	for (uint8_t i = 16; i > 0; i--)
	{
		counts[i] = counts[i - 1];
		temp[i] = temp[i - 1];
	}
	counts[0] = 0;
	temp[0] = 0;

	elapsedSeconds += interval;
	uptime += interval;

	if (elapsedSeconds >= 60) // one minute elapsed
	{
		co2PerMinutes[0] = sumForInterval / 6;
		sumForInterval = 0;
		if (maxLevelToday < co2PerMinutes[0]) {
			maxLevelToday = co2PerMinutes[0];
		}
		if (maxLevel < maxLevelToday)
		{
			maxLevel = maxLevelToday;
		}

		for (uint8_t i = 61; i > 0; i--)
		{
			co2PerMinutes[i] = co2PerMinutes[i - 1];
		}
		co2PerMinutes[0] = 0;
		
		elapsedMinutes++;
		elapsedSeconds = 0;
	}
	
	if (elapsedMinutes >= 60) // one hour elapsed
	{
		for (uint8_t i = 0; i < 60; i++) {
			co2PerHours[0] += co2PerMinutes[i];
		}
		co2PerHours[0] = co2PerHours[0] / 60; // 60 поминутных выборок усредняем - средний уровнь за час

		for (uint8_t i = 25; i > 0; i--)
		{
			co2PerHours[i] = co2PerHours[i - 1];
		}
		co2PerHours[0] = 0;
		
		elapsedHours++;
		elapsedMinutes = 0;
	}
	
	if (elapsedHours % 24 == 0) // one day elapsed
	{
		maxLevelToday = 0;	
	}
}

uint16_t* CO2Sensor::GetPerSecondCo2()
{
	return this->counts;
}

uint16_t* CO2Sensor::GetPerMinuteCo2()
{
	return this->co2PerMinutes;
}

uint16_t* CO2Sensor::GetPerHourCo2()
{
	return this->co2PerHours;
}

uint16_t CO2Sensor::GetErrorsCount()
{
	return errorsCount;
}

uint16_t CO2Sensor::GetMaxCO2Level()
{
	return maxLevel;
}

uint16_t CO2Sensor::GetMaxPerHourCO2Level()
{
	return *max_element(co2PerMinutes, co2PerMinutes + 60);
}

uint16_t CO2Sensor::GetMaxPerDayCO2Level()
{
	return *max_element(co2PerHours, co2PerHours + 24);
}

uint32_t CO2Sensor::GetUptime()
{
	return uptime;
}

void CO2Sensor::ResetErrorsCount()
{
	errorsCount = 0;
}
