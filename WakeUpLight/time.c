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

	if(ROM_HibernateIsActive() == 0) {
		// OSCDRV bit needs to be set to set high drive strength as the launchpad board has
		// capacitors on the XOSC crystal. This can't be set after clock is enabled so I can't
		// use driverlib functions here.
		HWREG(HIB_CTL) |= HIB_CTL_CLK32EN | HIB_CTL_OSCDRV;

		// Wait the required 1500ms for the crystal oscillator to stabilize.
		ROM_SysCtlDelay(ROM_SysCtlClockGet() / 2);

		// Now set back the default values of the hibernate registers which can get corrupted according to Errata 3.1.
		ROM_HibernateRTCSet(0);
		HibernateRTCMatch0Set(0xFFFFFFFF);
		HibernateRTCSSMatch0Set(0);
		ROM_HibernateRTCTrimSet(0x7FFF); // This one not being set to default is the culprit of 1% RTC drift.
		ROM_HibernateIntDisable(HIBERNATE_INT_PIN_WAKE | HIBERNATE_INT_LOW_BAT | HIBERNATE_INT_RTC_MATCH_0 | HIBERNATE_INT_WR_COMPLETE);
	}

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

static void buildTimeString(Time *time, char *timeString) {
	sprintf(timeString, "   %02hhu:%02hhu:%02hhu, ", time->hour, time->minute, time->second);

	switch(time->day) {
	case sunday:
		strcat(timeString, "Su ");
		break;
	case monday:
		strcat(timeString, "M  ");
		break;
	case tuesday:
		strcat(timeString, "Tu ");
		break;
	case wednesday:
		strcat(timeString, "W  ");
		break;
	case thursday:
		strcat(timeString, "Th ");
		break;
	case friday:
		strcat(timeString, "F  ");
		break;
	case saturday:
		strcat(timeString, "Sa ");
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
		printf("%s \r\n", currentTimeString);
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
		AlarmTimes[i].rawTime = AlarmTimes[i].second + 60*AlarmTimes[i].minute + 3600*AlarmTimes[i].hour + SECONDS_IN_A_DAY*AlarmTimes[i].day;
		printf("Setting alarm at raw-time: %lu\r\n", AlarmTimes[i].rawTime);
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

		AlarmTimes[i].day = AlarmTimes[i].rawTime / (60*60*24);
		AlarmTimes[i].hour = (AlarmTimes[i].rawTime - AlarmTimes[i].day * (60*60*24)) / (60*60);
		AlarmTimes[i].minute = (AlarmTimes[i].rawTime / 60) % 60;
		AlarmTimes[i].second = AlarmTimes[i].rawTime % 60;
	}

	NumberOfAlarms = numberOfAlarms;

	return 0;
}

int time_getAlarms(Time *alarms, unsigned long *numberOfAlarms) {
	if(alarms==NULL || numberOfAlarms==NULL) {
		return -1;
	}

	memcpy(alarms, AlarmTimes, NumberOfAlarms*sizeof(Time));
	*numberOfAlarms = NumberOfAlarms;

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
