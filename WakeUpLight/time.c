/*
 * time.c
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#include <stdio.h>
#include <string.h>
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <inc/hw_hibernate.h>
#include <driverlib/sysctl.h>
#include <driverlib/hibernate.h>
#include <driverlib/rom.h>
#include "lcd44780_LP.h"
#include "time.h"

#define SECONDS_IN_A_WEEK	604800
#define SECONDS_IN_A_DAY	86400

// Day number close to the time when the RTC will reach 0xFFFFFFFF and roll over to zero.
// Although this will happen in ~136 years, it's fun to account for that too.
// If this RTC lives 136 years, it won't get a Y2K bug.
// When time_get is called on that day, it will account for this bug.
#define ROLLOVER_DAY		49710

#define MAX_NUMBER_OF_ALARMS	7

static Time AlarmTimes[MAX_NUMBER_OF_ALARMS] = {{0}};
static unsigned int NumberOfAlarms = 0;
static tBoolean AlarmAcknowledged[MAX_NUMBER_OF_ALARMS];

void time_init() {
	unsigned int i;

	for(i=0; i<sizeof(AlarmAcknowledged); i++) {
		AlarmAcknowledged[i] = true;
	}

	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_HIBERNATE);
	ROM_HibernateEnableExpClk(ROM_SysCtlClockGet());
	ROM_HibernateClockSelect(HIBERNATE_CLOCK_SEL_RAW);
	ROM_HibernateRTCSet(0);
	ROM_HibernateRTCEnable();
}

void time_set(Time *time) {
	time->rawTime = time->second + 60*time->minute + 3600*time->hour + SECONDS_IN_A_DAY*time->day;
	time_setRaw(time->rawTime);
}

void time_setRaw(unsigned long rawTime) {
	volatile unsigned long setTime;
	do {
		ROM_HibernateRTCSet(rawTime);
		setTime = ROM_HibernateRTCGet();
	} while(setTime - rawTime > 3);
}

void time_get(Time *time) {
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
		time_setRaw(rawTime % SECONDS_IN_A_WEEK);
	}
}

static void uCharToSting(unsigned char num, char *strEnd) {
	while(num > 0) {
		*strEnd = num%10 + '0';
		strEnd--;
		num /= 10;
	}
}

static void buildTimeString(Time *time, char *timeString) {
	char blankTimeString[] = "   0:00:00,    ";
	memcpy(timeString, blankTimeString, sizeof(blankTimeString));

	uCharToSting(time->hour, timeString+3);
	uCharToSting(time->minute, timeString+6);
	uCharToSting(time->second, timeString+9);

	switch(time->day) {
	case monday:
		timeString[12] = 'M';
		break;
	case tuesday:
		timeString[12] = 'T';
		timeString[13] = 'u';
		break;
	case wednesday:
		timeString[12] = 'W';
		break;
	case thursday:
		timeString[12] = 'T';
		timeString[13] = 'h';
		break;
	case friday:
		timeString[12] = 'F';
		break;
	case saturday:
		timeString[12] = 'S';
		timeString[13] = 'a';
		break;
	case sunday:
		timeString[12] = 'S';
		timeString[13] = 'u';
		break;
	default:
		break;
	}
}

void time_printCurrentOnLCD() {
	static Time currentTime = {0}, lastTime = {0};
	static char currentTimeString[64] = {0};

	time_get(&currentTime);

	if(currentTime.rawTime != lastTime.rawTime) {
		buildTimeString(&currentTime, currentTimeString);
		lcd_writeText(currentTimeString, 0, 0);
		lastTime.rawTime = currentTime.rawTime;
	}
}

int time_setAlarms(Time *time, unsigned int numberOfAlarms) {
	unsigned int i;

	if(time==NULL || numberOfAlarms>MAX_NUMBER_OF_ALARMS) {
		return -1;
	}

	memcpy(AlarmTimes, time, numberOfAlarms*sizeof(Time));
	for(i=0; i<numberOfAlarms; i++) {
		if(AlarmTimes[i].rawTime == 0) {
			AlarmTimes[i].rawTime = AlarmTimes[i].second + 60*AlarmTimes[i].minute + 3600*AlarmTimes[i].hour + SECONDS_IN_A_DAY*AlarmTimes[i].day;
		}
	}

	NumberOfAlarms = numberOfAlarms;

	return 0;
}

tBoolean time_checkAlarm() {
	static Time currentTime = {0};
	unsigned int i;

	time_get(&currentTime);

	for(i=0; i<NumberOfAlarms; i++) {
		if((AlarmTimes[i].rawTime - currentTime.rawTime < 11) && AlarmAcknowledged[i]==true) {
			AlarmAcknowledged[i] = false;
			return true;
		}
	}

	return false;
}

void time_acknowledgeAlarm() {
	unsigned int i;

	for(i=0; i<NumberOfAlarms; i++) {
		AlarmAcknowledged[i] = true;
	}
}
