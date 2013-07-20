/*
 * main.c
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/rom.h>
#include <driverlib/uart.h>
#include <driverlib/gpio.h>
#include <utils/uartstdio.h>
#include "lcd44780_LP.h"
#include "time.h"
#include "buttons.h"
#include "uartBt.h"
#include "lights.h"
#include "alarm.h"

// Alarm-Sound
// PF2(T1CCP0)/PF3(T1CCP1) can be used as 16-bit PWM for Alarm-Sound(PWM). It's in use for the RGB-LED for the Blue/Green channel. Maybe R11/R12 needs to be removed to use it properly.
// PD2 (WT3CCP0) can also be used as 32-bit timer PWM.
// PB2 and PB3 are T3CCP0, T3CCP1 and can be used as 16/32 bit timer PWM.

// Button Pins: PA2-PA6
// Dimmer(lights) Pins: PE1-2
// LCD Pins: PB0,1,4-7
// UART(2) Pins: PD6-7 (PD6=U2Rx, PD7=U2Tx) -- used for uart to bluetooth converter
// PB2 is used for PWM using T3CCP0.

// Enable only one of the below
#define AC_FREQUENCY_TEST		0
#define LIGHTS_TEST				0
#define BUTTONS_TEST			0
#define UARTBT_LOOPBACK_TEST	0
#define UARTBT_ECHO_TEST		0
#define ALARM_TEST				1

int main(void) {
	unsigned long brightness = 0, rxSize = 0;
	char stringBuffer[128] = {0};
	char helloBluetooth[] = "hello bluetooth! \r\n";
	char hi[] = "hoi";
	Time time = {sunday, 23, 59, 45, 0};

	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 400MHz / 2 / 5 (SYSCTL_SYSDIV_5) = 40MHz

    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 1000); // 1mS period of Sys-Tick interrupt.
    ROM_SysTickEnable();
    ROM_IntMasterEnable();

    lcd_init();
    buttons_init();
    uartBt_init();

	time_init();
	time_set(&time);
	lights_init();
	ROM_IntMasterEnable();


#if AC_FREQUENCY_TEST
	lights_printACfrequencyOnLCD();
#endif

	while(1) {
#if LIGHTS_TEST
		ROM_SysCtlDelay(ROM_SysCtlClockGet()/64);
		lights_setBrightness(brightness);
		brightness += 600;
		if(brightness > lights_MaxBrightness+2000) {
			brightness = 0;
		}

		sprintf(stringBuffer, "%u   ", brightness);
		lcd_writeText(stringBuffer, 0, 0);
#elif UARTBT_LOOPBACK_TEST
		// This test needs Rx and Tx pins to be shorted.
		uartBt_send((unsigned char *)helloBluetooth, (unsigned long)strlen(helloBluetooth));
		rxSize = uartBt_receive((unsigned char*)stringBuffer);
		if(rxSize != 0) {
			if(memcmp(stringBuffer, helloBluetooth, strlen(helloBluetooth)-3) != 0) {
				brightness++; // error state
			}
		}
#elif UARTBT_ECHO_TEST
		// This test needs Rx and Tx pins to be shorted.
		rxSize = uartBt_receive((unsigned char*)stringBuffer);
		if(rxSize != 0) {
			uartBt_send((unsigned char*)stringBuffer, (unsigned long)strlen(stringBuffer));
			uartBt_send((unsigned char*)"\r\n", 2);
			memset(stringBuffer, 0, rxSize);
		}
#elif BUTTONS_TEST
		buttons_poll();
		sprintf(stringBuffer, "%d%d%d %d%d%d", Buttons_States[0], Buttons_States[1], Buttons_States[2], Buttons_States[3], Buttons_States[4], Buttons_States[5]);
		lcd_writeText(stringBuffer, 0, 0);
#elif ALARM_TEST
		alarm_init();
		ROM_SysCtlDelay(ROM_SysCtlClockGet()); // Each SysCtlDelay is about 3 clocks.
		alarm_stop();
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 2);
#else
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 100);
		time_printCurrentOnLCD();
#endif
	}
}
