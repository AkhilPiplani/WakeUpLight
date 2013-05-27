/*
 * lights.c
 *
 *  Created on: 18-May-2013
 *      Author: Akhil
 */

#include <stddef.h>
#include <stdio.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_gpio.h>
#include <inc/hw_timer.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/gpio.h>
#include <driverlib/interrupt.h>
#include <driverlib/timer.h>
#include <driverlib/rom.h>
#include "lcd44780_LP.h"
#include "lights.h"
#include "Time.h"

// This whole module generally assumes a 50Hz AC input.

#define LIGHTS_AC_HALF_CYCLE_TIME (SysCtlClockGet()/100) // -20 cycles to account for miscellaneous delays.

static unsigned int ZeroCrossingCount;
static tBoolean DimmerPinState = false;
static unsigned long DimmerTriggerDelay;


void ISR_lights(void) {
	// Not using ROM_ functions here because they are slower.
	GPIOPinIntClear(LIGHTS_PORT, 0xFF); // Clear the interrupt at GPIO
	ZeroCrossingCount++; // Used to calculate zero-crossing frequency

	GPIOPinWrite(LIGHTS_PORT, LIGHTS_DIMMER_OUT, 0);

	// Only set the timer if the brightness is not 0 percent.
	if(DimmerTriggerDelay < LIGHTS_AC_HALF_CYCLE_TIME) {
		TimerLoadSet(LIGHTS_TIMER, TIMER_A, DimmerTriggerDelay);
		TimerEnable(LIGHTS_TIMER, TIMER_A);
	}
}

void ISR_lightsTimer(void) {
	unsigned long highTime = LIGHTS_AC_HALF_CYCLE_TIME - DimmerTriggerDelay;//SysCtlClockGet()/200;//SysCtlClockGet()/2000;

	// Not using ROM_ functions here because they are slower.
	TimerIntClear(LIGHTS_TIMER, TIMER_TIMA_TIMEOUT);  // Clear the interrupt at timer
	TimerDisable(LIGHTS_TIMER, TIMER_A);

	if(DimmerPinState == false) {
		// Set the dimmer out pin and set another timer to clear it.
		DimmerPinState = true;
//		if(highTime > (LIGHTS_AC_HALF_CYCLE_TIME - 20 - DimmerTriggerDelay)) {  // -20 cycles to account for miscellaneous delays.
//			// Not sure if the timer will trigger if we use 0 as highTime.
//			highTime = 4;
//		}
//		else {
//			// Don't pull the trigger if there isn't enough time left in the AC half-cycle
//			GPIOPinWrite(LIGHTS_PORT, LIGHTS_DIMMER_OUT, LIGHTS_DIMMER_OUT);
//		}

		GPIOPinWrite(LIGHTS_PORT, LIGHTS_DIMMER_OUT, LIGHTS_DIMMER_OUT);

		// Setup the timer to trigger this ISR again and switch off the dimmer pin.
		TimerLoadSet(LIGHTS_TIMER, TIMER_A, highTime);
		TimerEnable(LIGHTS_TIMER, TIMER_A);
	}
	else {
		// Clear the dimmer pin.
		DimmerPinState = false;
		GPIOPinWrite(LIGHTS_PORT, LIGHTS_DIMMER_OUT, 0);
	}
}

void lights_init() {
	DimmerTriggerDelay = LIGHTS_AC_HALF_CYCLE_TIME;
	ZeroCrossingCount = 0;

	ROM_SysCtlPeripheralEnable(LIGHTS_TIMERENABLE);
	ROM_TimerDisable(LIGHTS_TIMER, TIMER_A);
	ROM_TimerConfigure(LIGHTS_TIMER, TIMER_CFG_ONE_SHOT);
	ROM_TimerIntEnable(LIGHTS_TIMER, TIMER_TIMA_TIMEOUT);
	ROM_IntEnable(LIGHTS_TIMER_INTERRUPT);

	ROM_SysCtlPeripheralEnable(LIGHTS_PORTENABLE);
	ROM_GPIOPinTypeGPIOOutput(LIGHTS_PORT, LIGHTS_DIMMER_OUT);
	ROM_GPIOPadConfigSet(LIGHTS_PORT, LIGHTS_DIMMER_OUT, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	ROM_GPIOPinTypeGPIOInput(LIGHTS_PORT, LIGHTS_0CROSSING_IN);
	ROM_GPIOIntTypeSet(LIGHTS_PORT, LIGHTS_0CROSSING_IN, GPIO_FALLING_EDGE);
	ROM_GPIOPinIntDisable(LIGHTS_PORT, 0xFF);
	ROM_GPIOPinIntEnable(LIGHTS_PORT, LIGHTS_0CROSSING_IN);
	ROM_IntEnable(LIGHTS_0CROSSING_INTERRUPT);

}

void lights_setBrightness(unsigned char percent) {
	unsigned long delay  = LIGHTS_AC_HALF_CYCLE_TIME - LIGHTS_AC_HALF_CYCLE_TIME * (unsigned long)percent / 100;

	// Not sure if the timer will trigger if we use 0 as delay.
	// Input validation: if percent>100 delay is negative and will overflow into a very large value.
	if(delay<4 || percent>100) {
		delay = 4;
	}
	DimmerTriggerDelay = delay;
}

void lights_printACfrequencyOnLCD() {
	Time time, lastTime;
	char printString[64] = {0};

	ZeroCrossingCount = 0;
	Time_get(&lastTime);

	while(1) {
		Time_get(&time);
		if(time.second != lastTime.second) {
			sprintf(printString, "%u %u   ", ZeroCrossingCount, time.second);
			LCD_writeText(printString, 0, 0);
			lastTime = time;
			ZeroCrossingCount = 0;
		}
	}
}
