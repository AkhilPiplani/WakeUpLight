/*
 * Buttons.c
 *
 *  Created on: 09-May-2013
 *      Author: Akhil
 */

#include <stddef.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/timer.h>
#include <driverlib/rom.h>
#include "Buttons.h"

static unsigned int DebouncingCount = 0;
static unsigned long LastColumnsState = 0;

static const unsigned long RowPins[3] = {BUTTONS_PIN1, BUTTONS_PIN2, BUTTONS_PIN3};

int Buttons_init(tBoolean *buttonStates) {
	unsigned int i;

	if(buttonStates == NULL) {
		return -1;
	}

	// Configure the peripheral and pins
	ROM_SysCtlPeripheralEnable(BUTTONS_PORTENABLE);
	ROM_GPIOPinTypeGPIOOutput(BUTTONS_PORT, BUTTONS_ROW_PINS);
	ROM_GPIOPadConfigSet(BUTTONS_PORT, BUTTONS_COLUMN_PINS, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	ROM_GPIOPinTypeGPIOInput(BUTTONS_PORT,  BUTTONS_COLUMN_PINS);
	//ROM_GPIOPadConfigSet(BUTTONS_PORT, BUTTONS_COLUMN_PINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD); // Use this if the pins don't read correctly

	// Set all the row-pins to high
	ROM_GPIOPinWrite(BUTTONS_PORT, BUTTONS_ROW_PINS, BUTTONS_ROW_PINS);

	DebouncingCount = 0;
	LastColumnsState = 0;

	for(i=0; i<BUTTONS_NB_BUTTONS; i++) {
		buttonStates[i] = false;
	}

	return 0;
}

void Buttons_poll(tBoolean *buttonStates) {
	unsigned int i;
	unsigned long columnsState, scanColumnsState, columnOffset = 0;

	if(buttonStates == NULL) {
		return;
	}

	// Continue debouncing
	if(DebouncingCount>0 && DebouncingCount<=BUTTONS_DEBOUNCE_WAIT) {
		DebouncingCount++;
		return;
	}
	else if(DebouncingCount > BUTTONS_DEBOUNCE_WAIT) {
		// Debouncing finished, scan the buttons and update states
		DebouncingCount = 0;


		columnsState = ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_COLUMN_PINS);
		LastColumnsState = columnsState;
		if(columnsState == 0) { // All buttons are in released state.
			for(i=0; i<BUTTONS_NB_BUTTONS; i++) {
				buttonStates[i] = false;
			}
			return;
		}

		// Scan buttons by switching off columns and checking the row-states one by one
		for(i=0; i<3; i++) {
			// Set the i'th row low
			ROM_GPIOPinWrite(BUTTONS_PORT, BUTTONS_ROW_PINS, (BUTTONS_ROW_PINS & ~(RowPins[i])));

			// TODO: Is there need for this delay to let the pin settle?
			//ROM_SysCtlDelay(10);

			scanColumnsState = ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_COLUMN_PINS);
			if(scanColumnsState != columnsState) { // If columns change state when a row is set low, then this was the row whose button was pressed.
				if(columnsState & BUTTONS_PIN5) {
					columnOffset = 3;
				}
				buttonStates[columnOffset + i] = true;
				break;
			}
		}

		ROM_GPIOPinWrite(BUTTONS_PORT, BUTTONS_ROW_PINS, BUTTONS_ROW_PINS);
	}

	// Start debouncing if needed
	columnsState = ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_COLUMN_PINS);
	if(columnsState!=LastColumnsState && DebouncingCount==0) {
		DebouncingCount++;
	}
}
