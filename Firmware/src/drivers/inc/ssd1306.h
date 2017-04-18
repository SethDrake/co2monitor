#pragma once
#ifndef __SSD1306_H
#define __SSD1306_H

#include <sys/_stdint.h>
#include <stm32f1xx_hal.h>

#define SSD1306_ADDR  0x78
#define SSD1306_TIMEOUT  1000

#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_BUFFERSIZE (SSD1306_WIDTH * SSD1306_HEIGHT)/8

#define SSD1306_SetContrast                 0x81
#define SSD1306_AllPixRAM                   0xA4
#define SSD1306_AllPixOn                    0xA5
#define SSD1306_SetInverseOff               0xA6
#define SSD1306_SetInverseOn                0xA7
#define SSD1306_Sleep                       0xAE
#define SSD1306_Wake                        0xAF
#define SSD1306_DeactivateScroll            0x2E
#define SSD1306_SetMemAdressingMode         0x20    
#define SSD1306_SetColumnAddr               0x21
#define SSD1306_SetPageAddr                 0x22
#define SSD1306_PageAddrMode_SetPage        0xB0
#define SSD1306_PageAddrMode_StartColumnLo  0x00
#define SSD1306_PageAddrMode_StartColumnHi  0x10
#define SSD1306_SetDisplayStartLine         0x40
#define SSD1306_SetSegmentRemap             0xA0
#define SSD1306_SetMultiplexRatio           0xA8
#define SSD1306_SetCOMoutScanDirection      0xC0 
#define SSD1306_SetDisplayOffset            0xD3
#define SSD1306_SetCOMPinsConfig            0xDA
#define SSD1306_SetDisplayClockDivider      0xD5
#define SSD1306_SetPrechargePeriod          0xD9
#define SSD1306_SetVCOMHDeselectLevel       0xDB
#define SSD1306_ChargePumpSetting           0x8D

class SSD1306 {
public:
	SSD1306();
	~SSD1306();
	void initDisplay(I2C_HandleTypeDef* i2c);
	void sleepMode(bool enabled);
	void setContrast(uint8_t contrast);
	void clearScreen(bool light = false);
	void drawPixel(uint8_t x, uint8_t y, bool light = true);
	void drawLine(uint8_t xstart, uint8_t ystart, uint8_t xstopt, uint8_t ystop, bool light = true);
	void setFont(const unsigned char font[]);
	void printChar(uint8_t x, uint8_t y, char letter, bool light = true);
	void printString(uint8_t x, uint8_t y, char *string, bool light = true);
	void printf(uint16_t x, uint16_t y, bool light, const char *format, ...);
	void printf(uint16_t x, uint16_t y, const char *format, ...);
	void drawFramebuffer(void);
	bool IsSleep(void);
	bool IsActive(void);
protected:
	I2C_HandleTypeDef* i2c;
	const unsigned char *font;
private:
	bool isSleep;
	bool isActive;
	char buf[20];
	uint8_t framebuffer[SSD1306_BUFFERSIZE + 1];
	bool sendJustCommand(uint8_t cmd);
	bool sendCommandByte(uint8_t cmd, uint8_t value);
	bool sendCommand(uint8_t cmd, uint8_t* pBuffer, uint16_t NumByteToWrite);
	bool sendData(uint8_t* pBuffer, uint16_t NumByteToWrite);
};

#endif /* __SSD1306_H */


