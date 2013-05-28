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

static unsigned int ZeroCrossingCount;
static tBoolean DimmerPinState = false;
static unsigned long DimmerTriggerDelay;
static unsigned long AChalfCycleTime;

void ISR_lights(void) {
	// Not using ROM_ functions here because they are slower.
	GPIOPinIntClear(LIGHTS_PORT, 0xFF); // Clear the interrupt at GPIO
	ZeroCrossingCount++; // Used to calculate zero-crossing frequency

	GPIOPinWrite(LIGHTS_PORT, LIGHTS_DIMMER_OUT, 0);

	if(DimmerTriggerDelay < 20) {
		// If the delay is very low, simply set the dimmer output pin here instead of using the timer
		GPIOPinWrite(LIGHTS_PORT, LIGHTS_DIMMER_OUT, LIGHTS_DIMMER_OUT);
	}
	else if(DimmerTriggerDelay < AChalfCycleTime) {
		// Only set the timer if the brightness is not 0 percent.
		GPIOPinWrite(LIGHTS_PORT, LIGHTS_DIMMER_OUT, 0);
		TimerLoadSet(LIGHTS_TIMER, TIMER_A, DimmerTriggerDelay);
		TimerEnable(LIGHTS_TIMER, TIMER_A);
	}
}

void ISR_lightsTimer(void) {
	unsigned long highTime;

	// Not using ROM_ functions here because they are slower.
	TimerIntClear(LIGHTS_TIMER, TIMER_TIMA_TIMEOUT);  // Clear the interrupt at timer
	TimerDisable(LIGHTS_TIMER, TIMER_A);

	if(DimmerPinState == false) {
		// Set the dimmer out pin and set another timer to clear it.
		// This makes sure the dimmer trigger is switched off irrespective of whether ISR_lights is triggered or not.
		highTime = AChalfCycleTime - DimmerTriggerDelay;
		if(highTime > 35) {
			highTime -= 20;
		}

		DimmerPinState = true;

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
	ZeroCrossingCount = 0;
	AChalfCycleTime = (ROM_SysCtlClockGet()/100);
	DimmerTriggerDelay = AChalfCycleTime;

	ROM_SysCtlPeripheralEnable(LIGHTS_TIMERENABLE);
	ROM_TimerDisable(LIGHTS_TIMER, TIMER_A);
	ROM_TimerConfigure(LIGHTS_TIMER, TIMER_CFG_ONE_SHOT);
	ROM_TimerIntEnable(LIGHTS_TIMER, TIMER_TIMA_TIMEOUT);
	ROM_IntEnable(LIGHTS_TIMER_INTERRUPT);

	ROM_SysCtlPeripheralEnable(LIGHTS_PORTENABLE);
	ROM_GPIOPinTypeGPIOOutput(LIGHTS_PORT, LIGHTS_DIMMER_OUT);
	ROM_GPIOPadConfigSet(LIGHTS_PORT, LIGHTS_DIMMER_OUT, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	ROM_GPIOPinTypeGPIOInput(LIGHTS_PORT, LIGHTS_0CROSSING_IN);
	ROM_GPIOIntTypeSet(LIGHTS_PORT, LIGHTS_0CROSSING_IN, GPIO_RISING_EDGE);
	ROM_GPIOPinIntDisable(LIGHTS_PORT, 0xFF);
	ROM_GPIOPinIntEnable(LIGHTS_PORT, LIGHTS_0CROSSING_IN);
	ROM_IntEnable(LIGHTS_0CROSSING_INTERRUPT);

}
char printBuffer[64] = {0};
void lights_setBrightness(unsigned char percent) {
	// Input validation.
	if(percent > 100) {
		percent = 100;
	}

	DimmerTriggerDelay = AChalfCycleTime - AChalfCycleTime * (unsigned long)percent / 100;
}

void lights_printACfrequencyOnLCD() {
	Time time, lastTime;
	char printString[64] = {0};

	ZeroCrossingCount = 0;
	time_get(&lastTime);

	while(1) {
		time_get(&time);
		if(time.second != lastTime.second) {
			sprintf(printString, "%u %u   ", ZeroCrossingCount, time.second);
			lcd_writeText(printString, 0, 0);
			lastTime = time;
			ZeroCrossingCount = 0;
		}
	}
}
