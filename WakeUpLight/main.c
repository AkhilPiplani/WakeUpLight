/*
 * main.c
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
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
#include <driverlib/fpu.h>
#include "lcd44780_LP.h"
#include "time.h"
#include "buttons.h"
#include "uartBt.h"
#include "lights.h"
#include "sound.h"

// Alarm-Sound
// PF2(T1CCP0)/PF3(T1CCP1) can be used as 16-bit PWM for Alarm-Sound(PWM). It's in use for the RGB-LED for the Blue/Green channel. Maybe R11/R12 needs to be removed to use it properly.
// PD2 (WT3CCP0) can also be used as 32-bit timer PWM.
// PB2 and PB3 are T3CCP0, T3CCP1 and can be used as 16/32 bit timer PWM.

// Button Pins: PA2-PA6
// Dimmer(lights) Pins: PE1-2
// LCD Pins: PB0,1,4-7
// UART(2) Pins: PD6-7 (PD6=U2Rx, PD7=U2Tx) -- used for uart to bluetooth converter
// PB2 is used for PWM using T3CCP0.

#define ENABLE_TESTS			0
// Enable only one of the below
#define AC_FREQUENCY_TEST		0
#define LIGHTS_TEST				0
#define BUTTONS_TEST			0
#define UARTBT_LOOPBACK_TEST	0
#define UARTBT_ECHO_TEST		0
#define SOUND_TEST				1

static void uartDebug_init() {
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
    ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
    ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

#if ENABLE_TESTS
static void performTests() {
	unsigned long brightness = 0, rxSize = 0;
	char stringBuffer[128] = {0};
	char helloBluetooth[] = "hello bluetooth! \r\n";
	char hi[] = "hoi";
	Time testTime = {sunday, 23, 59, 45, 0};

	time_set(&testTime);

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

		sprintf(stringBuffer, "%lu   ", brightness);
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
#elif SOUND_TEST
		sound_init();
		while(1);
		ROM_SysCtlDelay(ROM_SysCtlClockGet()); // Each SysCtlDelay is about 3 clocks.
		sound_stop();
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 2);
#else
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 100);
		time_printCurrentOnLCD();
#endif
	}
}
#endif

static void initSystem() {
	ROM_FPUStackingDisable(); // Disable the Lazy Stacking of FPU registers. This reduces ISR latency but makes using FPU in ISR dangerous.
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 400MHz / 2 / 2.5 (SYSCTL_SYSDIV_2_5) = 80MHz

    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 1000); // 1mS period of Sys-Tick interrupt.
    ROM_SysTickEnable();
    ROM_IntMasterEnable();

    uartDebug_init(); // Used for printf.
    uartBt_init(); // Used for bluetooth communication with Android App.
    lcd_init();
    buttons_init();
	time_init();
	lights_init();
	ROM_IntMasterEnable();
}

typedef struct __attribute__ ((__packed__)) _AlarmGetSet {
	unsigned char commandByte;
	unsigned long numberOfAlarms;
	unsigned long alarms[7];
} AlarmSet;

#define ALARM_BRIGHTNESS_DELAY 		0
#define ALARM_BRIGHTNESS_INCREMENT	1

int main(void) {
	unsigned char command[UARTBT_MAX_COMMAND_SIZE]  = {0};
	unsigned char response[UARTBT_MAX_COMMAND_SIZE] = {0};
	Time tempTime;
	unsigned long tempUlong, alarmLightBrightness = 0, alarmBrightnessDelay = 0;
	AlarmSet *alarmsSet = (AlarmSet *)command;

	initSystem();

#if ENABLE_TESTS
	performTests();
#endif

	if(uartBt_receive(command) != 0) {
		switch(command[0]) { // Set command bytes are Capitalized, get are not.
		case 't': // get Time
			time_get(&tempTime);
			uartBt_send((unsigned char *)&tempTime, sizeof(tempTime));
			break;
		case 'T': // Set Time
			memcpy((unsigned char *)&(tempTime.rawTime), &command[1], sizeof(tempTime.rawTime));
			time_set(&tempTime);
			break;
		case 'a': // get Alarms
			time_getRawAlarms((unsigned long*)&response[sizeof(unsigned long)], (unsigned long*)&response[0]);
			uartBt_send(response, *((unsigned long*)response));
			break;
		case 'A': // Set Alarms
			time_setRawAlarms((unsigned long *)(&(alarmsSet->alarms)), alarmsSet->numberOfAlarms);
			break;
		case 'L': // Lights
			memcpy(&tempUlong, &command[1], sizeof(tempUlong));
			lights_setBrightness(tempUlong);
			break;
		case 'z': // Snoozzzzze Alarm
			time_get(&tempTime);
			time_setSnoozeAlarm(tempTime.rawTime + 10*60);
			lights_setBrightness(0);
			alarmLightBrightness = 0;
			sound_stop();
			break;
		case 'U': // I'm Up! Stop Alarm
			lights_setBrightness(0);
			alarmLightBrightness = 0;
			sound_stop();
			time_clearSnoozeAlarm();
			break;
		default:
			break;
		}
	}

	if(alarmLightBrightness!=0 && alarmLightBrightness<0xFFFFFFFF) {
		if(alarmBrightnessDelay != 0) {
			alarmBrightnessDelay--;
		}
		else {
			alarmBrightnessDelay = ALARM_BRIGHTNESS_DELAY;
			alarmLightBrightness += ALARM_BRIGHTNESS_INCREMENT;
			lights_setBrightness(alarmLightBrightness);
		}
	}

	if(time_checkAlarm() != false) {
		alarmLightBrightness = 1;
		lights_setBrightness(alarmLightBrightness);
		sound_play();
	}

	return 0;
}
