
#pragma once
#ifndef __PERIPH_CONFIG_H_
#define __PERIPH_CONFIG_H_

/* PA13, PA14 - SWD - DON'T USE!!! */
/* PA11, PA12 - USB */

#define I2C_PORT		GPIOB
#define I2C_SCL			GPIO_PIN_6
#define I2C_SDA			GPIO_PIN_7

#define KBRD_PORT		GPIOB
#define KBRD_PIN		GPIO_PIN_0

#define USART_PORT		GPIOA
#define USART_TX	    GPIO_PIN_9
#define USART_RX		GPIO_PIN_10

#endif //__PERIPH_CONFIG_H_
