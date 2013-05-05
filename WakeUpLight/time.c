/*
 * time.c
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"
#include "lcd44780_LP.h"
#include "commons.h"
#include "time.h"

void Time_init() {
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_32_RTC);
	//ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, 604800);
	ROM_TimerEnable(TIMER0_BASE, TIMER_A);
}

void Time_set(Time *time) {
	time->rawTime = time->second + 60*time->minute + 3600*time->hour + 86400*time->day;
	ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, time->rawTime);
}

void Time_get(Time *time) {
	unsigned long rawTime = ROM_TimerValueGet(TIMER0_BASE, TIMER_A);
	time->rawTime = rawTime;

	time->seconds = rawTime % 60;
	rawTime /= 60;

	time->minute = rawTime % 60;
	rawTime /= 60;

	time->hour = rawTime % 24;
	rawTime /= 24;

	time->day = rawTime % 7;
}
