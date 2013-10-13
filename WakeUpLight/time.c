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
static Time SnoozeAlarmTime = {0};
static unsigned long NumberOfAlarms = 0;
static tBoolean SnoozeAlarmSet = false;

void time_init() {
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
	case sunday:
		timeString[12] = 'S';
		timeString[13] = 'u';
		break;
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
	default:
		break;
	}
}

void time_printCurrent() {
	static Time currentTime = {0}, lastTime = {0};
	static char currentTimeString[64] = {0};

	time_get(&currentTime);

	if(currentTime.rawTime != lastTime.rawTime) {
		buildTimeString(&currentTime, currentTimeString);
		printf("%s \n\r", currentTimeString);
		lastTime.rawTime = currentTime.rawTime;
	}
}

int time_setAlarms(Time *time, unsigned long numberOfAlarms) {
	unsigned long i;

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

int time_setRawAlarms(unsigned long *rawAlarms, unsigned long numberOfAlarms) {
	unsigned long i;

	if(rawAlarms==NULL || numberOfAlarms>MAX_NUMBER_OF_ALARMS) {
		return -1;
	}

	for(i=0; i<numberOfAlarms; i++) {
		AlarmTimes[i].rawTime = rawAlarms[i];
	}

	NumberOfAlarms = numberOfAlarms;

	return 0;
}

int time_getRawAlarms(unsigned long *rawAlarms, unsigned long *numberOfAlarms) {
	unsigned int i;

	if(rawAlarms==NULL || numberOfAlarms==NULL) {
		return -1;
	}

	for(i=0; i<NumberOfAlarms; i++) {
		rawAlarms[i] = AlarmTimes[i].rawTime;
	}

	*numberOfAlarms = NumberOfAlarms;

	return 0;
}

void time_setSnoozeAlarm(unsigned long rawSnoozeAlarm) {
	SnoozeAlarmTime.rawTime = rawSnoozeAlarm;
	SnoozeAlarmSet = true;
}

void time_clearSnoozeAlarm() {
	SnoozeAlarmSet = false;
}

tBoolean time_checkAlarm() {
	static Time currentTime = {0};
	unsigned int i;

	time_get(&currentTime);

	for(i=0; i<NumberOfAlarms; i++) {
		if(AlarmTimes[i].rawTime - currentTime.rawTime < 2) {
			return true;
		}
	}

	if(SnoozeAlarmSet!=false && (SnoozeAlarmTime.rawTime - currentTime.rawTime < 2)) {
		return true;
	}

	return false;
}
