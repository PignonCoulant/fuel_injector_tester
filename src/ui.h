#include "stm32f0xx.h"

#define THRESHOLD						100
#define LONG_PRESS_DURATION 			1500
#define PRESS_EXTRA_LONG_DURATION		3000


#define MAX_BUTTON	2
#define BUTTON_PRESSED	0x00

#define MAX_EVENT_FIFO_SIZE 16
#define MAX_ADC_SAMPLES		16

#define DEBOUNCE			50

#define SW_EVENT			0

#define SW_MODE				1
#define SW_START			0

#define START				0
#define STOP				1

#define MODE_LED			0
#define ON_OFF_LED			1



#define NORM				0
#define CONT				1

typedef struct{
	uint8_t type; //pot or switch
	uint8_t id;		//which one
	uint16_t data;		//value for adc or time pressed
}USER_EVENT;




void switch_debounce(uint8_t sw);
void user_button_init(void);
void get_user_event(USER_EVENT *event); 
void push_user_event(USER_EVENT *event); 
void user_event_poll(void); 
void user_event_do(void);
