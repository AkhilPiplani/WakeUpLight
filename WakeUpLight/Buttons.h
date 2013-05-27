/*
 * Buttons.h
 *
 *  Created on: 09-May-2013
 *      Author: Akhil
 */

#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <inc/hw_types.h>

#define BUTTONS_PORT		GPIO_PORTA_BASE
#define BUTTONS_PORTENABLE	SYSCTL_PERIPH_GPIOA

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

int Buttons_init(tBoolean *buttonStates);
void Buttons_poll(tBoolean *buttonStates);

#endif /* BUTTONS_H_ */
