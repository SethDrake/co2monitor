
#include "delay.h"
#include <stm32f1xx_hal.h>

volatile uint32_t DelayManager::timingDelay;
volatile uint64_t DelayManager::sysTickCount;

DelayManager::DelayManager() {
	this->sysTickCount = 0;
	this->timingDelay = 0;
}

DelayManager::~DelayManager() {

}

void DelayManager::Delay(volatile uint32_t nTime){
  timingDelay = nTime;
  while(timingDelay > 0);
}

void DelayManager::TimingDelay_Decrement(void){
  if (timingDelay > 0){
    timingDelay--;
  }
}

void DelayManager::SysTickIncrement(void) {
	sysTickCount++;
	TimingDelay_Decrement();
}

uint64_t DelayManager::GetSysTickCount() {
	return sysTickCount;
}

void DelayManager::DelayMs(volatile uint32_t nTime)
{
	volatile uint64_t nCount;
	nCount = ((uint64_t)HAL_RCC_GetHCLKFreq() * nTime / 50000);
	for (; nCount != 0; nCount--);
}

void DelayManager::DelayUs(volatile uint32_t nTime)
{
	volatile uint64_t nCount;
	nCount = ((uint64_t)HAL_RCC_GetHCLKFreq() * nTime / 50000000);
	for (; nCount != 0; nCount--);
}

