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
	monday = 0,
	tuesday = 1,
	wednesday = 2,
	thursday = 3,
	friday = 4,
	saturday = 5,
	sunday = 6
} Day;

typedef struct _Time {
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
void time_printCurrentOnLCD();
int time_setAlarms(Time *time, unsigned int size); // Size = number of Time array elements, max 7
tBoolean time_checkAlarm();
void time_acknowledgeAlarm();

#endif /* TIME_H_ */
