WakeUpLight
===========
A wake up light using a table lamp and a Stellaris Launchpad.
The project is described here: http://beagaroo.wordpress.com

The project can be compiled in eclipse CDT with the  GNU ARM plugin (http://sourceforge.net/projects/gnuarmeclipse/) installed along with the GCC ARM Embedded compiler toolchain (https://launchpad.net/gcc-arm-embedded) . 

lights: Controls an AC dimmer circuit to set the brightness of a lamp attached to it.

sound: Uses PWM to play back a sound stored in a header file.

time: Used to get and set time form the RTC of the microcontroller and set alarms.

uartBt: Uart connection to the HC-05 module. The incoming commands are expected to end with a newline and carriage return "\n\r".

buttons: Functions to poll the snooze / alarm-off button.

lcd44780_LP: Controls an LCD over GPIO. Not used in the project at the end.

syscalls: Lowest layer to newlib for printf and malloc etc.

LM4F.ld and LM4F_startup.c: Linker script to tell the GCC linker what goes where and startup code that runs before main.

main.c Implements all the commands and the application level logic.

Command Interface:

Get time: Send 't'; Light will respond in its time format.

Set time: Send 'T' followed by the time format. e.g. T1:16:45:10 will set time to Monday, 16:45 and 10 seconds.

Get alarms: Send 'a'; Light will respond with 7 alarms in time format. Alarms that are not set will be sent as 7:00:00:00. 

Set alarms: Send 'A' followed by up-to 7 comma separated time values.

Set Maximum alarm Brightness and Delay: B\<Maximum Alarm Brightness\>\<Alarm Delay\>. Brightness is in percentage (0-100); Alarm delay is the time taken in minutes for the ligth to ramp up to the maximum allowed brightness.

Set Light Brightness: Send L followed by brightness in percent. e.g. L35 will set the brightness to 35 percent.

Snooze Alarm: Send 'z' and the alarm sound will stop for 10 minutes but the light will stay on. This command is currently unused.

Stop Alarm: Send 'U'.

Demo Mode: Send 'd' and the light will ramp up its brightness gradually to full and gradually lower it back to 0.


Time format: \<Day of Week\>:\<Hour\>:\<Minute\>:\<Second\> where \<Hour\> is in 24-hour format and \<Day of Week\> is a number (0-6) where Sunday=0, Monday=1, ..... Saturday=6




