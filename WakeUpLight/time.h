/*
 * time.h
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#ifndef TIME_H_
#define TIME_H_

#include <inc/hw_types.h>

typedef enum _Day {
	sunday = 0,
	monday = 1,
	tuesday = 2,
	wednesday = 3,
	thursday = 4,
	friday = 5,
	saturday = 6,
} Day;

typedef struct __attribute__ ((__packed__)) _Time {
	Day day;

	unsigned char hour;
	unsigned char minute;
	unsigned char second;

	volatile unsigned long rawTime;
} Time;

void time_init();
void time_get(Time *time);
void time_set(Time *time);
void time_setRaw(unsigned long rawtime);
void time_printCurrent();
int time_setAlarms(Time *time, unsigned long numberOfAlarms);
int time_setRawAlarms(unsigned long *rawAlarms, unsigned long numberOfAlarms);
int time_getRawAlarms(unsigned long *rawAlarms, unsigned long *numberOfAlarms);
void time_setSnoozeAlarm(unsigned long rawSnoozeAlarm);
void time_clearSnoozeAlarm();
tBoolean time_checkAlarm();

#endif /* TIME_H_ */
