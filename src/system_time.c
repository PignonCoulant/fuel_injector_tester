#include "system_time.h"

uint32_t ms_counter; 

void init_system_tick(void)
{
	ms_counter = 0; 
}
void system_tick(void)
{
	ms_counter++; 
}
uint32_t milliseconds(void)
{
	return ms_counter; 
}
void init_system_clock(void)
{
	if ((RCC->CFGR & RCC_CFGR_SWS) == RCC_CFGR_SWS_PLL) // is the PLL the system clock ?
	{
		RCC->CFGR &= (uint32_t) (~RCC_CFGR_SW);		//switch to HSI
		
		while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI){}  //loop while the systemclock isn't on the internal clock 
	}
	
		RCC->CR &= (uint32_t)(~RCC_CR_PLLON);		// shutt off the PLL
		while((RCC->CR & RCC_CR_PLLRDY) != 0) {} //loop while PLL is still active
	
		RCC->CR |= 1<<16; 					//switch HSE on
		while(!(RCC->CR & 1<<17)); 	//wait for it to be clean and stable


	
	RCC->CFGR |= 1<<16 | (RCC_CFGR_PLLMUL6);
				RCC->CFGR |= (1<<24 | 1<<25 | 1<<26 | 1<<31);
		
	RCC->CR |= RCC_CR_PLLON; 		// turn the pLL on 
		
	while((RCC->CR & RCC_CR_PLLRDY) == 0) {}		// wait for the PLL to be ON
	
	RCC->CFGR |= (uint32_t) ((RCC_CFGR_SW_PLL));		//use PLL as clock 
		
	while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL){}		//wait 'till it is stabilised
}

