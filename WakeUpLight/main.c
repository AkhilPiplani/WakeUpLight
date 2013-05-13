/*
 * main.c
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/rom.h>
//#include <utils/uartstdio.h>
#include "lcd44780_LP.h"
#include "Time.h"
#include "Buttons.h"

// Alarm-Sound
// PF2(T1CCP0)/PF3(T1CCP1) can be used as 16-bit PWM for Alarm-Sound(PWM). It's in use for the RGB-LED for the Blue/Green channel. Maybe R11/R12 needs to be removed to use it properly.
// PD7 (WT5CCP1) or PD2 (WT3CCP0) can also be used as 32-bit timer PWM.

// Button Pins: PA2-PA6
// Dimmer Pins: PE1-2

tBoolean ButtonStates[BUTTONS_NB_BUTTONS];
char LCDprintBuffer[64] = {0};

void ISR_sysTick() {
	Buttons_poll(ButtonStates);
}

int main(void) {
	Time time = {sunday, 23, 59, 45, 0};

	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 400MHz / 2 / 5 (SYSCTL_SYSDIV_5) = 40MHz


    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 1000); // 1mS period of Sys-Tick interrupt.
    ROM_SysTickEnable();
    //SysTickIntRegister(ISR_sysTick);
    //ROM_SysTickIntEnable();
    //ROM_IntMasterEnable(); // Enable interrupts

    LCD_init();
    if(Buttons_init(ButtonStates) != 0) {
    	while(1);
    }

	Time_init();
	Time_set(&time);

	while(1) {
		//ROM_SysCtlDelay(ROM_SysCtlClockGet() / 1000);
		//Time_printCurrentOnLCD();
		Buttons_poll(ButtonStates);
		sprintf(LCDprintBuffer, "%d%d%d %d%d%d", ButtonStates[0], ButtonStates[1], ButtonStates[2], ButtonStates[3], ButtonStates[4], ButtonStates[5]);
		LCD_writeText(LCDprintBuffer, 0, 0);
		//printf("Button States: %d, %d, %d, %d, %d, %d \n", ButtonStates[0], ButtonStates[1], ButtonStates[2], ButtonStates[3], ButtonStates[4], ButtonStates[5]);
	}
}
