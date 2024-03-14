#include "pwm.h"

// config PA9 for AF TIM1_CH2 PWM
/*
 * LOW 	870RPM = 137ms	-> clock = 6Kh ARR = 8563
 * MED 	2000RPM = 60ms	-> clock = 6kh ARR = 3750
 * FAST 6000RPM = 20ms 	-> clock = 6kh ARR = 1250
 *
 * PULSE WIDTH (6Khz)
 *5ms = 313
 *
 * 3.5ms = 219
 * 2.5ms = 157
 * 1.5ms = 93
 * 1ms = 63
 *
 *
*/
uint8_t user_mode = 0;
uint8_t old_user_mode = 0;

uint8_t lock_mode;

const uint16_t arr_v = 1499;
const uint8_t comp_v = 21;



 //value for the timer  ARR and CCR3
 /* 1 / 100ms @ 35ms on
  * 2 / 50ms  @ 15ms on
  * 3 / 20ms  @ 3ms
  */
const uint16_t pulse_width[5]		= {210,90,18,0,0};
const uint16_t pulse_period[5] 		= {602,301,120,0,0};
const uint8_t repetition_count[5] 	= {6, 11, 21, 0, 0};


uint8_t current_repetition_count;
uint8_t pulse_count;

uint8_t cont_lock;
uint8_t count;

uint8_t pulse_counter;

uint8_t timer_flag;

uint8_t get_mode(void)
{
	return user_mode;

}

uint8_t get_timer_condition(void)
{
	if(TIM1->CR1 & 1) 	return 1;
	else 				return 0;
}

//Horrible and ugly mess to get the pin disconnected to  PWM module
//it seems that exiting MOE mode leave the pin in high impedance and triggers the fet with static.
// we need the pin to go low !
void pwm_link(uint8_t state)
{

	if(!state)
	{
		 //remvoe the af fonction
		GPIOA->AFR[1] 	&=~ (1<<5);
		GPIOA->AFR[1] 	&=~ (1<<9);

		//disable the AF to the pin and go back to the general purpose mode
		GPIOA->MODER 	&=~ (1<<19);
		GPIOA->MODER 	&=~ (1<<21);

		//go back to the general output mode
		GPIOA->MODER 	|= (1<<18);
		GPIOA->MODER 	|= (1<<20);

		//low state
		GPIOA->ODR &=~ 1<<9;
		GPIOA->ODR &=~ 1<<10;

		//cut the link with the PWM
		TIM1->BDTR 	&=~ TIM_BDTR_MOE ;

	}
	else
	{
		// AF and mode register to link the PWM to the pin
		GPIOA->MODER 	&=~ (1<<18);
		GPIOA->MODER 	&=~ (1<<20);
		GPIOA->MODER 	|= (1<<19 | 1<<21);
		GPIOA->AFR[1] 	|= (1<<5 | 1<<9);
		TIM1->BDTR 	|= TIM_BDTR_MOE ;
	}
}

void pwm_pin_init(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
	//PA10 , PA9 -> PWM output, IDLE = LOW
	GPIOA->MODER 	|= (1<<19 | 1<<21);
	GPIOA->AFR[1] 	|= (1<<5 | 1<<9);

	TIM1->PSC = 8000;

	//PWM setting up

	TIM1->CCMR1 |=  TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1;
	TIM1->CCMR2 |= 	TIM_CCMR2_OC3PE | TIM_CCMR2_OC3M_2 | TIM_CCMR2_OC3M_1;
	TIM1->CCER  |= 	TIM_CCER_CC2E 	| TIM_CCER_CC3E;


	//active low
	TIM1->CCER |= (1<<5 | 1<<9);

	//PWM_OUTPUT_DISABLED;     	// need this to set the pin low at boot up.
	TIM1->CR1 |= TIM_CR1_ARPE;
	TIM1->DIER |= TIM_DIER_UIE;

	TIM1->BDTR 	|= TIM_BDTR_MOE ;

	TIM1->ARR = pulse_period[user_mode];
	TIM1->CCR2 =pulse_width[user_mode];
	TIM1->CCR3 =pulse_width[user_mode];
	current_repetition_count = repetition_count[user_mode];

    TIM1->EGR |= 1<<TIM_EGR_UG;
	NVIC_EnableIRQ( TIM1_BRK_UP_TRG_COM_IRQn);
	pwm_link(0);

}

void timer_process(void)
{
	if(timer_flag)
	{
		timer_flag = 0;
		if(get_mode() < MAX_REPETITION_MODE)
		TIM1->CR1 &=~  TIM_CR1_CEN;	// in REPETITION_MODE we stop the timer after a delimited number of pulses

	}
}

void user_start_cmd(uint8_t on_condition)
{
	TIM1->CR1 ^= 1<<0;
	if(TIMER_CONDITION)	pwm_link(1);
	else pwm_link(0);
}

//need to be processed only when the timer is OFF !
void timer_config(uint8_t conf)
{
	//we need to distinguish the mode we are in: pulse or contionus
			// CONT. mode

	//reset timer
	TIM1->CNT = 0;

	if(conf >= MAX_REPETITION_MODE)
	{
		TIM1->ARR = pulse_period[conf - MAX_REPETITION_MODE];
		TIM1->CCR2 = pulse_width[conf - MAX_REPETITION_MODE];
		TIM1->CCR3 = pulse_width[conf - MAX_REPETITION_MODE];

		//we ignore pulse_counter as we are in continus mode and the timer is only
		//stopped by the user
	}
	else
	{
		current_repetition_count = repetition_count[conf];
		TIM1->ARR = pulse_period[conf];
		TIM1->CCR2 = pulse_width[conf];
		TIM1->CCR3 = pulse_width[conf];
	}




	TIM1->EGR |= 1<<TIM_EGR_UG;



}

void toogle_user_cont_mode()
{
	cont_lock ^= 1<<0;
	user_change_mode(0);
}

// when the user call for a new config we need to reassign param. to the working register.
// we also need to consider if we're continus mode or not.

void user_change_mode(uint8_t lock)
{

	// in lock (continus) user_mode should only
	//know two values (2 top led) 3 and 4
	// it also should bring up to 3 comming from the
	//normal mode.

	//if the timer is still running we do nothing, otherwise the timer is OFF we can
	// change the param
	if(!(TIMER_CONDITION))
	{
		// if we're in continius mode, the user_mode knows only 2 values 3 and 4
		if(cont_lock)
		{
			if(user_mode < 3) user_mode = 3;
			else user_mode++;

			if(user_mode == MAX_MODE) user_mode = 3;
		}

		// 	limit the user mode range to 0,1 and 2.
		// 	if the value is above that limit, trim it to 0
		// 	which will happen when exiting the previous lock mode
		//	hence the count will return to 0 after leaving the cont_lock mode

		// PULSE MODE
		else
		{
				if(user_mode >= 3) user_mode = 0;
				else user_mode++;
				if(user_mode >= (MAX_MODE-2))	user_mode = 0;
		}
		// now that we have a valid config id from the user we can reset the
		//timer and registers with new data for the next pass
		timer_config(user_mode);

	}

}
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
	if(TIM1->SR & 1)
	{
		//BLINK_LED;						// just to be sure it's working
		TIM1->SR &=~ 1<<0;				//acknlg the interupt

		//distinguish between pulse or continius mode
		if(user_mode < MAX_REPETITION_MODE)
		{
			pulse_count++;

			if(pulse_count == current_repetition_count)
			{
				//reset the count
				pulse_count = 0;

				//stop the interupt
				TIM1->CR1 &=~ TIM_CR1_CEN;

				// disconnect the PWM pin to the module
				pwm_link(0);
			}

			//we haven't finished the determined number of pulses
			else TIM1->CR1 |= TIM_CR1_CEN;		// reload for the next round
		}

		//we are in continuis mode, nothing matters anymore !
		else TIM1->CR1 |= TIM_CR1_CEN;


	}
}



