#include <sys/_stdint.h>

#include <stm32f1xx_hal.h>

#include "cmsis_os.h"

#include "drivers/inc/ssd1306.h"
#include "src/drivers/inc/fonts.h"
#include "drivers/inc/co2sensor.h"
#include "periph_config.h"
#include "objects.h"
#include "delay.h"
#include <cerrno>
#include "usb/usb_device.h"
#include "usb/usbd_cdc_if.h"

DMA_HandleTypeDef i2cDmaTx;
UART_HandleTypeDef uart;
I2C_HandleTypeDef i2c;

SSD1306 display;
CO2Sensor co2sensor;
SYSTEM_MODE systemMode;
GRPAH_MODE graphMode;

bool sensorOk;

const char logoStr[] = "CO2 MONITOR V.0.1";
const char errorStr[] = "  SENSOR ERROR!  ";

osThreadId processSensorTaskHandle;
osThreadId processKeysTaskHandle;
osThreadId drawDataTaskHandle;

void errorHandler(const char* reason);

void DBG_Configuration()
{
	//Enable debug in powersafe modes
	HAL_DBGMCU_EnableDBGSleepMode();
	HAL_DBGMCU_EnableDBGStopMode();
	HAL_DBGMCU_EnableDBGStandbyMode();

	//hard fault on div by zero
	SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;
}

void SystemClock_Configuration()
{
	
	/*RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.PLL.PLLState    = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource   = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLMUL      = RCC_PLL_MUL9;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		while (true); //error on init clock
	}*/

	/*RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		errorHandler(); //error on init clock
	}*/

	RCC_PeriphCLKInitTypeDef PeriphClkInit;
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
	PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
	{
		errorHandler("PERIPH_CLCK"); //error on init clock
	}

	/**Configure the Systick interrupt time */
	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);
	/**Configure the Systick */
	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	SystemCoreClockUpdate();
}

void RCC_Configuration()
{

#if (PREFETCH_ENABLE != 0)
	__HAL_FLASH_PREFETCH_BUFFER_ENABLE();
#endif
	
	__HAL_RCC_BKP_CLK_ENABLE();
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_I2C1_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();
}

void I2C_Configuration(I2C_HandleTypeDef* i2cHandle)
{
	i2cHandle->Instance             = I2C1;
	i2cHandle->Init.ClockSpeed      = 400000; //400000
	i2cHandle->Init.DutyCycle       = I2C_DUTYCYCLE_2;
	i2cHandle->Init.OwnAddress1     = 0x00;
	i2cHandle->Init.AddressingMode  = I2C_ADDRESSINGMODE_7BIT;
	i2cHandle->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	i2cHandle->Init.OwnAddress2     = 0x00;
	i2cHandle->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	i2cHandle->Init.NoStretchMode   = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(i2cHandle) != HAL_OK)
	{
		errorHandler("I2C_INIT");
	}
	__HAL_I2C_ENABLE(i2cHandle);
}

void USART_Configuration(UART_HandleTypeDef* usartHandle)
{
	usartHandle->Instance			= USART1;
	usartHandle->Init.BaudRate		= 9600;
	usartHandle->Init.WordLength	= UART_WORDLENGTH_8B;
	usartHandle->Init.StopBits		= UART_STOPBITS_1;
	usartHandle->Init.Parity		= UART_PARITY_NONE;
	usartHandle->Init.HwFlowCtl		= UART_HWCONTROL_NONE;
	usartHandle->Init.Mode			= UART_MODE_TX_RX;
	usartHandle->Init.OverSampling  = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(usartHandle) != HAL_OK)
	{
		errorHandler("USART_INIT");
	}
	__HAL_UART_ENABLE(usartHandle);
}

void DMA_I2C_TX_Configuration(DMA_HandleTypeDef* dmaHandle)
{
	dmaHandle->Instance                 = DMA1_Channel6;
	dmaHandle->Init.Direction           = DMA_MEMORY_TO_PERIPH;
	dmaHandle->Init.PeriphInc           = DMA_PINC_DISABLE;
	dmaHandle->Init.MemInc              = DMA_MINC_ENABLE;
	dmaHandle->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	dmaHandle->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
	dmaHandle->Init.Mode                = DMA_NORMAL;
	dmaHandle->Init.Priority            = DMA_PRIORITY_VERY_HIGH;
	if (HAL_DMA_Init(dmaHandle) != HAL_OK)
	{
		errorHandler("I2C_DMA_INIT");
	}
}

void GPIO_Configuration(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
		/* Configure I2C pins: SCL and SDA */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = I2C_SCL | I2C_SDA;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

		/* USART1 PINS */
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
	GPIO_InitStruct.Pin = USART_RX | USART_TX;
	HAL_GPIO_Init(USART_PORT, &GPIO_InitStruct);

		/* Configure KEYBOARD pins */
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = KBRD_PIN;
	HAL_GPIO_Init(KBRD_PORT, &GPIO_InitStruct);

	__HAL_AFIO_REMAP_SWJ_NOJTAG(); //disable JTAG
}

void EXTI_Configuration()
{

}

void NVIC_Configuration(void) {
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* System interrupt init*/
  /* MemoryManagement_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
	/* BusFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
	/* UsageFault_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
	/* SVCall_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SVCall_IRQn, 0, 0);
	/* DebugMonitor_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
	/* PendSV_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 14, 0);

	HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 15, 0);
	HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
}

void drawGraph(uint8_t y) 
{
	int8_t i, j;
	uint8_t maxHeight = 48;
	uint8_t barWidth = 2;
	uint8_t eleCounts = 60;
	uint16_t* counts = {};
	if (graphMode == SECONDS)
	{
		counts = co2sensor.GetPerSecondCo2();
		barWidth = 6;
		eleCounts = 17;
	}
	else if (graphMode == MINUTES)
	{
		counts = co2sensor.GetPerMinuteCo2();
		barWidth = 2;
		eleCounts = 50;
	}
	else if (graphMode == HOURS)
	{
		counts = co2sensor.GetPerHourCo2();
		barWidth = 4;
		eleCounts = 24;
	}
	uint8_t legendWidth = 28;
	uint16_t maxValue = *max_element(counts, counts + eleCounts - 1);
	display.drawLine(0, maxHeight + 12, legendWidth - 3, maxHeight + 12);
	display.printf(0, maxHeight  + 12 - 9, "%d", maxValue);
	uint8_t midPoint = maxHeight/ 2 - 1;
	display.printf(0, midPoint + y - 3, "%d", maxValue/2);
	//display.drawLine(0, midPoint + y, legendWidth - 3, midPoint + y);
	display.printString(0, y + 1, "0");
	display.drawLine(0, y, legendWidth - 3, y);
	float coeff = (maxValue > 0) ? maxHeight * 1.0f / maxValue : 0;
	for (int8_t r = eleCounts - 2; r >= 0; r--)
	{
		uint16_t lineHeight = counts[r+1] * coeff;
		if (lineHeight > maxHeight)
		{
			lineHeight = maxHeight;
		}
		for (i = maxHeight; i >= 0; i--)
		{
			if (barWidth > 1)
			{
				for (j = 0; j < barWidth - 1; j++)
				{
					display.drawPixel(legendWidth + r * barWidth + j, y + i, (i <= lineHeight));
				}
				display.drawPixel(legendWidth + r * barWidth + barWidth - 1, y + i, false);
			}
			else
			{
				for (j = 0; j < barWidth; j++)
				{
					display.drawPixel(legendWidth + r * barWidth + j, y + i, (i <= lineHeight));
				}	
			}
		}
	}
}



void switchSystemMode(SYSTEM_MODE mode)
{
	if (mode == systemMode)
	{
		return;	
	}
	if (mode == SLEEP)
	{
		systemMode = SLEEP;
	}
	else
	{
		systemMode = ACTIVE;
	}
}


void drawDataTask(void const * argument)
{
	uint16_t cntr = 0;
	bool point = true;
	while (true)
	{
		if (systemMode == ACTIVE && display.IsSleep())
		{
			display.sleepMode(false);
		}
		else if (systemMode == SLEEP && !display.IsSleep())
		{
			display.sleepMode(true);
		}
		if (sensorOk && !display.IsSleep())
		{
			uint16_t co2level = co2sensor.GetCurrentCO2Level();
			if (co2level >= 2500 && (co2level != 5000))
			{
				switchSystemMode(ACTIVE);	
			}
			display.clearScreen();
			if (co2level > 0)
			{ 
				if (graphMode == STATISTICS)
				{
					display.printf(20, 56, "MAX CO2 LEVELS");
					display.printf(0, 43, "TOTAL: %d ppm", co2sensor.GetMaxCO2Level());
					display.printf(0, 33, "PER DAY: %d ppm", co2sensor.GetMaxPerDayCO2Level());
					display.printf(0, 23, "PER HOUR: %d ppm", co2sensor.GetMaxPerHourCO2Level());
					display.printf(0, 13, "UPTIME: %d s.", co2sensor.GetUptime());
				}
				else
				{
					drawGraph(12);
				}

				if (isUsbSerialActive)
				{
					display.printf(100, 0, "U");	
				}
				display.printf(0, 0, "CO2:%d  T:%d%cC", co2level, co2sensor.GetCurrentTemperature(), 248);
				if (graphMode == SECONDS)
				{
					display.printf(109, 0, "SEC");	
				}
				else if (graphMode == MINUTES)
				{
					display.printf(109, 0, "MIN");
				}
				else if (graphMode == HOURS)
				{
					display.printf(109, 0, "HRS");
				}
				else if (graphMode == STATISTICS)
				{
					display.printf(109, 0, "STA");
				}
				if (cntr)
				{
					cntr = 0;
				}
			}
			else
			{
				display.printf(12, 50, logoStr);
				display.drawLine(0, 44, 127, 44);
				display.printf(12, 30, "SENSOR PREHEATING ");
				display.printf(12, 5, "C: %ds.", cntr / 2);
				display.printf(80, 5, "E: %d", co2sensor.GetErrorsCount());
				cntr++;
			}
			display.drawPixel(0, 63, point);
			display.drawPixel(1, 63, point);
			display.drawPixel(0, 62, point);
			display.drawPixel(1, 62, point);
			point = !point;
			display.drawFramebuffer();
		}

		osDelay(500);
	}
}

void processSensorTask(void const * argument)
{
	char buf[16] = {};
	while (true)
	{
		if (systemMode == LOADING)
		{
			osDelay(1000);
			return;
		}
		if (sensorOk)
		{
			co2sensor.TickSeconds(10);
			uint16_t co2level = co2sensor.GetCurrentCO2Level();
			uint16_t co2prevLevel = co2sensor.GetPerSecondCo2()[3];
			if ((co2level > 0) && (co2level <= 100) && (co2prevLevel > 0) && (co2prevLevel <= 100)) //reset sensor
			{
				co2sensor.resetSensor();	
			}
			if (isUsbSerialActive)
			{
				memset(buf, 0x00, 16);
				sprintf(buf, "CO2: %d\r\n", co2level);
				WriteToPort(buf, strlen(buf));
			}
		}
		osDelay(10000);
	}
}

void processKeysTask(void const * argument)
{
	uint16_t slpCntr = 0;
	bool prevState = false;
	while (true)
	{
		if (systemMode == LOADING)
		{
			osDelay(350);
			return;
		}
		bool keyPressed = !HAL_GPIO_ReadPin(KBRD_PORT, KBRD_PIN);
		if (keyPressed)
		{
			if (systemMode == ACTIVE)
			{
				if (prevState) //long press
				{
					switchSystemMode(SLEEP);
					osDelay(500);
				}
				else //short press
				{
					if (graphMode == SECONDS)
					{
						graphMode = MINUTES;
					}
					else if (graphMode == MINUTES)
					{
						graphMode = HOURS;
					}
					else if (graphMode == HOURS)
					{
						graphMode = STATISTICS;
					}
					else if (graphMode == STATISTICS)
					{
						graphMode = SECONDS;
					}
				}
			}
			else
			{
				switchSystemMode(ACTIVE);
				osDelay(500);
			}
			slpCntr = 0;
		}
		prevState = keyPressed;
		if (systemMode == ACTIVE)
		{
			slpCntr++;
		}
		if (slpCntr / 3 >= 120)
		{
			//switchSystemMode(SLEEP);
			slpCntr = 0;
		}
		osDelay(350);
	}
}

void processRequestFromHost(uint8_t* buf, uint32_t* size)
{
	char response[64] = {0};
	char* request = strstr((char*)buf, "REQ_");
	if (request)
	{	
		if (strstr(request, "REQ_INFO"))
		{
			sprintf(response, "CO2 SENSOR V.0.1\r\n");
			CDC_Transmit_FS((uint8_t*)response, strlen(response));
		}
		else if (strstr(request, "REQ_GET"))
		{
			uint16_t co2level = co2sensor.GetCurrentCO2Level();
			sprintf(response, "CO2: %d\r\n", co2level);
			CDC_Transmit_FS((uint8_t*)response, strlen(response));
		}
		else if (strstr(request, "REQ_MODE"))
		{
			if (graphMode == SECONDS)
			{
				graphMode = MINUTES;
			}
			else if (graphMode == MINUTES)
			{
				graphMode = HOURS;
			}
			else if (graphMode == HOURS)
			{
				graphMode = STATISTICS;
			}
			else if (graphMode == STATISTICS)
			{
				graphMode = SECONDS;
			}
			sprintf(response, "MODE CHANGED\r\n");
			CDC_Transmit_FS((uint8_t*)response, strlen(response));
		}
		else if (strstr(request, "REQ_SLP"))
		{
			switchSystemMode(SLEEP);
			sprintf(response, "DISPLAY OFF\r\n");
			CDC_Transmit_FS((uint8_t*)response, strlen(response));
		}
		else if (strstr(request, "REQ_WAKE"))
		{
			switchSystemMode(ACTIVE);
			sprintf(response, "DISPLAY ON\r\n");
			CDC_Transmit_FS((uint8_t*)response, strlen(response));
		}
		else if (strstr(request, "REQ_RST"))
		{
			co2sensor.resetSensor();
			sprintf(response, "SENSOR RESETED\r\n");
			CDC_Transmit_FS((uint8_t*)response, strlen(response));
		}
		else
		{
			sprintf(response, "NOT SUPPORTED\r\n");
			CDC_Transmit_FS((uint8_t*)response, strlen(response));
		}
	}
}

int main()
{
	DBG_Configuration();
	SystemClock_Configuration();
	DelayManager::DelayMs(150);
	RCC_Configuration();
	GPIO_Configuration();
	EXTI_Configuration();
	NVIC_Configuration();
	I2C_Configuration(&i2c);
	USART_Configuration(&uart);
	DMA_I2C_TX_Configuration(&i2cDmaTx);
	//__HAL_LINKDMA(&i2c, hdmatx, i2cDmaTx);

	MX_USB_DEVICE_Init();

	systemMode = LOADING;
	graphMode = SECONDS;

	display.initDisplay(&i2c);
	display.setFont(font5x7);
	display.clearScreen();
	display.printf(12, 50, logoStr);
	display.drawLine(0, 44, 127, 44);
	display.printf(12, 15, ".... LOADING ....");
	display.drawFramebuffer();

	sensorOk = co2sensor.initSensor(&uart);
	systemMode = ACTIVE;
	if (!sensorOk)
	{
		display.clearScreen();
		display.printf(12, 50, logoStr);
		display.drawLine(0, 44, 127, 44);
		display.printf(12, 15, errorStr);
		display.drawFramebuffer();
		errorHandler(NULL);
	}

	osThreadDef(processSensorThread, processSensorTask, osPriorityNormal, 0, 128);
	processSensorTaskHandle = osThreadCreate(osThread(processSensorThread), NULL);

	osThreadDef(processKeysThread, processKeysTask, osPriorityLow, 0, configMINIMAL_STACK_SIZE);
	processKeysTaskHandle = osThreadCreate(osThread(processKeysThread), NULL);

	osThreadDef(drawDataThread, drawDataTask, osPriorityHigh, 0, 256);
	drawDataTaskHandle = osThreadCreate(osThread(drawDataThread), NULL);

	osKernelStart();
	
	while (true)
	{
	}
}

void errorHandler(const char* reason)
{
	if (display.IsActive() && reason != NULL)
	{
		display.printf(0, 56, "SYSTEM ERROR!");
		display.printf(0, 40, "%s", *reason);
	}
	
	//Switch key pin to out
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Pin = KBRD_PIN;
	HAL_GPIO_Init(KBRD_PORT, &GPIO_InitStruct);
	while (true)
	{
		HAL_GPIO_TogglePin(KBRD_PORT, KBRD_PIN);
		DelayManager::DelayMs(500);
	}
}
