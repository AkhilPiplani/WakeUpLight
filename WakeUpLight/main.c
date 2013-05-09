#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/timer.h"
#include "driverlib/rom.h"
#include "utils/uartstdio.h"
#include "lcd44780_LP.h"
#include "commons.h"
#include "time.h"

char PrintString[64] = {0};

unsigned char FillLed(unsigned int h,unsigned int j);

int main(void) {
	Time time = {sunday, 23, 59, 45, 0};

	ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN); // 400MHz / 2 / 5 (SYSCTL_SYSDIV_5) = 40MHz

	//ROM_IntMasterEnable();

    LCD_init();

	Time_init();
	Time_set(&time);

	while(1) {
		ROM_SysCtlDelay(ROM_SysCtlClockGet()/10);
		Time_printCurrentOnLCD();
	}
}
