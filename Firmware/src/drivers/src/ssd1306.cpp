#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <sys/_stdint.h>
#include <string.h>
#include "src/drivers/inc/ssd1306.h"
#include "stm32f1xx_hal_i2c.h"

SSD1306::SSD1306()
{
	isSleep = true;
}

SSD1306::~SSD1306()
{
}


bool SSD1306::IsSleep()
{
	return this->isSleep;
}


bool SSD1306::IsActive()
{
	return this->isActive;
}

bool SSD1306::sendJustCommand(uint8_t cmd)
{
	return this->sendCommand(cmd, 0, 0);
}

bool SSD1306::sendCommandByte(uint8_t cmd, uint8_t value)
{
	return this->sendCommand(cmd, &value, 1);
}

bool SSD1306::sendCommand(uint8_t cmd, uint8_t* pBuffer, uint16_t NumByteToWrite)
{
	uint8_t buf[4];
	buf[0] = 0x00;
	buf[1] = cmd;
	for (uint8_t i = 0; i < NumByteToWrite; i++) 
	{
		buf[i + 2] = pBuffer[i];		
	}
	while (HAL_I2C_Master_Transmit(i2c, SSD1306_ADDR, buf, NumByteToWrite + 2, SSD1306_TIMEOUT) != HAL_OK)
	{
		if (HAL_I2C_GetError(i2c) != HAL_I2C_ERROR_AF)
		{
			return false;
		}
	}
	return true;
}

bool SSD1306::sendData(uint8_t* pBuffer, uint16_t NumByteToWrite)
{
	pBuffer[0] = 0x40;
	while (i2c->State == HAL_I2C_STATE_BUSY_TX); //wait for end of previous transfer
	if (i2c->hdmatx)
	{
		while (HAL_I2C_Master_Transmit_DMA(i2c, SSD1306_ADDR, pBuffer, NumByteToWrite) != HAL_OK)
		{
			if (HAL_I2C_GetError(i2c) != HAL_I2C_ERROR_AF)
			{
				return false;
			}
		}	
	}
	else
	{
		while (HAL_I2C_Master_Transmit(i2c, SSD1306_ADDR, pBuffer, NumByteToWrite, SSD1306_TIMEOUT) != HAL_OK)
		{
			if (HAL_I2C_GetError(i2c) != HAL_I2C_ERROR_AF)
			{
				return false;
			}
		}	
	}
	return true;
}


void SSD1306::sleepMode(bool enabled)
{
	sendCommand(enabled? SSD1306_Sleep : SSD1306_Wake , 0, 0);
	isSleep = enabled;
}

void SSD1306::setContrast(uint8_t contrast)
{
	sendCommandByte(SSD1306_SetContrast, contrast);
}

void SSD1306::initDisplay(I2C_HandleTypeDef* i2c)
{
	this->i2c = i2c;
    // Init sequence for 128x64 OLED module
	
	sleepMode(true);													// Display Off
	sendCommandByte(SSD1306_SetDisplayClockDivider, 0x80);				// Set Display Clk Div 0x80
	sendCommandByte(SSD1306_SetMultiplexRatio, SSD1306_HEIGHT - 1);		// Set Multiplex
	sendCommandByte(SSD1306_SetDisplayOffset, 0x00);					// Set Display Offset
	sendJustCommand(SSD1306_SetDisplayStartLine | 0x00);				// Set Display Start Line
	sendCommandByte(SSD1306_ChargePumpSetting, 0x14);					// Charge Pump 0x14
	sendCommandByte(SSD1306_SetMemAdressingMode, 0x00);					// Set Memory Addressing Mode - Horizontal
	sendJustCommand(SSD1306_SetSegmentRemap | 0x01);					// Set Segment Remap
	sendJustCommand(SSD1306_SetCOMoutScanDirection | 0x00);				// Scan direction
	sendCommandByte(SSD1306_SetCOMPinsConfig, 0x12);					// COM Pins 0x12
	setContrast(0xCF);
	sendCommandByte(SSD1306_SetPrechargePeriod, 0xF1);					// Precharge period
	sendCommandByte(SSD1306_SetVCOMHDeselectLevel, 0x40);				// VCOM Detect
	sendJustCommand(SSD1306_AllPixRAM);									// Draw from RAM - normal mode
	sendJustCommand(SSD1306_SetInverseOff);								// Disable inversion
	sendJustCommand(SSD1306_DeactivateScroll);							// Disable scroll						
	sleepMode(false);

	this->isActive = true;
}

void SSD1306::clearScreen(bool light) {
	memset(this->framebuffer, light ? 0xFF : 0x00, SSD1306_BUFFERSIZE + 1);
}

void SSD1306::drawFramebuffer() {
	uint8_t buf[2] = { 0, SSD1306_WIDTH - 1 };
	sendCommand(SSD1306_SetColumnAddr, buf, 2);
	buf[1] = (SSD1306_HEIGHT/8)-1;
	sendCommand(SSD1306_SetPageAddr, buf, 2);	
	sendData(framebuffer, SSD1306_BUFFERSIZE + 1);
}

void SSD1306::drawPixel(uint8_t x, uint8_t y, bool light) {
	if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
		return;
	}
	if (light)
	{
		this->framebuffer[1 + x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 7));
	}
	else
	{
		this->framebuffer[1 + x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
	}
}


void SSD1306::drawLine(uint8_t xstart, uint8_t ystart, uint8_t xstop, uint8_t ystop, bool light)
{
	if (xstart >= SSD1306_WIDTH || ystart >= SSD1306_HEIGHT) {
		return;
	}
	if (xstop >= SSD1306_WIDTH || ystop >= SSD1306_HEIGHT) {
		return;
	}
	const int16_t deltaX = std::abs(xstop - xstart);
	const int16_t deltaY = std::abs(ystop - ystart);
	const int16_t signX = xstart < xstop ? 1 : -1;
	const int16_t signY = ystart < ystop ? 1 : -1;

	int16_t error = deltaX - deltaY;
	drawPixel(xstop, ystop, light);

	while (xstart != xstop || ystart != ystop) 
	{
		drawPixel(xstart, ystart, light);

		const int16_t error2 = error * 2;
 
		if (error2 > -deltaY) 
		{
			error -= deltaY;
			xstart += signX;
		}
		if (error2 < deltaX)
		{
			error += deltaX;
			ystart += signY;
		}
	}
}


void SSD1306::setFont(const unsigned char font[])
{
	this->font = font;
}

void SSD1306::printChar(uint8_t x, uint8_t y, char chr, bool light) {
	
	int8_t i, j;
	uint8_t f_width = font[0];	
	uint8_t f_height = font[1];

	for (i = 0; i < (f_width + 1); i++) {
		uint8_t line;
		if (i < f_width) 
			line = font[(2 + (chr * f_width) + i)];
		else {
			line = 0x00;
		}
		for (j = 0; j < f_height + 1; j++, line >>= 1) {
			if (line & 0x01)
			{
				drawPixel(x + i, y + (f_height - j), light);
			}
			else
			{
				drawPixel(x + i, y + (f_height - j), !light);
			}
		}
	}
}
void SSD1306::printString(uint8_t x, uint8_t y, char *str, bool light) {
	while (*str != 0) {
		printChar(x, y, *str, light);
		x += font[0] + 1; //increment to font width
		str++;
	}
}


void SSD1306::printf(uint16_t x, uint16_t y, bool light, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	memset(buf, 0x00, 20);
	// ReSharper disable once CppLocalVariableMightNotBeInitialized
	vsprintf(buf, format, args);
	printString(x, y, buf, light);
}

void SSD1306::printf(uint16_t x, uint16_t y, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	memset(buf, 0x00, 20);
	// ReSharper disable once CppLocalVariableMightNotBeInitialized
	vsprintf(buf, format, args);
	printString(x, y, buf, true);
}
