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

void Timing_init() {
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_32_RTC);
	ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, 0);
	ROM_TimerEnable(TIMER0_BASE, TIMER_A);
}
