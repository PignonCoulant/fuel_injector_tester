#include "stm32f0xx.h"

#define MAX_REPETITION_MODE			3
#define ACTIVE_PWM_OUTPUTS			TIM1->CCER  |= 	TIM_CCER_CC2E 	| TIM_CCER_CC3E
#define DISABLE_PWM_OUTPUTS			TIM1->CCER  &=~	TIM_CCER_CC2E; TIM1->CCER  &=~	TIM_CCER_CC3E

#define UPDATE_TIMER_REGISTER		TIM1->EGR |= TIM_EGR_UG

#define PWM_MOE_CONDITION			TIM1->BDTR	& (1<<15)

#define PWM_OUTPUT_DISABLED			TIM1->BDTR 	&=~ TIM_BDTR_MOE
#define PWM_OUTPUT_ENABLED			TIM1->BDTR 	|= TIM_BDTR_MOE

#define TIMER_CONDITION		TIM1->CR1 & 1
#define START_TIMER			TIM1->CR1 |= 1<<0;
#define STOP_TIMER			TIM1->CR1 &=~(1<<0);

#define START_STOP_CMD_BIT	1<<0
#define LOCK_MODE_BIT		1<<1
#define LATCH_CMD_BIT		1<<2

#define MAX_MODE			5

#define CONTINUS_MODE		1

#define LOCK_MODE_SIG		4
#define MAX_DURATION_MODE	4

#define BLINK_LED			GPIOB->ODR ^= 1<<1


typedef struct{
	uint8_t repetition;
	uint16_t width;
	uint16_t period;
}PWM_MODE;

/*
 * connect or disconnect the PWM module to the output pin. THis is to avoid the coil in the injector
 * to burn out if something goes wrong. The pin is always on Low state.
 */
void pwm_link(uint8_t);


/* return the user_mode number, useful for displaying the data on the leds */
uint8_t get_mode(void);


uint8_t get_timer_condition(void);
void load_timer_data(uint8_t);
void pwm_pin_init(void);
void init_pwm_mode(void);
uint8_t user_on_off_cmd(void);
void timer_config(uint8_t conf);
void user_start_cmd(uint8_t);
uint8_t user_lock_mode(void);
void toogle_user_cont_mode (void);


void user_change_mode(uint8_t);


void change_PWM_param(uint8_t);
void timer_process(void);

/*we reduce the 48000000mhz clock to 6khz
 *
 * => clock_tick	=  1/6K = 16uS tick => 0.016uS => 0.000016S
 * we need the numbers to be on the same unit to be calculated
 *
 *ARR/CCR count = period(S) / clock_tick(s)
 *
 *
 *
 *Precaler of the timer :8000
 *
 * 1er mode : imp. 250ms p: 1.5s
 * ARR : 4012
 * COMP : 1562
 *
 * 2nd mode : 50imp 7ms 250ms
 * ARR : 1562
 * COMP : 45
 * CRC : 50
 *
 * 3rd mode : 100 imp. 3.5ms 250ms
 * ARR : 1562
 * COMP: 22
 *
 * !-----------------------------------------!
 * 870RPM = 137 ms	-> clock = 6000Kh ARR = 8563
 * 2000RPM = 60ms	-> clock = 6000kh ARR = 3750
 * 6000RPM = 20ms 	-> clock = 6000kh ARR = 1250
 *
 *!-------------------------------------------!
	THE OTC module offers 3 modes with 0.5ms periods with variable duty cycle

	6Khz clock -> 0.000166s -> 166uS tick

	we need 3012 ticks (0.5 / 0.000166) to reach 500ms interval (value loaded into the ARR register)
	value in CCR varies :
	10ms -> 60
	5ms  -> 30

 *!-------------------------------------------!
 *!-------------------------------------------!
 *!-------------------------------------------!
 *!-------------------------------------------!
 *!-------------------------------------------!
 *
 *to change mode, be sure that the previous repetitions are done or
 *force the PWM pin low to avoid having the pin and the mosfet closed
 *
 *
 *
 *IN continus mode (two isntance). The start/stop command only shut down the timer
 *BUT we need to force the output of PWM low everytime we stop to not let the coil
 *get hot and burn.
 *
 *
 *
 *
 */
