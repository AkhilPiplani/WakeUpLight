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
// UART(2) Pins: PD6-7 (PD6=U2Rx, PD7=U2Tx) -- used for UART to Bluetooth converter
// PB2 is used for sound using T3CCP0 for PWM.
// PC6 is used for sound amplifier power-enable / shutdown.
// PC7 is used for the snooze button.

#define ENABLE_TESTS			0
// Enable only one of the below
#define AC_FREQUENCY_TEST		0
#define LIGHTS_TEST				0
#define BUTTONS_TEST			0
#define UARTBT_LOOPBACK_TEST	0
#define UARTBT_ECHO_TEST		0
#define SOUND_TEST				0

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
	unsigned long brightness = 0, rxSize = 0, echoCount = 0;
	char stringBuffer[128] = {0};
	char helloBluetooth[] = "hello bluetooth! \r\n";
	char hi[] = "hoi";
	Time testTime = {sunday, 23, 59, 45, 0};

	time_set(&testTime);

#if AC_FREQUENCY_TEST
	lights_printACfrequency();
#endif

	while(1) {
#if LIGHTS_TEST
		ROM_SysCtlDelay(ROM_SysCtlClockGet()/64);
		lights_setBrightness(brightness);
		brightness += 600;
		if(brightness > lights_MaxBrightness+2000) {
			brightness = 0;
		}

		printf("%lu \n\r", brightness);
#elif UARTBT_LOOPBACK_TEST
		// This test needs Rx and Tx pins to be shorted
		uartBt_send((unsigned char *)helloBluetooth, (unsigned long)strlen(helloBluetooth));
		rxSize = uartBt_receive((unsigned char*)stringBuffer);
		if(rxSize != 0) {
			if(memcmp(stringBuffer, helloBluetooth, strlen(helloBluetooth)-3) != 0) {
				brightness++; // error state
			}
		}
#elif UARTBT_ECHO_TEST
		rxSize = uartBt_receive((unsigned char*)stringBuffer);
		//uartBt_send((unsigned char*)"hi\r\n", 4);
		if(rxSize != 0) {
			uartBt_send((unsigned char*)stringBuffer, (unsigned long)strlen(stringBuffer));
			sprintf(stringBuffer, "%lu \r\n", echoCount);
			uartBt_send((unsigned char*)stringBuffer, (unsigned long)strlen(stringBuffer));
			memset(stringBuffer, 0, rxSize);
			echoCount++;
		}
#elif BUTTONS_TEST
#if SIMPLIFIED_BUTTONS
		printf("%lu \n\r", buttons_poll());
#else
		buttons_poll();
		printf("%d%d%d %d%d%d \n\r", Buttons_States[0], Buttons_States[1], Buttons_States[2], Buttons_States[3], Buttons_States[4], Buttons_States[5]);
#endif
#elif SOUND_TEST
		sound_init();
		sound_play();
		while(1);
		ROM_SysCtlDelay(ROM_SysCtlClockGet()); // Each SysCtlDelay is about 3 clocks.
		sound_stop();
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 2);
#else
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 100);
		time_printCurrent();
#endif
	}
}
#endif

static void initSystem() {
//	ROM_FPUStackingDisable(); // Disable the Lazy Stacking of FPU registers. This reduces ISR latency but makes using FPU in ISR dangerous.
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 400MHz / 2 / 2.5 (SYSCTL_SYSDIV_2_5) = 80MHz

    ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 1000); // 1mS period of Sys-Tick interrupt.
    ROM_SysTickEnable();
    ROM_IntMasterEnable();

    uartDebug_init(); // Used for printf.
    
    //uartBt_init(9600); 
    //uartBt_oneTimeSetup();
    
    uartBt_init(115200); // Used for bluetooth communication with Android App.
    //lcd_init(); // Not used anymore
    
    buttons_init();
	time_init();
	lights_init();
	sound_init();
	ROM_IntMasterEnable();
}

typedef struct __attribute__ ((__packed__)) _AlarmGetSet {
	unsigned char commandByte;
	unsigned long numberOfAlarms;
	unsigned long alarms[7];
} AlarmSet;

#define ALARM_BRIGHTNESS_DELAY 		0
#define ALARM_BRIGHTNESS_INCREMENT	1

static unsigned long AlarmLightBrightness = 0, AlarmBrightnessDelay = 0, AlarmLightMaxBrightness = 0xFFFFFFFF;
static Time TempTime;

static void snoozeAlarm() {
	time_get(&TempTime);
	time_setSnoozeAlarm(TempTime.rawTime + 10*60);
	lights_setBrightness(0);
	AlarmLightBrightness = 0;
	sound_stop();
}

static void stopAlarm() {
	lights_setBrightness(0);
	AlarmLightBrightness = 0;
	sound_stop();
	time_clearSnoozeAlarm();
}

int main(void) {
	char command[UARTBT_MAX_COMMAND_SIZE]  = {0};
	char response[UARTBT_MAX_COMMAND_SIZE] = {0};
	unsigned char tempUchar;
	unsigned long rawAlarms[7] = {0}, numberOfAlarms, i, echoCount = 0;
	AlarmSet *alarmsSet = (AlarmSet *)command;

	initSystem();

#ifdef __OPTIMIZE__
	printf("\n\r -- Compiled on %s %s in Release mode --\r\n", __DATE__, __TIME__);
#else
	printf("\n\r -- Compiled on %s %s in Debug mode --\r\n",   __DATE__, __TIME__);
#endif

#if ENABLE_TESTS
	performTests();
#endif

	while(1) {
		if(uartBt_receive((unsigned char*)command) != 0) {

				// DEBUG CODE: Echo back the command along with the number of commands sent so far
				uartBt_send((unsigned char*)command, (unsigned long)strlen(command));
				sprintf(response, "%lu \r\n", echoCount);
				uartBt_send((unsigned char*)response, (unsigned long)strlen(response));
				echoCount++;

				switch(command[0]) { // Set command bytes are Capitalized, get are not.
				case 't': // get Time
					time_get(&TempTime);
					sprintf(response, "%d:%02d:%02d:%02d\r\n", TempTime.day, TempTime.hour, TempTime.minute, TempTime.second);
					uartBt_send((unsigned char*)response, strlen(response));
					break;

				case 'T': // Set Time
					sscanf(&command[1], "%d:%02d:%02d:%02d\r\n", &(TempTime.day), &(TempTime.hour), &(TempTime.minute), &(TempTime.second));
					time_set(&TempTime);
					break;

				case 'a': // get Alarms
					time_getRawAlarms(rawAlarms, &numberOfAlarms);

					memset(response, 0, sizeof(response));
					sprintf(response, "%lu", numberOfAlarms);
					for(i=0; i<numberOfAlarms; i++) {
						sprintf(response + strlen(response), "%lu", rawAlarms[i]);
					}
					sprintf(response + strlen(response), "\r\n");

					uartBt_send((unsigned char*)response, strlen(response));
					break;

				case 'A': // Set Alarms
					// We always receive 7 time-values from the Android app in the same format as "Set Time".
					// The app will set an invalid time-value in the alarm-slot it want's to be Off. We check this using the day-field
					// TODO: Write the Set Alarms Case.

					time_setRawAlarms((unsigned long *)(&(alarmsSet->alarms)), alarmsSet->numberOfAlarms);
					break;

				case 'B': // Set Maximum Alarm-Brightness
					sscanf(&command[1], "%d\r\n", (unsigned int*)&tempUchar); // App sends a brightness percentage (0-100).
					AlarmLightMaxBrightness = (unsigned long)tempUchar * (lights_MaxBrightness / 100);
					break;

				case 'L': // Lights
					sscanf(&command[1], "%d\r\n", (unsigned int*)&tempUchar); // App sends a brightness percentage (0-100).
					printf("Received percentage = %d, setting brightness = %lu \r\n", tempUchar, ((unsigned long)tempUchar * (lights_MaxBrightness / 100)));
					lights_setBrightness((unsigned long)tempUchar * (lights_MaxBrightness / 100));
					break;

				case 'z': // Snoozzzzze Alarm
					snoozeAlarm();
					break;

				case 'U': // I'm Up! Stop Alarm
					stopAlarm();
					break;

				default:
					break;
				}
			}

			// Poll the snooze button. If pressed, wait 1-second and poll again.
			// Still-pressed = alarm-off, else snooze.
			if(buttons_poll() != 0) {
				ROM_SysCtlDelay(ROM_SysCtlClockGet() / 3);  // Each SysCtlDelay is about 3 clocks.
				if(buttons_poll() != 0) {
					stopAlarm();
				}
				else {
					snoozeAlarm();
				}
			}

			if(AlarmLightBrightness!=0 && AlarmLightBrightness<=AlarmLightMaxBrightness) {
				if(AlarmBrightnessDelay != 0) {
					AlarmBrightnessDelay--;
				}
				else {
					AlarmBrightnessDelay = ALARM_BRIGHTNESS_DELAY;
					AlarmLightBrightness += ALARM_BRIGHTNESS_INCREMENT;
					lights_setBrightness(AlarmLightBrightness);
				}
			}

			if(time_checkAlarm() != false) {
				AlarmLightBrightness = 1;
				lights_setBrightness(AlarmLightBrightness);
				sound_play();
			}
	}

	return 0;
}
