
#include <p18cxxx.h>
#include <delays.h>
#include <usart.h>
#include <i2c.h>
#include <timers.h>
#include <stdio.h>
#include <string.h>

#include "ds3231.h"
#include "dcf.h"

#pragma config FOSC = IRC, CPUDIV = NOCLKDIV, PLLEN = OFF
#pragma config FCMEN = OFF, BOREN = OFF
#pragma config WDTEN = OFF, MCLRE = ON, LVP = OFF




#define DEBUG_GENERAL 1
#define DEBUG_RTC 1


#define ISCP_PGD PORTAbits.RA0

#define ICSP_PGC PORTAbits.RA1

#define MCLR    PORTAbits.RA3

#define DAT_RCK PORTAbits.RA4

#define DAT_SRC PORTAbits.RA5



#define I2C_SDAx PORTBbits.RB4

#define DEB_RX  PORTBbits.RB5

#define I2C_SCLx PORTBbits.RB6

#define DEB_TX  PORTBbits.RB7


// #define DCF_LED PORTCbits.RC0 // BREADBOARD 
// #define DCF_SIG PORTCbits.RC1 // BREADBOARD 

#define DCF_SIG PORTCbits.RC0
#define DCF_LED PORTCbits.RC1


#define RTC_INT PORTCbits.RC2

#define DAT_OUT PORTCbits.RC3

#define BTN_NXT PORTCbits.RC4

#define BTN_SET PORTCbits.RC5

#define SWI_DEB PORTCbits.RC6
//#define SWI_DEB (PORTC & 0b01000000)

#define SWI_ROT PORTCbits.RC7
//#define SWI_ROT (PORTC & 0b10000000)


/* ***************************** */
/*  CLOCK                        */
/* ***************************** */
#define DEBUGMODE_OFF 0x0
#define DEBUGMODE_ON 0x2

unsigned int debugmode = DEBUGMODE_ON;

#define MODE_AUTO 0x0
#define MODE_MANUAL 0x1

unsigned int mode = MODE_AUTO;

#define DISPLAY_HRS_MIN 0x0
#define DISPLAY_DAY_MON 0x1
#define DISPLAY_YEAR 0x2
#define DISPLAY_TEMP 0x3

#define DISPLAY_FIRST 0x0
#define DISPLAY_LAST 0x3

unsigned int display = DISPLAY_HRS_MIN;

#define WORKMODE_DISPLAY 0x0
#define WORKMODE_SETTIME_SEC 0x1
#define WORKMODE_SETTIME_MIN 0x2
#define WORKMODE_SETTIME_HRS 0x3
#define WORKMODE_SETTIME_DAY 0x4
#define WORKMODE_SETTIME_MON 0x5
#define WORKMODE_SETTIME_YRS 0x6

#define WORKMODE_SETTIME_FIRST 0x2
#define WORKMODE_SETTIME_LAST 0x6

unsigned int workmode = WORKMODE_DISPLAY;


unsigned int lastTemp = 0;

/* ***************************** */
/*                               */
/* ***************************** */

void configure();
void shiftout(unsigned int data);
void strobe();


void high_isr(void);
void low_isr(void);

void displayUpdate();
void displayTemperature(long temp);
void displayTime();
void displayDate();
void displayYear();
void displayZero();

void handlekeycode(unsigned int keycode);
void dokeyboard();
char pop();
void dcfsavetime();
void dcfdecode();


/**********************************************************
/ Global
/**********************************************************/

unsigned char timeout50msec = 0; // TODO: Make bit variable
unsigned char timeout1sec = 0; // TODO: Make bit variable
unsigned char timeout500msec = 0; // TODO: Make bit variable

#define TIMEOUT_50MSEC_START 5
#define TIMEOUT_500MSEC_START 50
#define TIMEOUT_1SEC_START 100

unsigned char timer50msec = TIMEOUT_50MSEC_START;
unsigned char timer500msec = TIMEOUT_500MSEC_START;
unsigned char timer1sec = TIMEOUT_1SEC_START;

/**********************************************************
/ Interrupt Low
/**********************************************************/

#pragma code low_vector=0x18
void interrupt_at_low_vector(void)
{
_asm GOTO low_isr _endasm
}


#pragma code // return to the default code section 
#pragma interruptlow low_isr
void low_isr (void)
{
/*
 	if (INTCONbits.INT0IF) { // DCF CHANGE

		INTCONbits.INT0IF = 0; // CLEAR INTERRUPT FLAG

		if (DCF_SIG) { // UP

			INTCON2bits.INTEDG0 = 0; // Interrupt on falling edge

			dcfpulseup();

		} else {             // DOWN

			INTCON2bits.INTEDG0 = 1; // Interrupt on rising edge

			dcfpulsedown();

        }      
	}
*/
	if (INTCON3bits.INT2IF) { // RTC
		INTCON3bits.INT2IF = 0; // CLEAR INTERRUPT FLAG
	}

}


/**********************************************************
/ Interrupt High
/**********************************************************/

#pragma code high_vector=0x08
void interrupt_at_high_vector(void)
{
_asm GOTO high_isr _endasm
}

int state1 = 0;


#pragma code /* return to the default code section */
#pragma interrupt high_isr
void high_isr (void)
{

	if (INTCONbits.TMR0IF) { // TIMER0: 

		// TMR0H = 0x65; // 20msec
		TMR0H = 0xB3; // 10msec
        TMR0L = 0x00;

		INTCONbits.TMR0IF = 0; // CLEAR INTERRUPT FLAG
		
		// DCF TIMER MACRO
		DCFTICK_INCREMENT 
		// dcfpulsetime++;

		#if defined DEBUG_TIMER
			if (state1) {
				state1 = 0;
			} else {
				state1 = 1;
			}
		#endif

	    if (!timer50msec) {
			timeout50msec = 1;
			timer50msec = TIMEOUT_50MSEC_START;
		} else timer50msec--;
 
	    if (!timer500msec) {
			timeout500msec = 1;
			timer500msec = TIMEOUT_500MSEC_START;
		} else timer500msec--;

	    if (!timer1sec) {
			timeout1sec = 1;
			timer1sec = TIMEOUT_1SEC_START;
		} else timer1sec--;
		
	}

 	if (INTCONbits.INT0IF) { // DCF CHANGE

		INTCONbits.INT0IF = 0; // CLEAR INTERRUPT FLAG

		if (DCF_SIG) { // UP

			INTCON2bits.INTEDG0 = 0; // Interrupt on falling edge

			dcfpulseup();

		} else {             // DOWN

			INTCON2bits.INTEDG0 = 1; // Interrupt on rising edge

			dcfpulsedown();

        }      
	}

	if (INTCONbits.INT0IF) { // RTC
		INTCONbits.INT0IF = 0; // CLEAR INTERRUPT FLAG
	}

}


void dcfsavetime()
{
     if ( (
          ( (dcf2.hour == dcf1.hour) && ((dcf2.min - dcf1.min) == 1) )
          || 
          ( ((dcf2.hour - dcf1.hour) == 1) && (dcf2.min == 0) ) 
         ) && (dcf2.wday == dcf1.wday) && (dcf2.day == dcf1.day) && (dcf2.month == dcf1.month) && (dcf2.year == dcf1.year) ) {
     // alles super, zwei konsistente zeiten empfangen...
     // also setzen und speichern!
#if defined DEBUG_GENERAL
      printf("Valid Time: %02u:%02u    %u, %02u.%02u.%02u\r\n",dcf2.hour,dcf2.min,dcf2.wday,dcf2.day,dcf2.month,dcf2.year);
#endif

      CurrentTime.Hour = dcf2.hour;
      CurrentTime.Minute = dcf2.min;
      CurrentTime.Second = 0;
      CurrentTime.Hundredth = 1;  // ist immer zweite mainloop-runde seit letztem dcf-takt,
                                 // also ist die zeit schon 2x50msec alt!
      CurrentTime.Day = dcf2.day;
      CurrentTime.Weekday = dcf2.wday;
      CurrentTime.Month = dcf2.month;
      CurrentTime.Year = dcf2.year;

      RTCSetTime(&CurrentTime);

      dcfsyncstate = 3;
     } else { // if ( (dcf2.min 
      // zeit ist zwar gültig aber nicht konsistent mit der vorigen, 
      // also diese speichern und auf die nächste warten...
      dcfsyncstate = 2;
     }
  
     // immer den aktuellen satz in den ersten puffer schieben 
     memcpy(&dcf1,&dcf2,sizeof(dcf1));
     dcfsequence = 1;
}


/**********************************************************
/ Queue
/**********************************************************/
#define QUEUE_LENGTH 30

int queue[QUEUE_LENGTH];

unsigned int queue_start = 0;
unsigned int queue_stop = 0;

void push(char c) {

// fprintf(terminal,"push %c qlen: %u qstart: %u qstop: %u \r\n",c,get_queue_length(),queue_start,queue_stop);
 queue[queue_stop] = c;
 if (queue_stop == (QUEUE_LENGTH-1)) {
   if (queue_start>0) {
    queue_stop = 0;
   }
 } else {
  if (queue_stop == (queue_start - 1)) {
  } else {
   queue_stop++;
  }
 } 
// output_high(INT_PIN);
}

unsigned int get_queue_length()
{
  if (queue_start == queue_stop) return 0;
  if (queue_start < queue_stop) return  (queue_stop - queue_start);
  else return  (QUEUE_LENGTH - queue_start) + queue_stop;
}

char pop()
{
 char c = 0;
 if (queue_start != queue_stop) {
  c = queue[queue_start];
//  fprintf(terminal,"pop %c qstart: %u qstop: %u \r\n",c,queue_start,queue_stop);
  if (queue_start == (QUEUE_LENGTH-1)) queue_start = 0;
  else {
    queue_start++;
  }
 }
 return c;
}

//**********************************************************
// 1x2 STATIC KEYBOARD HANDLING 
//**********************************************************
#define MBIT_MAKE  0b00010000
#define MBIT_BREAK 0b00100000

#define MBIT_SET  0b00000010
#define MBIT_NXT  0b00000001

#define MBIT_ID_ROTARYPUSH 0x0

void dokeyboard()
{
  static unsigned int keystate1 = 0xFF;
  unsigned int keystate;
  unsigned int keydelta;
  unsigned int i;

  keystate = ((PORTC & 0b00110000) >> 4);
  if (keystate != keystate1) 
  {
    keydelta = (keystate ^ keystate1);
    for (i=0;i<2;i++) {
     if (keydelta & ( 1 << i)) {
        if (keystate & (1 << i)) push (MBIT_ID_ROTARYPUSH | MBIT_BREAK | (i+1) );
       else push (MBIT_ID_ROTARYPUSH | MBIT_MAKE | (i+1) );
     }
    }    
    keystate1 = keystate;
  }
}


/**********************************************************/
// Execute Keyboard Commands
/**********************************************************/
void handlekeycode(unsigned int keycode) {

#ifdef DEBUG_GENERAL
	if (keycode & MBIT_MAKE) {
	  printf("MAKE ");
	} else {
	  printf("BREAK ");
	}
	
	if (keycode & MBIT_SET) printf ("SET\r\n");
	else printf ("UP\r\n");
#endif 

 if (keycode & MBIT_MAKE) {
	if (keycode & MBIT_SET) {

      if (workmode == WORKMODE_SETTIME_LAST) {
        RTCSetTime(&CurrentTime);
		workmode = WORKMODE_DISPLAY;
      } else if (workmode == WORKMODE_DISPLAY) {
		workmode = WORKMODE_SETTIME_FIRST;
      } else workmode++;
#ifdef DEBUG_GENERAL
  printf ("WORKMODE %d\r\n",workmode);
#endif

	} else if (keycode & MBIT_NXT) {

      if (workmode == WORKMODE_SETTIME_MIN) {
		if (CurrentTime.Minute == 59) CurrentTime.Minute = 0;
        else CurrentTime.Minute++;
      } else if (workmode == WORKMODE_SETTIME_HRS) {
		if (CurrentTime.Hour == 23) CurrentTime.Hour = 0;
        else CurrentTime.Hour++;
      } else if (workmode == WORKMODE_SETTIME_DAY) {
		if (CurrentTime.Day == 31) CurrentTime.Day = 1;
        else CurrentTime.Day++;
      } else if (workmode == WORKMODE_SETTIME_MON) {
		if (CurrentTime.Month == 12) CurrentTime.Month = 1;
        else CurrentTime.Month++;
      } else if (workmode == WORKMODE_SETTIME_YRS) {
		if (CurrentTime.Year == 20) CurrentTime.Year = 0;
        else CurrentTime.Year++;
	  } else {  // ANZEIGE WECHSELN
         RTCSetTime(&CurrentTime);
         if (display == DISPLAY_LAST) display = DISPLAY_FIRST;
         else display++;
      }
#ifdef DEBUG_GENERAL
  printf ("HOUR %d MIN %d\r\n",CurrentTime.Hour,CurrentTime.Minute);
#endif

    }
 }



}



/**********************************************************
/ BCD to 7-segment conversion
/**********************************************************/

const unsigned int BCDto7SEG[16] = {

				0b00111111,
				0b00000110,
				0b01011011,
				0b01001111,
				
				0b01100110,
				0b01101101,
				0b01111101,
				0b00000111,
				0b01111111,
				0b01101111,
				
				0b01110111,
				0b01111100,
				0b00111001,
				0b01011110,
				0b01111001,
				0b01110001

};


/**********************************************************
/ WRITE
/**********************************************************/
void shiftout(unsigned int data)
{
	int i;
	
	for (i = 0; i < 8; i++) {

		//PORTBbits.RB5 = (data & 0x1); // DATA
		DAT_OUT = (data & 0x1); // DATA
        data >>= 1;
		Delay10TCYx(1);
		DAT_SRC = 1; // SRCK
		Delay10TCYx(1);
		DAT_SRC = 0; // SRCK

	}

}

void strobe()
{
	DAT_RCK = 1; // RCK
	Delay10TCYx(1);
	DAT_RCK = 0; // RCK
}

/**********************************************************
/ DISPLAY
/**********************************************************/

int digit[5];
int editdisplay = 0;

void displayUpdate()
{
	shiftout(digit[4]);
    shiftout(digit[3]);
	shiftout(digit[2]);
	shiftout(digit[1]);

    strobe();
}

void displayTemperature(long temp)
{

    digit[4] = BCDto7SEG[temp % 10];
    temp = temp / 10;

    digit[3] = BCDto7SEG[temp % 10];
    temp = temp / 10;
    
    digit[2] = BCDto7SEG[temp % 10];
    temp = temp / 10;

  //  digit[1] = BCDto7SEG[temp % 10];

	digit[1] = 0;
 
    digit[3] |= 0b10000000;
}

void displayTime()
{
    digit[4] = BCDto7SEG[CurrentTime.Minute % 10];

    digit[3] = BCDto7SEG[CurrentTime.Minute / 10];

    digit[2] = BCDto7SEG[CurrentTime.Hour % 10];

    digit[1] = BCDto7SEG[CurrentTime.Hour / 10];

	digit[2] |= 0b10000000;
}

void displayDate()
{
    digit[4] = BCDto7SEG[CurrentTime.Month % 10];

    digit[3] = BCDto7SEG[CurrentTime.Month / 10];

    digit[2] = BCDto7SEG[CurrentTime.Day % 10];

    digit[1] = BCDto7SEG[CurrentTime.Day / 10];

	digit[2] |= 0b10000000;
    digit[4] |= 0b10000000;
}

void displayYear()
{
    digit[4] = BCDto7SEG[CurrentTime.Year % 10];

    digit[3] = BCDto7SEG[CurrentTime.Year / 10];

    digit[2] = BCDto7SEG[0];

    digit[1] = BCDto7SEG[2];

}

void setDisplayBuffersToZero()
{
	digit[1] = BCDto7SEG[0];
	digit[2] = BCDto7SEG[0];
	digit[3] = BCDto7SEG[0];
	digit[4] = BCDto7SEG[0];
}




/**********************************************************
/ Configuration
/**********************************************************/
void configure()
{
	OSCCON = 0x70;  // bit 6-4 IRCF<2:0>: Internal Oscillator Frequency Select bits = 16MHZ

	ANSEL = 0x00; // All Pins digital
	ANSELH = 0x00; // All Pins digital
	ADCON0 = 0x00; // ADC Off

	TRISA = 0; // PORT A ALL OUTPUT
	TRISB = 0b01010000; // PORT B ALL OUTPUT, 4 AND 6 / I2C INPUT
	// TRISC = 0b11110110; // BREADBOARD
	TRISC = 0b11110101; 

	INTCONbits.TMR0IE = 1;   // TMR0 Overflow Interrupt Enable bit

	INTCONbits.INT0IE = 1;   // Enables the INT0 external interrupt
	INTCON2bits.INTEDG0 = 1; // Interrupt on rising edge
//	INTCON3bits.INT0IP = 0;  // INT1 External Interrupt Priority bit: Low priority
	INTCONbits.INT0IF = 0;   // CLEAR INTERRUPT FLAG

	//INTCON3bits.INT1IE = 1;  // Enables the INT1 external interrupt
	//INTCON2bits.INTEDG1 = 0; // Interrupt on falling edge
	//INTCON3bits.INT1IP = 0;  // INT1 External Interrupt Priority bit: Low priority
	//INTCON3bits.INT1IF = 0;  // CLEAR INTERRUPT FLAG

	INTCON3bits.INT2IE = 1;  // Enables the INT2 external interrupt
	INTCON2bits.INTEDG2 = 1; // Interrupt on rising edge
	INTCON3bits.INT2IP = 0;  // INT2 External Interrupt Priority bit: Low priority
	INTCON3bits.INT2IF = 0;  // CLEAR INTERRUPT FLAG

}




/**********************************************************
/ MAIN
/**********************************************************/
void main (void)
{

	// unsigned int counter = 0;
	int displaycountdown = 0;

	configure();

	PORTAbits.RA4 = 0; // RCK
	PORTAbits.RA5 = 0; // SRCK

	// configure USART
	OpenUSART( 
		USART_TX_INT_OFF &
		USART_RX_INT_OFF &
		USART_ASYNCH_MODE &
		USART_EIGHT_BIT &
		USART_CONT_RX &
		USART_BRGH_HIGH,
		25 ); // 38.400 baud

	OpenTimer0( 
		TIMER_INT_ON &
		T0_16BIT &
		T0_SOURCE_INT &
		T0_PS_1_1 );

	OpenI2C(MASTER, 
			SLEW_ON);
	SSPADD = 39; //400kHz Baud clock(9) @16MHz
				//100kHz Baud clock(39) @16MHz

	RCONbits.IPEN = 1;   

	INTCONbits.GIE = 1; // General Interrupt Enable
	INTCONbits.GIEH = 1; // High Priority Interrupt Enable
	INTCONbits.GIEL = 1; // Low Priority Interrupt Enable

	CurrentTime.Minute = 0;
	CurrentTime.Hour = 0;
	CurrentTime.Day = 0;
	CurrentTime.Month = 0;
	CurrentTime.Year = 0;

	setDisplayBuffersToZero();
	displayUpdate();

	initRTC();
	displayTime();

	DCF_LED = 1;

	#ifdef DEBUG_GENERAL
		printf("\r\nBuzz Numitron Clock v.1\r\n");
	#endif

	while (1) {
/*
	   	if (dcfsequence) {
 		 	output_low(LED2);
       	} else {
         	output_high(LED2);
       	}

	   	if (dcfsyncstate == 3) {
 		 	output_low(LED3);
       	} else {
         	output_high(LED3);
       	}

*/
		if (queue_start != queue_stop) {
        	handlekeycode(queue[queue_start]);
	    	pop();
       	}


		if (dcfsavetimenow) {
			dcfsavetimenow = 0;

			if (workmode == WORKMODE_DISPLAY) {
				dcfsavetime();
	
				displayTime();
				displayUpdate();
			}
		}

		if (dcfdecodenow) {
			dcfdecodenow = 0;
			dcfdecode();		
		}

		if (timeout50msec) {
			timeout50msec = 0;
			dokeyboard();

            if (SWI_ROT) {
				if (mode != MODE_AUTO) {
					mode = MODE_AUTO;
					#ifdef DEBUG_GENERAL
						printf("Rotating mode set to auto\r\n");
					#endif
				}
			} else {
				if (mode != MODE_MANUAL) {
					mode = MODE_MANUAL;
					#ifdef DEBUG_GENERAL
						printf("Rotating mode set to manual\r\n");
					#endif
				}
			}

            if (SWI_DEB) {
				if (debugmode != DEBUGMODE_ON) {
					debugmode = DEBUGMODE_ON;
					#ifdef DEBUG_GENERAL
						printf("Debug mode enabled\r\n");
					#endif
				}
			} else {
				if (debugmode != DEBUGMODE_OFF) {
					debugmode = DEBUGMODE_OFF;
					#ifdef DEBUG_GENERAL
						printf("Debug mode disabled\r\n");
					#endif
				}
			}

		}

		if (timeout500msec) {
			timeout500msec = 0;

            if (workmode != WORKMODE_DISPLAY ) {
				if (workmode == WORKMODE_SETTIME_MIN) {
					displayTime();
					if (!editdisplay) {
						digit[3] = 0;
                       	digit[4] = 0;
					}
                } else if (workmode == WORKMODE_SETTIME_HRS) {
					displayTime();
					if (!editdisplay) {
						digit[1] = 0;
                       	digit[2] = 0;
					}
                } else if (workmode == WORKMODE_SETTIME_DAY) {
					displayDate();
					if (!editdisplay) {
						digit[1] = 0;
                       	digit[2] = 0;
					}
                } else if (workmode == WORKMODE_SETTIME_MON) {
					displayDate();
					if (!editdisplay) {
						digit[3] = 0;
                       	digit[4] = 0;
					}
                } else if (workmode == WORKMODE_SETTIME_YRS) {
					displayYear();
					if (!editdisplay) {
						digit[3] = 0;
                       	digit[4] = 0;
					}
				}
              	editdisplay = !editdisplay;
				displayUpdate();
            } 

		}

		if (timeout1sec) {
			timeout1sec = 0;

           if (workmode == WORKMODE_DISPLAY) {

                if (display == DISPLAY_TEMP) {
					lastTemp = RTCGetTemp();
					displayTemperature(lastTemp);

		 		} else if (display == DISPLAY_HRS_MIN) {
	
					RTCGetTime(&CurrentTime);
					displayTime();	

		 		} else if (display == DISPLAY_DAY_MON) {
	
					RTCGetTime(&CurrentTime);
					displayDate();	

		 		} else if (display == DISPLAY_YEAR) {
	
					RTCGetTime(&CurrentTime);
					displayYear();	
				}

    			displayUpdate();

	            if (mode == MODE_AUTO) {
				  
                  if (!displaycountdown) 
                  {
                    if (display == DISPLAY_LAST) display = DISPLAY_FIRST;
                    else display++;

                    if (display == DISPLAY_HRS_MIN) {
                      displaycountdown = 8;
                    } else if (display == DISPLAY_DAY_MON) {
                      displaycountdown = 2;
                    } else if (display == DISPLAY_YEAR) {
                      displaycountdown = 2;
                    } else if (display == DISPLAY_TEMP) {
                      displaycountdown = 2;
                    }
					
                  } else displaycountdown--;
                }

			}
		}

	}

}
