/*
 * main.c
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inc/hw_ints.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/pin_map.h>
#include <driverlib/sysctl.h>
#include <driverlib/systick.h>
#include <driverlib/interrupt.h>
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

		printf("%lu \r\n", brightness);
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
		printf("%lu \r\n", buttons_poll());
#else
		buttons_poll();
		printf("%d%d%d %d%d%d \r\n", Buttons_States[0], Buttons_States[1], Buttons_States[2], Buttons_States[3], Buttons_States[4], Buttons_States[5]);
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


#define SYSTICK_FREQUENCY_HZ	10
void ISR_sysTick();

static void systick_init() {
	ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICK_FREQUENCY_HZ); // 10Hz frequency of Sys-Tick interrupt.
	ROM_SysTickEnable();

	// Interrupt priorities are from 0(highest) to 7(lowest).
	// The register only uses the top 3 bits of a byte so shifted up by 5.
	// Default priority for an interrupt is 0(highest).
	IntPrioritySet(FAULT_SYSTICK, 7<<5);

	SysTickIntRegister(ISR_sysTick);
	ROM_SysTickIntEnable();
}

static void initSystem() {
	ROM_FPUStackingDisable(); // Disable the Lazy Stacking of FPU registers. This reduces ISR latency but makes using FPU in ISR dangerous.
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 400MHz / 2 / 2.5 (SYSCTL_SYSDIV_2_5) = 80MHz

	ROM_IntMasterEnable();

	systick_init(); // periodic interrupt used for timing such as slowly increasing the light-brightness for alarm.
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

#define ALARM_SOUND_TIMEOUT_SECONDS		600
#define ALARM_LIGHT_TIMEOUT_SECONDS		1800

typedef enum _AlarmStatus {
	AlarmStatus_off = 0,
	AlarmStatus_snoozed,
	AlarmStatus_lightsOn,
	AlarmStatus_playingSound,
	AlarmStatus_soundTimedOut,
} AlarmStatus;

// The brightness levels are not linear with lights-delays.
// The intermediate values in this enum give a good range of mood-lighting.
typedef enum _ButtonLightBrigthness {
	buttonLightBrightness_off = 0,
	buttonLightBrightness_full,
	buttonLightBrightness_60percent,
	buttonLightBrightness_45percent,
	buttonLightBrightness_37percent
} ButtonLightBrigthness;

static unsigned long AlarmLightBrightness = 0, AlarmLightMaxBrightness = 0xFFFFFFFF, AlarmLightIncrement = 1;
static Time TempTime;
AlarmStatus AlarmState = AlarmStatus_off;

volatile unsigned long TickCount = 0;

void ISR_sysTick() {
	TickCount++;
	if(AlarmState!=AlarmStatus_off && AlarmLightBrightness<=AlarmLightMaxBrightness) {
		AlarmLightBrightness += AlarmLightIncrement;
		lights_setBrightness(AlarmLightBrightness);
	}
}

static void snoozeAlarm() {
	printf("snoozeAlarm\r\n");

	if(AlarmState == AlarmStatus_off) {
		lights_setBrightness(0);
		AlarmLightBrightness = 0;
	}
	else {
		time_get(&TempTime);
		time_setSnoozeAlarm(TempTime.rawTime + 10*60);
		// lights_setBrightness(0);
		// AlarmLightBrightness = 0;
		AlarmState = AlarmStatus_snoozed;
	}

	sound_stop();
}

static void stopAlarm() {
	printf("stopAlarm\r\n");
	lights_setBrightness(0);
	AlarmLightBrightness = 0;
	sound_stop();
	AlarmState = AlarmStatus_off;
	time_clearSnoozeAlarm();
}

static void calculateAlarmLightIncrement(unsigned char alarmLightTimeToMax) {
	AlarmLightIncrement = AlarmLightMaxBrightness / ((unsigned long)alarmLightTimeToMax * 60 * SYSTICK_FREQUENCY_HZ);

	if(AlarmLightIncrement == 0) { // Round up when necessary.
		AlarmLightIncrement = 1;
	}
}

int main(void) {
	char command[UARTBT_MAX_COMMAND_SIZE]  = {0};
	char response[UARTBT_MAX_COMMAND_SIZE] = {0};
	char tempString[UARTBT_MAX_COMMAND_SIZE] = {0};
	unsigned char tempUchar;
	unsigned char alarmLightTimeToMax = 15; // in minutes
	unsigned int offset, i;
	unsigned long numberOfAlarms, echoCount = 0, lightBrightness, tempUlong;
	Time alarms[7], alarmSoundStartTime;
	ButtonLightBrigthness buttonLightBrightness = buttonLightBrightness_off;

	initSystem();

	// For now, use the maximum-lights-brightness for the alarm, this can be user-settable in future.
	AlarmLightMaxBrightness = lights_MaxBrightness;
	lightBrightness = lights_MaxBrightness;
	calculateAlarmLightIncrement(alarmLightTimeToMax);

#ifdef __OPTIMIZE__
	printf("\r\n -- Compiled on %s %s in Release mode --\r\n", __DATE__, __TIME__);
#else
	printf("\r\n -- Compiled on %s %s in Debug mode --\r\n",   __DATE__, __TIME__);
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
			case 't': // get Time -- not used by Android App yet.
				time_get(&TempTime);
				sprintf(response, "%hhu:%02hhu:%02hhu:%02hhu\r\n", TempTime.day, TempTime.hour, TempTime.minute, TempTime.second);
				uartBt_send((unsigned char*)response, strlen(response));
				break;

			case 'T': // Set Time
				printf("Setting Time\r\n");
				sscanf(&command[1], "%hhu:%02hhu:%02hhu:%02hhu\r\n", &(TempTime.day), &(TempTime.hour), &(TempTime.minute), &(TempTime.second));
				time_set(&TempTime);
				break;

			case 'a': // get Alarms  -- not used by Android App yet.
				memset(response, 0, sizeof(response));
				time_getAlarms(alarms, &numberOfAlarms);

				for(i=0; i<7; i++) {
					if(i < numberOfAlarms) {
						sprintf(response + strlen(response), "%hhu:%02hhu:%02hhu:%02hhu,", alarms[i].day, alarms[i].hour, alarms[i].minute, alarms[i].second);
					}
					else {
						sprintf(response + strlen(response), "7:00:00:00,");
					}
				}
				sprintf(response + strlen(response) - 1, "\r\n"); // -1 to overwrite the last comma.

				uartBt_send((unsigned char*)response, strlen(response));
				break;

			case 'A': // Set Alarms
				printf("Setting Alarm\r\n");
				// We always receive 7 time-values from the Android app in the same format as "Set Time".
				// The app will set an invalid time-value in the alarm-slot it want's to be Off. This is checked using the day-field.
				offset = 0;
				numberOfAlarms = 0;

				// Parse 7 time-values in the format day:hour:minute:seconds separated by commas ','.
				for(i=0; i<7; i++) {
					sscanf(&command[1] + offset, "%[^,]", tempString);
					printf("Alarm String %d = %s\r\n", i, tempString);
					sscanf(tempString, "%hhu:%02hhu:%02hhu:%02hhu", &(alarms[numberOfAlarms].day), &(alarms[numberOfAlarms].hour), &(alarms[numberOfAlarms].minute), &(alarms[numberOfAlarms].second));
					printf("%lu : %hhu, %hhu, %hhu, %hhu\r\n", numberOfAlarms, alarms[numberOfAlarms].day, alarms[numberOfAlarms].hour, alarms[numberOfAlarms].minute, alarms[numberOfAlarms].second);
					if(alarms[numberOfAlarms].day <= saturday) { // Ignore the alarm if the day-value is out-of-range
						numberOfAlarms++;
					}
					offset += strlen(tempString) + 1;
				}

				time_printCurrent();
				time_setAlarms(alarms, numberOfAlarms);
				break;

			case 'B': // Set maximum alarm-Brightness and delay
				sscanf(&command[1], "%hhu,%hhu\r\n", &tempUchar, &alarmLightTimeToMax); // App sends a brightness percentage (0-100).
				AlarmLightMaxBrightness = (unsigned long)tempUchar * (lights_MaxBrightness / 100);
				calculateAlarmLightIncrement(alarmLightTimeToMax);
				break;

			case 'L': // Lights
				sscanf(&command[1], "%hhu\r\n", &tempUchar); // App sends a brightness percentage (0-100).
				lightBrightness = (unsigned long)tempUchar * (lights_MaxBrightness / 100);
				lights_setBrightness(lightBrightness);
				printf("Received percentage = %hhu, setting brightness = %lu \r\n", tempUchar, lightBrightness);
				break;

			case 'z': // Snoozzzzze Alarm -- not used by Android App yet.
				snoozeAlarm();
				break;

			case 'U': // I'm Up! Stop Alarm -- not used by Android App yet.
				stopAlarm();
				break;

			case 'd': // demo mode.
				lightBrightness = 1200 * 4;
				while(lightBrightness < lights_MaxBrightness) {
					ROM_SysCtlDelay(ROM_SysCtlClockGet()/64);
					lights_setBrightness(lightBrightness);
					lightBrightness += 1200;
				}
				
				while(lightBrightness > 0) {
					ROM_SysCtlDelay(ROM_SysCtlClockGet()/128);
					lights_setBrightness(lightBrightness);
					lightBrightness -= 1200;
				}
				
				lights_setBrightness(0);
				break;

			default:
				break;
			}
		}

		if(time_checkAlarm() != false) {
			if(AlarmState == AlarmStatus_off) {
				printf("alarm starting \r\n");
				AlarmState = AlarmStatus_lightsOn;
				AlarmLightBrightness = lights_MaxBrightness / 16;
				lights_setBrightness(AlarmLightBrightness);
			}
			else if(AlarmState == AlarmStatus_snoozed) {
				printf("alarm resuming from snooze \r\n");
				AlarmState = AlarmStatus_lightsOn;
			}
		}

		// Start playing alarm-sound once lights reach full-brightness.
		if(AlarmLightBrightness>=AlarmLightMaxBrightness && AlarmState==AlarmStatus_lightsOn) {
			sound_play();
			time_get(&alarmSoundStartTime);
			AlarmState = AlarmStatus_playingSound;
		}

		// Don't play the alarm forever. If I don't wake-up after 10-minutes of alarm-sound, I'm not home.
		time_get(&TempTime);
		if((TempTime.rawTime - alarmSoundStartTime.rawTime > ALARM_SOUND_TIMEOUT_SECONDS) && AlarmState==AlarmStatus_playingSound) {
			printf("Stopping alarm sound due to timeout\r\n");
			sound_stop();
			AlarmState = AlarmStatus_soundTimedOut;
		}
		else if((TempTime.rawTime - alarmSoundStartTime.rawTime > ALARM_LIGHT_TIMEOUT_SECONDS) && AlarmState==AlarmStatus_soundTimedOut) {
			printf("Stopping alarm completely due to timeout\r\n");
			stopAlarm();
		}

		// Poll the snooze button. If pressed, wait 1-second and poll again.
		// Still-pressed = alarm-off, else snooze.
		// If the alarm is not active, use the button to toggle light on/off
		if((buttons_poll() & BUTTONS_SNOOZE__LIGHT_TOGGLE_PIN) != 0) {

			if(AlarmState != AlarmStatus_off) {
				ROM_SysCtlDelay(ROM_SysCtlClockGet() / 3);  // Each SysCtlDelay is about 3 clocks.
				if((buttons_poll() & BUTTONS_SNOOZE__LIGHT_TOGGLE_PIN) != 0) {
					printf("Stopping alarm due to button\r\n");
					stopAlarm();
				}
				else {
					snoozeAlarm();
				}
			}

			else {
				// Wait 1/10th of a second and poll again.
				ROM_SysCtlDelay(ROM_SysCtlClockGet() / 30);  // Each SysCtlDelay is about 3 clocks.

				if((buttons_poll() & BUTTONS_SNOOZE__LIGHT_TOGGLE_PIN) != 0) {
					switch(buttonLightBrightness) {
					case buttonLightBrightness_full:
						lights_setBrightness(lights_MaxBrightness / 100 * 60);
						buttonLightBrightness = buttonLightBrightness_60percent;
						break;
					case buttonLightBrightness_60percent:
						lights_setBrightness(lights_MaxBrightness / 100 * 45);
						buttonLightBrightness = buttonLightBrightness_45percent;
						break;
					case buttonLightBrightness_45percent:
						lights_setBrightness(lights_MaxBrightness / 100 * 37);
						buttonLightBrightness = buttonLightBrightness_37percent;
						break;
					case buttonLightBrightness_37percent:
						lights_setBrightness(0);
						buttonLightBrightness = buttonLightBrightness_off;
						break;
					case buttonLightBrightness_off:
					default:
						lights_setBrightness(lights_MaxBrightness);
						buttonLightBrightness = buttonLightBrightness_full;
						break;
					}
				}
			}

			// Wait for the button to be released, otherwise the light will keep toggling.
			while((buttons_poll() & BUTTONS_SNOOZE__LIGHT_TOGGLE_PIN) != 0);
		}

		//time_printCurrent();

	}

	return 0;
}
