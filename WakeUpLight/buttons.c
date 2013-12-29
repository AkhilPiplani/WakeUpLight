/*
 * buttons.c
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
#include "buttons.h"

#if SIMPLIFIED_BUTTONS

void buttons_init() {
	// Configure the peripheral and pins
	ROM_SysCtlPeripheralEnable(BUTTONS_PORTENABLE);
	// ROM_GPIOPadConfigSet(BUTTONS_PORT, BUTTONS_ALL_PINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
	ROM_GPIOPadConfigSet(BUTTONS_PORT, BUTTONS_ALL_PINS, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPD); // Use this if buttons don't read so well.
	ROM_GPIOPinTypeGPIOInput(BUTTONS_PORT,  BUTTONS_ALL_PINS);

}

long buttons_poll() {
	// No de-bouncing is needed for now.
	// If this code detects the snooze button being hit many times due to bouncing, it doesn't matter.
	return ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_ALL_PINS);

	// Debug code to print the button status when it's pressed.
//	unsigned long status = ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_ALL_PINS);
//
//	if(status != 0) {
//		printf("Buttons: %d \r\n", status);
//	}
//
//	return status;

	// Simple debouncing code.
//	if(ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_ALL_PINS) != 0) {
//		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 3 / 4);
//		if(ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_ALL_PINS) != 0) {
//			return ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_ALL_PINS);
//		}
//	}
//	return 0;
}

#else

static unsigned int DebouncingCount = 0;
static unsigned long LastColumnsState = 0;

static const unsigned long RowPins[3] = {BUTTONS_PIN1, BUTTONS_PIN2, BUTTONS_PIN3};

tBoolean ButtonsStates[BUTTONS_NB_BUTTONS];

void buttons_init() {
	unsigned int i;

	for(i=0; i<BUTTONS_NB_BUTTONS; i++) {
		ButtonsStates[i] = false;
	}

	DebouncingCount = 0;
	LastColumnsState = 0;

	// Configure the peripheral and pins
	ROM_SysCtlPeripheralEnable(BUTTONS_PORTENABLE);
	ROM_GPIOPinTypeGPIOOutput(BUTTONS_PORT, BUTTONS_ROW_PINS);
	ROM_GPIOPadConfigSet(BUTTONS_PORT, BUTTONS_COLUMN_PINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
	ROM_GPIOPinTypeGPIOInput(BUTTONS_PORT,  BUTTONS_COLUMN_PINS);
	//ROM_GPIOPadConfigSet(BUTTONS_PORT, BUTTONS_COLUMN_PINS, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPD); // Use this if the pins don't read correctly

	// Set all the row-pins to high
	ROM_GPIOPinWrite(BUTTONS_PORT, BUTTONS_ROW_PINS, BUTTONS_ROW_PINS);
}

void buttons_poll() {
	unsigned int i;
	unsigned long columnsState, scanColumnsState, columnOffset = 0;

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
				ButtonsStates[i] = false;
			}
			return;
		}

		// Scan buttons by switching off columns and checking the row-states one by one
		for(i=0; i<3; i++) {
			// Set the i'th row low
			ROM_GPIOPinWrite(BUTTONS_PORT, BUTTONS_ROW_PINS, (BUTTONS_ROW_PINS & ~(RowPins[i])));

			// Is there need for this delay to let the pin settle?
			//ROM_SysCtlDelay(10);

			scanColumnsState = ROM_GPIOPinRead(BUTTONS_PORT, BUTTONS_COLUMN_PINS);
			if(scanColumnsState != columnsState) { // If columns change state when a row is set low, then this was the row whose button was pressed.
				if(columnsState & BUTTONS_PIN5) {
					columnOffset = 3;
				}
				ButtonsStates[columnOffset + i] = true;
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

#endif
