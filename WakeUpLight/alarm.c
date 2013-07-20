/*
 * alarm.c
 *
 *  Created on: 15-Jul-2013
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
#include "alarmSound.h"
#include "alarm.h"

volatile unsigned int AlarmSoundIndex = 0;

void alarm_init() {
	AlarmSoundIndex = 0;

	// Enable the relevant peripherals
	ROM_SysCtlPeripheralEnable(ALARM_SAMPLERATE_TIMERENABLE);
	ROM_SysCtlPeripheralEnable(ALARM_PWM_TIMERENABLE);
	ROM_SysCtlPeripheralEnable(ALARM_PWM_PORTENABLE);

	// Set-up PWM
	ROM_GPIOPinConfigure(ALARM_PINMAP_PWM);
	ROM_GPIOPinTypeTimer(ALARM_PWM_PORT, ALARM_PWM_PIN);

	ROM_TimerDisable(ALARM_PWM_TIMER, TIMER_A);
	ROM_TimerConfigure(ALARM_PWM_TIMER, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM);
	ROM_TimerLoadSet(ALARM_PWM_TIMER, TIMER_A,  1<<ALARM_SOUND_BITDEPTH);
	ROM_TimerMatchSet(ALARM_PWM_TIMER, TIMER_A, 0); // Initial Duty-Cycle = 0
	ROM_TimerEnable(ALARM_PWM_TIMER, TIMER_A);

	// Set up the sample-rate timer
	ROM_TimerDisable(ALARM_SAMPLERATE_TIMER, TIMER_A);
	ROM_TimerConfigure(ALARM_SAMPLERATE_TIMER, TIMER_CFG_32_BIT_PER);
	ROM_TimerLoadSet(ALARM_SAMPLERATE_TIMER, TIMER_A, ROM_SysCtlClockGet() / ALARM_SOUND_SAMPLE_RATE);
	ROM_IntEnable(ALARM_SAMPLERATE_TIMER_INTERRUPT);
	ROM_TimerIntEnable(ALARM_SAMPLERATE_TIMER, TIMER_TIMA_TIMEOUT);
	ROM_TimerEnable(ALARM_SAMPLERATE_TIMER, TIMER_A);
}

void alarm_stop() {
	// Disable the sample-rate timer
	ROM_TimerDisable(ALARM_SAMPLERATE_TIMER, TIMER_A);

	// Disable the PWM timer.
	ROM_TimerMatchSet(ALARM_PWM_TIMER, TIMER_A, 0);
	ROM_TimerDisable(ALARM_PWM_TIMER, TIMER_A);

	// Ensure the PWM pin is set low.
	ROM_GPIOPinTypeGPIOOutput(ALARM_PWM_PORT, ALARM_PWM_PIN);
	ROM_GPIOPinTypeGPIOOutput(ALARM_PWM_PORT, ALARM_PWM_PIN);
	ROM_GPIOPinWrite(ALARM_PWM_PORT, ALARM_PWM_PIN, 0);
}

// TODO: Write the ISR that loads the next sample value from the array, checks if the alarm-stop-flag is set and stops playing things and also resets index-etc
// TODO: configure the ISR in startup_ccs.c file
void ISR_alarmSamples(void) {
	TimerMatchSet(ALARM_PWM_TIMER, TIMER_A, AlarmSound[AlarmSoundIndex++]);
	if(AlarmSoundIndex == ALARM_SOUND_ELEMENTS) {
		AlarmSoundIndex = 0;
	}
}
