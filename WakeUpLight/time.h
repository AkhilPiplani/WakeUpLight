/*
 * time.h
 *
 *  Created on: 05-May-2013
 *      Author: Akhil
 */

#ifndef TIME_H_
#define TIME_H_

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

	unsigned long rawTime;
} Time;

void Time_init();
void Time_set(Time *time);
void Time_get(Time *time);
void Time_printCurrentOnLCD();
//void Time_setAlarm(Time *time, Day *days);

#endif /* TIME_H_ */
