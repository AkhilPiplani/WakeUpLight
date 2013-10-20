/*
 * buttons.h
 *
 *  Created on: 09-May-2013
 *      Author: Akhil
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <inc/hw_types.h>

// Previously I planned to scavenge an old system that had 6 buttons and an LCD.
// The 6-buttons were in a 2D row/column format where scanning was needed.
#define SIMPLIFIED_BUTTONS	1

#if SIMPLIFIED_BUTTONS

#define BUTTONS_PORT			GPIO_PORTC_BASE
#define BUTTONS_PORTENABLE		SYSCTL_PERIPH_GPIOC
#define BUTTONS_SNOOZE_PIN		GPIO_PIN_7
#define BUTTONS_ALL_PINS		BUTTONS_SNOOZE_PIN // Only one button for now, later can be logical OR of multiple buttons on the same port.

void buttons_init();
long buttons_poll();

#else

#define BUTTONS_PORT		GPIO_PORTA_BASE
#define BUTTONS_PORTENABLE	SYSCTL_PERIPH_GPIOA
//#define BUTTONS_INTERRUPT	INT_GPIOA

#define BUTTONS_DEBOUNCE_WAIT	2

// Buttons are arranged in a matrix arrangement.
// All the column pins are set as outputs(default-high) and all the row pins are set as inputs.

// Row pins
#define BUTTONS_PIN1		GPIO_PIN_2
#define BUTTONS_PIN2		GPIO_PIN_3
#define BUTTONS_PIN3		GPIO_PIN_4
#define BUTTONS_ROW_PINS	(BUTTONS_PIN1 | BUTTONS_PIN2 | BUTTONS_PIN3)

// Column pins
#define BUTTONS_PIN4		GPIO_PIN_5
#define BUTTONS_PIN5		GPIO_PIN_6
#define BUTTONS_COLUMN_PINS	(BUTTONS_PIN4 | BUTTONS_PIN5)

#define BUTTONS_NB_BUTTONS	6

// Can be used as indices for buttonStates
typedef enum _Buttons {
	down_button  = 0,
	up_button    = 1,
	left_button  = 2,
	right_button = 3,
	reset_button = 4,
	enter_button = 5
} Buttons;

extern tBoolean ButtonsStates[BUTTONS_NB_BUTTONS];

void buttons_init();
void buttons_poll();

#endif

#endif /* BUTTONS_H_ */
