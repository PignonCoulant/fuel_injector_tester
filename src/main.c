

#include "stm32f0xx.h"
#include "system_time.h"
#include "pwm.h"
#include "ui.h"

#define F_CPU						48000000
#define LED_TIMER					166

#define WATCHDOG_REFRESH_TIME		33
#define WATCHDOG_RELOAD_VALUE		500
#define WATCHDOG_START_TIMER		0x0000CCCC
#define WATCHDOG_ENABLE_WRITE		0x00005555
#define WATCHDOG_KEY_REFRESH		0x0000AAAA

/* I/O config */

/*
 * PA0 - PA4 -> led output
 * PA7	-PA6-> button
 * PB1 -> LED output
 * PA9 - AF - PWM output
 * PA10 - led ouput
 */

void led_output_init(void)
{
	GPIOA->MODER |= ((1<<0) | (1<<2) | (1<<4) | (1<<6) | (1<<8) /*| 1<<20*/);
	//pull up on PA7 and PA6, input pin
	GPIOB->MODER |= (1<<2);

}

void watchdog_init(void)
{
	//40khz independant clock, 100ms window
	IWDG->KR = WATCHDOG_START_TIMER;
	IWDG->KR = WATCHDOG_ENABLE_WRITE;
	IWDG->PR = 1<<0;					//DIV 8
	IWDG->RLR = WATCHDOG_RELOAD_VALUE;
	while(IWDG->SR) {
	}
	IWDG->KR = WATCHDOG_KEY_REFRESH;
}

void blink(void)
{
	static uint16_t div;
	div++;
	if(div >= 2000)
	{
		div = 0;
		GPIOB->ODR ^= 1<<1;
	}

}

void SysTick_Handler()
{
	static uint8_t watchdog_timer = 0;

	system_tick();
	user_event_poll();

	watchdog_timer++;
	if(watchdog_timer >= WATCHDOG_REFRESH_TIME)
	{
		watchdog_timer -= WATCHDOG_REFRESH_TIME;
		IWDG->KR = WATCHDOG_KEY_REFRESH;
	}
} 0x800
			
int main(void)
{

	RCC->AHBENR |=  (RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOBEN);
	SysTick_Config (F_CPU /1000);
	//watchdog_init();
	user_button_init();
	led_output_init();
	pwm_pin_init();
	timer_config(0);


	while(1)
	{
		user_event_do();

	}
}
