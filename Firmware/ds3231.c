#include <p18cxxx.h>
#include <delays.h>
#include <usart.h>
#include <i2c.h>
#include <timers.h>
#include <stdio.h>
#include <string.h>

#include "ds3231.h"


struct atimestamp CurrentTime;

unsigned char bcddecode(unsigned char  bcddata)
{
 return (bcddata / 16) * 10 + (bcddata%16);
}

unsigned char  bcdencode(unsigned char decdata)
{
  return ((decdata / 10) << 4) | (decdata % 10);
}
/**********************************************************
/ DS3231 RTC
/**********************************************************/

#define DS3231_WRITE 0b11010000
#define DS3231_READ 0b11010001

#define DS3231_CURRENTTIME 0x0
#define DS3231_ALARMTIME1 0x7
#define DS3231_ALARMTIME2 0xB

#define DS3231_MASK_WEEKDAY 0b00000111
#define DS3231_MASK_DATE 0b00111111
#define DS3231_MASK_HOUR 0b00111111
#define DS3231_MASK_CENTURY 0b10000000
#define DS3231_MASK_MONTH 0b00011111


unsigned int RTCRead(unsigned int address)
{
    unsigned int nack;
    unsigned char value = 0;

	IdleI2C();                          // ensure module is idle

	StartI2C();

    while ( SSPCON2bits.SEN );          // wait until start condition is over 

	nack = WriteI2C(DS3231_WRITE);

    if (nack != 0) {
	 StopI2C();
	 while ( SSPCON2bits.SEN );          // wait until start condition is over 

#if defined DEBUG_RTC
     printf("rtcread nack 1\n\r");
#endif
    } else {

  	 IdleI2C();                      // ensure module is idle
	 nack = WriteI2C(address);
     if (!nack) {

	  RestartI2C();
 	  while ( SSPCON2bits.RSEN );     // wait until re-start condition is over 
 
  	  IdleI2C();                      // ensure module is idle
	  nack = WriteI2C(DS3231_READ);
      if (!nack) {
   	   IdleI2C();                      // ensure module is idle
	   value = ReadI2C();
      } 
#if defined DEBUG_RTC
      else printf("rtcread nack 3\n\r");
#endif
     } 
#if defined DEBUG_RTC
     else printf("rtcread nack 2\n\r");
#endif
	 StopI2C();
     while ( SSPCON2bits.SEN );          // wait until start condition is over 
    }
    return value;
}

unsigned int RTCWrite(unsigned int address, unsigned int data)
{
    unsigned int nack;
#if defined DEBUG_RTC
   printf("RTCWrite a: %u d: %X\n\r",address,data);
#endif
	IdleI2C();                          // ensure module is idle
	StartI2C();
    while ( SSPCON2bits.SEN );          // wait until start condition is over 
  	 IdleI2C();                          // ensure module is idle
    nack = WriteI2C(DS3231_WRITE);
    if (nack != 0) {
    StopI2C();
    while ( SSPCON2bits.SEN );          // wait until start condition is over 
#if defined DEBUG_RTC
    printf("RTCWrite nack 1 %X %X\n\r",address,data);
#endif
    } else {
  	 IdleI2C();                          // ensure module is idle
     nack = WriteI2C(address);
     if (nack == 0) {
	  WriteI2C(data);
     }
#if defined DEBUG_RTC
     else printf("RTCWrite nack 2 %X %X\n\r",address,data);
#endif
	 StopI2C();
     while ( SSPCON2bits.SEN );          // wait until start condition is over 
    }
    return (!nack);
}

unsigned int RTCGetTemp()
{
	unsigned int upper = 0;
    unsigned int lower = 0;
    unsigned int i = 0;

	upper = RTCRead(0x11);
	lower = RTCRead(0x12);

    i = ((upper*100) + ((lower >> 6)*25)) / 10;

    return i;
}

void RTCGetTime(struct atimestamp* timestamp)
{
	unsigned int time_sec;
	unsigned int time_sec2;
	unsigned int time_min;
    unsigned int time_hour;
	unsigned int time_weekday;
	unsigned int time_day;
	unsigned int time_monthcentury;
    unsigned int time_year;

    int nack;
    int address;
    address  = DS3231_CURRENTTIME;
   
	IdleI2C();                          // ensure module is idle

    StartI2C();
   
    while ( SSPCON2bits.SEN );          // wait until start condition is over 

	nack = WriteI2C(DS3231_WRITE);

    if (nack != 0) {
	  StopI2C();
	  while ( SSPCON2bits.PEN );      // wait until stop condition is over 
#if defined DEBUG_RTC
      printf("rtcgettime nack 1 %d\n\r",nack);
#endif
    } else {
  	 IdleI2C();                          // ensure module is idle
     
	 WriteI2C(address);

     RestartI2C();
	 while ( SSPCON2bits.RSEN );     // wait until re-start condition is over 


	 nack = WriteI2C(DS3231_READ);
     if (nack) {
	  StopI2C();
	  while ( SSPCON2bits.PEN );      // wait until stop condition is over 
#if defined DEBUG_RTC
      printf("rtcgettime nack 2\n\r");
#endif
     } else {
  	  IdleI2C();                      // ensure module is idle
      time_sec  = ReadI2C();
	  AckI2C();
      time_min  = ReadI2C();
	  AckI2C();
      time_hour = ReadI2C();
	  AckI2C();
      time_weekday  = ReadI2C();
	  AckI2C();
      time_day      = ReadI2C();
	  AckI2C();
      time_monthcentury = ReadI2C();
	  AckI2C();
      time_year     = ReadI2C();
 	  NotAckI2C();                    // send not ACK condition
  	  
	  StopI2C();
	  while ( SSPCON2bits.PEN );      // wait until stop condition is over 

	  time_sec2 = bcddecode(time_sec);
	  if (timestamp->Second != time_sec2) {
	   timestamp->Second   = bcddecode(time_sec);
	   timestamp->Minute   = bcddecode(time_min);
	   timestamp->Hour     = bcddecode(time_hour & DS3231_MASK_HOUR); 

       timestamp->Weekday = (time_weekday & DS3231_MASK_WEEKDAY);  
 	   timestamp->Day     = bcddecode(time_day);
	   timestamp->Month   = bcddecode(time_monthcentury & DS3231_MASK_MONTH);
	   timestamp->Year    = bcddecode(time_year);

      timestamp->Updated = 1;
      }
#if defined DEBUG_RTC
	printf("RTCGetTime: %02u:%02u:%02u %u, %02u.%02u.%02u\r\n",timestamp->Hour,
				                                          timestamp->Minute,
														  timestamp->Second,
				                                          timestamp->Weekday,
														  timestamp->Day,
														  timestamp->Month,
														  timestamp->Year);
#endif

	 }
    }
}

void RTCSetTime(struct atimestamp* timestamp)
{
    int nack;

#if defined DEBUG_GENERAL
	printf("RTCSetTime: %02u:%02u:%02u %u, %02u.%02u.%02u\r\n",timestamp->Hour,
				                                          timestamp->Minute,
														  timestamp->Second,
				                                          timestamp->Weekday,
														  timestamp->Day,
														  timestamp->Month,
														  timestamp->Year);
#endif

    StartI2C();

    nack = WriteI2C(DS3231_WRITE);
    if (nack) {
#if defined DEBUG_RTC
      printf("rtcsettime nack \n\r");
#endif
	 StopI2C();
    } else {
     WriteI2C(DS3231_CURRENTTIME);
	 WriteI2C(bcdencode(timestamp->Second));
	 WriteI2C(bcdencode(timestamp->Minute));
	 WriteI2C(bcdencode(timestamp->Hour));
	 WriteI2C(bcdencode(timestamp->Weekday));
	 WriteI2C(bcdencode(timestamp->Day));
	 WriteI2C(bcdencode(timestamp->Month));
	 WriteI2C(bcdencode(timestamp->Year));
	 StopI2C();
    }
}

void initRTC()
{

    CurrentTime.Hour = 0;
    CurrentTime.Minute = 0;
    CurrentTime.Second = 0;
    CurrentTime.Weekday = 0;
    CurrentTime.Day = 0;
    CurrentTime.Month = 0;
    CurrentTime.Year = 0;

	RTCGetTime(&CurrentTime);
     
}
