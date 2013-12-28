/*
 * sound.c
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
#include "sound.h"

#define SOUND_ELEMENTS 		ALARM_SOUND_ELEMENTS
#define SOUND_SAMPLE_RATE	ALARM_SOUND_SAMPLE_RATE
#define SOUND_BITDEPTH		ALARM_SOUND_BITDEPTH

void ISR_soundSamples(void);

static volatile unsigned int SoundIndex = 0;

void sound_init() {
	// Enable the relevant peripherals.
	ROM_SysCtlPeripheralEnable(SOUND_SAMPLERATE_TIMERENABLE);
	ROM_SysCtlPeripheralEnable(SOUND_PWM_TIMERENABLE);
	ROM_SysCtlPeripheralEnable(SOUND_PWM_PORTENABLE);

	// Set-up PWM.
	ROM_GPIOPinConfigure(SOUND_PINMAP_PWM);
	ROM_GPIOPinTypeTimer(SOUND_PWM_PORT, SOUND_PWM_PIN);
	ROM_GPIOPadConfigSet(SOUND_PWM_PORT, SOUND_PWM_PIN, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD);
	// ROM_GPIOPadConfigSet(SOUND_PWM_PORT, SOUND_PWM_PIN, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);

	// Set-up the shutdown pin and set it low.
	ROM_SysCtlPeripheralEnable(SOUND_AMP_ENABLE_PORTENABLE);
	ROM_GPIOPinTypeGPIOOutput(SOUND_AMP_ENABLE_PORT, SOUND_AMP_ENABLE_PIN);
	ROM_GPIOPinTypeGPIOOutput(SOUND_AMP_ENABLE_PORT, 0);
}

void sound_play() {
	SoundIndex = 0;

	// Enable the amplifier
	ROM_GPIOPinWrite(SOUND_AMP_ENABLE_PORT, SOUND_AMP_ENABLE_PIN, SOUND_AMP_ENABLE_PIN);

	ROM_TimerDisable(SOUND_PWM_TIMER, TIMER_A);
	ROM_TimerConfigure(SOUND_PWM_TIMER, TIMER_CFG_SPLIT_PAIR | TIMER_CFG_A_PWM);
	// HWREG(SOUND_PWM_TIMER + TIMER_O_CTL) |= 1<<14; // Enable PWM inversion.
	ROM_TimerLoadSet(SOUND_PWM_TIMER, TIMER_A,  (1<<SOUND_BITDEPTH) - 1);
	ROM_TimerMatchSet(SOUND_PWM_TIMER, TIMER_A, 0); // Initial Duty-Cycle = 0
	ROM_TimerEnable(SOUND_PWM_TIMER, TIMER_A);

	// Set up the sample-rate timer.
	ROM_TimerDisable(SOUND_SAMPLERATE_TIMER, TIMER_A);
	ROM_TimerConfigure(SOUND_SAMPLERATE_TIMER, TIMER_CFG_32_BIT_PER);
	ROM_TimerLoadSet(SOUND_SAMPLERATE_TIMER, TIMER_A, ROM_SysCtlClockGet() / SOUND_SAMPLE_RATE);
	TimerIntRegister(SOUND_SAMPLERATE_TIMER, TIMER_A, ISR_soundSamples);
	ROM_IntEnable(SOUND_SAMPLERATE_TIMER_INTERRUPT);
	ROM_TimerIntEnable(SOUND_SAMPLERATE_TIMER, TIMER_TIMA_TIMEOUT);
	ROM_TimerEnable(SOUND_SAMPLERATE_TIMER, TIMER_A);

	// Synchronize the two timers.
	TimerSynchronize(TIMER0_BASE, SOUND_TIMERS_SYNC); // The first argument must be TIMER0_BASE.
}

void sound_stop() {
	// Disable the sample-rate timer.
	ROM_TimerDisable(SOUND_SAMPLERATE_TIMER, TIMER_A);

	// Disable the PWM timer.
	ROM_TimerMatchSet(SOUND_PWM_TIMER, TIMER_A, 0);
	ROM_TimerDisable(SOUND_PWM_TIMER, TIMER_A);

	// Ensure the PWM pin is set low.
	ROM_GPIOPinTypeGPIOOutput(SOUND_PWM_PORT, SOUND_PWM_PIN);
	ROM_GPIOPinTypeGPIOOutput(SOUND_PWM_PORT, SOUND_PWM_PIN);
	ROM_GPIOPinWrite(SOUND_PWM_PORT, SOUND_PWM_PIN, 0);

	// Disable the amplifier to save power and prevent any speaker noise.
	ROM_GPIOPinWrite(SOUND_AMP_ENABLE_PORT, SOUND_AMP_ENABLE_PIN, 0);

	SoundIndex = 0;
}

void ISR_soundSamples(void) {
	TimerIntClear(SOUND_SAMPLERATE_TIMER, TIMER_TIMA_TIMEOUT);
	TimerMatchSet(SOUND_PWM_TIMER, TIMER_A, AlarmSound[SoundIndex++]);
	if(SoundIndex == SOUND_ELEMENTS) {
		SoundIndex = 0;
	}
}
