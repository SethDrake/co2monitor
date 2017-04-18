
#include <stm32f1xx_hal.h>
#include <cmsis_os.h>
#include "interrupts.h"
#include "delay.h"
#include "objects.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
* @brief   This function handles NMI exception.
* @param  None
* @retval None
*/
	void __attribute__((interrupt("IRQ"))) NMI_Handler(void)
{
}

void __attribute__((interrupt("IRQ"))) HardFault_Handler(void)
{
	__asm__("TST LR, #4");
	__asm__("ITE EQ");
	__asm__("MRSEQ R0, MSP");
	__asm__("MRSNE R0, PSP");
	__asm__("B hard_fault_handler");
}

/**
* @brief  This function handles Memory Manage exception.
* @param  None
* @retval None
*/
void __attribute__((interrupt("IRQ"))) MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
	}
}

/**
* @brief  This function handles Bus Fault exception.
* @param  None
* @retval None
*/void __attribute__((interrupt("IRQ"))) BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
	}
}

/**
* @brief  This function handles Usage Fault exception.
* @param  None
* @retval None
*/
void __attribute__((interrupt("IRQ"))) UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
	}
}

/*void __attribute__((interrupt("IRQ"))) SVC_Handler(void)
{
}*/

void __attribute__((interrupt("IRQ"))) DebugMon_Handler(void)
{
}

/*void __attribute__((interrupt("IRQ"))) PendSV_Handler(void)
{
}*/

void __attribute__((interrupt("IRQ"))) SysTick_Handler(void) {
	//DelayManager::SysTickIncrement();
	osSystickHandler();
	HAL_IncTick();
}

void __attribute__((interrupt("IRQ"))) DMA1_Channel6_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&i2cDmaTx);
}

void __attribute__((interrupt("IRQ"))) USB_LP_CAN1_RX0_IRQHandler(void)
{
	HAL_PCD_IRQHandler(&hpcd_USB_FS);
}

void __attribute__((interrupt("IRQ"))) TIM1_UP_IRQHandler(void)
{
	//HAL_TIM_IRQHandler(&htim1);	
}

void hard_fault_handler(unsigned int * hardfault_args)
{
	unsigned int stacked_r0;
	unsigned int stacked_r1;
	unsigned int stacked_r2;
	unsigned int stacked_r3;
	unsigned int stacked_r12;
	unsigned int stacked_lr;
	unsigned int stacked_pc;
	unsigned int stacked_psr;

	stacked_r0 = ((unsigned long) hardfault_args[0]);
	stacked_r1 = ((unsigned long) hardfault_args[1]);
	stacked_r2 = ((unsigned long) hardfault_args[2]);
	stacked_r3 = ((unsigned long) hardfault_args[3]);

	stacked_r12 = ((unsigned long) hardfault_args[4]);
	stacked_lr = ((unsigned long) hardfault_args[5]);
	stacked_pc = ((unsigned long) hardfault_args[6]);
	stacked_psr = ((unsigned long) hardfault_args[7]);

	if (display.IsActive())
	{
		display.printf(10, 56, "HARD FAULT!");
		display.printf(10, 40, "LR = %x", stacked_lr);
		display.printf(10, 30, "PC = %x", stacked_pc);
		display.printf(10, 20, "PSR = %x", stacked_psr);
	}
	
	/*display.setDisableDMA(1);
	display.setColor(WHITE, BLUE);
	display.clear(BLUE);
	display.printf(10, 220, "HARD FAULT DETECTED --- SYSTEM STOPPED");
	display.printf(10, 200, "R0 = %x", stacked_r0);
	display.printf(10, 185, "R1 = %x", stacked_r1);
	display.printf(10, 170, "R2 = %x", stacked_r2);
	display.printf(10, 155, "R3 = %x", stacked_r3);
	display.printf(10, 140, "R12 = %x", stacked_r12);
	display.printf(10, 125, "LR [R14] = %x", stacked_lr);
	display.printf(10, 110, "PC [R15] = %x", stacked_pc);
	display.printf(10, 95, "PSR = %x", stacked_psr);
	display.printf(10, 80, "BFAR = %x", (*((volatile unsigned long *) (0xE000ED38))));
	display.printf(10, 65, "CFSR = %x", (*((volatile unsigned long *) (0xE000ED28))));
	display.printf(10, 50, "HFSR = %x", (*((volatile unsigned long *) (0xE000ED2C))));
	display.printf(10, 35, "DFSR = %x", (*((volatile unsigned long *) (0xE000ED30))));
	display.printf(10, 20, "AFSR = %x", (*((volatile unsigned long *) (0xE000ED3C))));
	display.printf(10, 5, "SCB_SHCSR = %x", SCB->SHCSR);*/

	while (1);
}

#ifdef __cplusplus
}
#endif
