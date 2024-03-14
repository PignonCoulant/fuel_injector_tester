#include "ui.h"
#include "pwm.h"
#include "system_time.h"

#define DEBUG_LED_TIME				250
#define TOOGLE_DEBUG_LED			GPIOB->ODR ^= 1<<1

uint8_t led_pattern[5]=
{
		0x01,
		0x02,
		0x04,
		0x08,
		0x10
};
uint8_t ptr_event_read, ptr_event_write, event_fifo_counter; 
USER_EVENT event_fifo[MAX_EVENT_FIFO_SIZE];
uint16_t gpio_mask[MAX_BUTTON] ={ GPIO_IDR_6, GPIO_IDR_7};
uint16_t press_timer[MAX_BUTTON]; 
uint32_t press_time[MAX_BUTTON]; 
uint32_t debug_time[MAX_BUTTON]; 
uint8_t state[MAX_BUTTON]; 

uint16_t debug_led_timer = DEBUG_LED_TIME;

void user_button_init(void)
{
	GPIOA->PUPDR |= ((1<<12) | (1<<14));
}
void get_user_event(USER_EVENT *event)
{
	if(event_fifo_counter >0)
	{
		event->type 	= event_fifo[ptr_event_read].type;
		event->id 		= event_fifo[ptr_event_read].id;
		event->data 	= event_fifo[ptr_event_read].data; 

		ptr_event_read++; 
		if(ptr_event_read >= MAX_EVENT_FIFO_SIZE) ptr_event_read = 0; 
		
		event_fifo_counter--;
	}
}
void push_user_event(USER_EVENT *event)
{
	if(event_fifo_counter < MAX_EVENT_FIFO_SIZE)
	{
		event_fifo[ptr_event_write].type	= event->type;
		event_fifo[ptr_event_write].id 		= event->id;
		event_fifo[ptr_event_write].data 	= event->data; 		
		
		ptr_event_write++; 
		if(ptr_event_write >= MAX_EVENT_FIFO_SIZE) ptr_event_write = 0; 
		
		event_fifo_counter++;
	}
}
void user_event_poll(void)
{
 USER_EVENT event;

	for(int i=0; i<2; i++)
	{
		if(!state[i])
		{
			if(!(GPIOA->IDR & gpio_mask[i])) 
			{
				press_timer[i]++; 
				if(press_timer[i] >= DEBOUNCE)	
				{
					state[i]++;
					press_timer[i] = 0; 
					press_time[i] = milliseconds(); 
				}
			
			}
			else press_timer[i] = 0; 
		}

		else if(state[i])
		{
			if(GPIOA->IDR & gpio_mask[i])
			{
				press_timer[i]++; 
				if(press_timer[i] >= DEBOUNCE)	
				{
					state[i]--;
					event.type = SW_EVENT;
					event.id = i;
					event.data = milliseconds() - press_time[i];
					push_user_event(&event);
					press_timer[i] = 0; 
				}
			}
			else press_timer[i] = 0; 
		}
	}

	uint16_t led = led_pattern[get_mode()];

	 //display which mode we're in
	 GPIOA->ODR = led;

	 //debug led for seeing if the thing is running.
	 debug_led_timer--;
	 if(!debug_led_timer)
	 {
		 debug_led_timer = DEBUG_LED_TIME;
		 TOOGLE_DEBUG_LED;
	 }
}
void user_event_do(void)
{
	USER_EVENT event;

	if(event_fifo_counter)			// is there some event to process ?
	{
		get_user_event(&event);

		if(event.type == SW_EVENT)
		{
			if(event.id == SW_START) user_start_cmd(STOP);

			if(event.id == SW_MODE)
			{
				if(event.data >= LONG_PRESS_DURATION)	toogle_user_cont_mode();

				else  user_change_mode(NORM);
			}
		}
	}
}
