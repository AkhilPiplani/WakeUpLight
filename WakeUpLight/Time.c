/*
 * Time.c
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#include <stdio.h>
#include "inc/hw_types.h"
#include "inc/hw_memmap.h"
#include "inc/hw_hibernate.h"
#include "driverlib/sysctl.h"
#include "driverlib/hibernate.h"
#include "driverlib/rom.h"
#include "lcd44780_LP.h"
#include "Time.h"

#define SECONDS_IN_A_WEEK	604800
#define SECONDS_IN_A_DAY	86400

// Day number close to the time when the RTC will reach 0xFFFFFFFF and roll over to zero.
// Although this will happen in ~136 years, it's fun to account for that too.
// If this RTC lives 136 years, it won't get a Y2K bug.
// When Time_get is called on that day, it will account for this bug.
#define ROLLOVER_DAY		49710

static char CurrentTimeString[64] = {0};
static Time CurrentTime =  {0}, LastTime = {0};

void Time_init() {
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
	ROM_HibernateEnableExpClk(ROM_SysCtlClockGet());
	ROM_HibernateClockSelect(HIBERNATE_CLOCK_SEL_RAW);
	ROM_HibernateRTCSet(0);
	ROM_HibernateRTCEnable();
}

void Time_set(Time *time) {
	time->rawTime = time->second + 60*time->minute + 3600*time->hour + SECONDS_IN_A_DAY*time->day;
	Time_setRaw(time->rawTime);
}

void Time_setRaw(unsigned long rawTime) {
	volatile unsigned long setTime;
	do {
		ROM_HibernateRTCSet(rawTime);
		setTime = ROM_HibernateRTCGet();
	} while(setTime - rawTime > 3);
}

void Time_get(Time *time) {
	volatile unsigned long oldRawTime = ROM_HibernateRTCGet();
	volatile unsigned long rawTime = ROM_HibernateRTCGet();

	while(rawTime != oldRawTime) {
		oldRawTime = rawTime;
		rawTime = ROM_HibernateRTCGet();
	}

	time->rawTime = rawTime;

	time->second = rawTime % 60;
	rawTime /= 60;

	time->minute = rawTime % 60;
	rawTime /= 60;

	time->hour = rawTime % 24;
	rawTime /= 24;

	time->day = (Day)(rawTime % 7);

	if(rawTime >= ROLLOVER_DAY) {
		rawTime = ROM_HibernateRTCGet();
		Time_setRaw(rawTime % SECONDS_IN_A_WEEK);
	}
}

static void uCharToSting(unsigned char num, char *strEnd) {
	while(num > 0) {
		*strEnd = num%10 + '0';
		strEnd--;
		num /= 10;
	}
}

static void buildCurrentTimeString() {
	char blankTimeString[] = "   0:00:00,    ";
	memcpy(CurrentTimeString, blankTimeString, sizeof(blankTimeString));

	uCharToSting(CurrentTime.hour, CurrentTimeString+3);
	uCharToSting(CurrentTime.minute, CurrentTimeString+6);
	uCharToSting(CurrentTime.second, CurrentTimeString+9);

	switch(CurrentTime.day) {
	case monday:
		CurrentTimeString[12] = 'M';
		break;
	case tuesday:
		CurrentTimeString[12] = 'T';
		CurrentTimeString[13] = 'u';
		break;
	case wednesday:
		CurrentTimeString[12] = 'W';
		break;
	case thursday:
		CurrentTimeString[12] = 'T';
		CurrentTimeString[13] = 'h';
		break;
	case friday:
		CurrentTimeString[12] = 'F';
		break;
	case saturday:
		CurrentTimeString[12] = 'S';
		CurrentTimeString[13] = 'a';
		break;
	case sunday:
		CurrentTimeString[12] = 'S';
		CurrentTimeString[13] = 'u';
		break;
	default:
		break;
	}
}

void Time_printCurrentOnLCD() {
	Time_get(&CurrentTime);

	if(CurrentTime.rawTime != LastTime.rawTime) {
		buildCurrentTimeString();
		LCD_writeText(CurrentTimeString, 0, 0);
		LastTime.rawTime = CurrentTime.rawTime;
	}
}
