
#include <p18cxxx.h>
#include <delays.h>
#include <usart.h>
#include <i2c.h>
#include <timers.h>
#include <stdio.h>
#include <string.h>

#include "dcf.h"


unsigned int dcfpulsetime = 0;
unsigned char dcfbuffer[8];
unsigned char dcfbitcount = 0;
unsigned char dcfsequence = 0;
unsigned char dcfdecodenow = 0;
unsigned char dcfsavetimenow = 0;
unsigned char dcfsyncstate = 0;



struct dcftimestamp dcf1;
struct dcftimestamp dcf2;


// #define DEBUG_DCF77 1

/**********************************************************
/ DCF Clock Signal
/**********************************************************

Sekunde
 00      : immer 0
 01 - 14 : codierung nach Bedarf
 15      : R 0=normale 1=Reserveantenne
 16      : Ankündigung des Wechsels von Sommer/Winterzeit
           eine Stunde lang vorher=1 sonst 0
 17 - 18 : Offset zu UTC,
           man kann auch sagen bei Sommerzeit ist Bit 17=1 im Winter Bit 18=1
 19      : Ankündigung einer Schaltsekunde
           eine Stunde lang vorher=1 sonst 0
 20      : Startbit, immer 1
 21 - 27 : Minuten BCD-codiert, Bit 21 ist LSB
 28      : Paritätsbit für Minuten
 29 - 34 : Stunden BCD-codiert
 35      : Paritätsbit für Stunden
 36 - 41 : Tag, BCD-codiert
 42 - 44 : Wochentag
 45 - 49 : Monat, BCD-codiert
 50 - 57 : Jahr BCD-codiert, letzte zwei Stellen
 58      : Paritätsbit für Bit 36-57
 59      : kein Impuls, beim nächsten fängt eine neue Minute an

;********************************************************************
;55555555 54444444 44433333 33333222 22222221 11111111
;87654321 09876543 21098765 43210987 65432109 87654321   Sekunde
;PJJJJJJJ JMMMMMWW WTTTTTTP SSSSSSPm mmmmmm1a ooaR0000   DCF-Bits
;
;JJJJJJJJ 000MMMMM 00tttttt 00ssssss 0mmmmmmm 1aooaR00 00000www
; Jahr     Monat    Tag      Stunde   Minute   STATUS   Wtag
;********************************************************************

       7        6        5        4        3        2        1        0
00000pJJ JJJJJJMM MMMwwwtt ttttphhh hhhpmmmm mmm1aooa Rxxxxxxx xxxxxxx0

/**********************************************************/


/**********************************************************
/ 
/*********************************************************/
unsigned char bcddecodelocal(unsigned char  bcddata)
{
 return (bcddata / 16) * 10 + (bcddata%16);
}
/*
unsigned char  bcdencodelocal(unsigned char decdata)
{
  return ((decdata / 10) << 4) | (decdata % 10);
}*/

/**********************************************************
/ 
/*********************************************************/
unsigned char do_dcf_decode(struct dcftimestamp* dcf)
{
	#if defined DEBUG_DCF77
  		// printf(" buffer: %X %X %X %X\r\n", dcfbuffer[0],dcfbuffer[1],dcfbuffer[2],dcfbuffer[3]);
	#endif
	dcf->hour = bcddecodelocal( (dcfbuffer[4] & 0b00000111) << 3 | ((dcfbuffer[3] & 0b11100000) >> 5)); 
   	dcf->min = bcddecodelocal( (dcfbuffer[3] & 0b00001111) << 3 | ((dcfbuffer[2] & 0b11100000) >> 5));
   	dcf->day = bcddecodelocal( (dcfbuffer[5] & 0b00000011) << 4 | ((dcfbuffer[4] & 0b11110000) >> 4));
   	dcf->wday = bcddecodelocal( (dcfbuffer[5] & 0b00011100) >>2 );
   	dcf->month = bcddecodelocal( (dcfbuffer[6] & 0b00000011) << 3 | ((dcfbuffer[5] & 0b11100000) >> 5));
   	dcf->year = bcddecodelocal( (dcfbuffer[7] & 0b00000011) << 6 | ((dcfbuffer[6] & 0b11111100) >> 2));

   	dcf->alert = dcfbuffer[1] & 0b10000000 >> 7;
   	dcf->offsetchange = dcfbuffer[2] & 0b00000001;
   	dcf->leapsecond = (dcfbuffer[2] & 0b00001000) >> 3;
   	dcf->utcoffset =  (dcfbuffer[2] & 0b00000110) >> 1;


  	if ( (dcfbitcount == 59) && 
       (! (dcfbuffer[0] & 0x1)) &&
       ( dcfbuffer[2] & (1 << 4)) ) {
	#if defined DEBUG_DCF77
 		printf(" %02u:%02u %u, %02u.%02u.%02u (Valid)\r\n",dcf->hour,dcf->min,dcf->wday,dcf->day,dcf->month,dcf->year);
	#endif
   return 1;
  }

#if defined DEBUG_DCF77
   printf(" %02u:%02u %u, %02u.%02u.%02u (Invalid)\r\n",dcf->hour,dcf->min,dcf->wday,dcf->day,dcf->month,dcf->year);
#endif

 return 0;
}



/**********************************************************
/ 
/*********************************************************/
void dcfdecode()
{

#if defined DEBUG_DCF77
     printf("Decoding Time n:%u seq:%u sta:%u\r\n",dcfbitcount,dcfsequence,dcfsyncstate);
#endif

     if (dcfsequence) {

      if (do_dcf_decode(&dcf2)) {
        // zweite formal gültige zeit empfangen, 
        // also in der nächsten runde kontrollieren und speichern
#if defined DEBUG_DCF77
//     printf(" SAVE NOW (dcfmodule)\r\n");
#endif

        dcfsavetimenow = 1;
      } // if (do_dcf_decode
      // zeit viel kaputt, alles von vorne...
      dcfsequence = 0;

     } else { // if (dcfsequence)

      if (do_dcf_decode(&dcf1)) {
       // erste formal gültige zeit empfangen,
       // also auf die zweite warten und dem benutzer hoffnung machen
       dcfsequence = 1;
       dcfsyncstate = 2;
       
      }
      // zeit ist schrott, nix machen, weiter warten 
      else dcfsyncstate = 0;
     } // if (dcfsequence)
     dcfbitcount = 0;  // mit erstem sekundenbit weitermachen
     // for (i=0; i<8; i++) dcfbuffer[i] = 0; // puffer löschen
     memset(dcfbuffer,0,sizeof(dcfbuffer));
}


/**********************************************************
/ 
/*********************************************************/
void dcfpulseup()
{
	// #if defined DEBUG_DCF77
	if (debugmode == DEBUGMODE_ON) {
		DCF_LED = 1;
	}
	//#endif

	// printf("UP %u\n",dcfpulsetime);
	// dcfpulsetime = 0;

	if (dcfpulsetime > MIN_PLAUSIBLE_PULSETIME) {  // CHECK FOR PLAUSIBLE LENGTH, IGNORE OTHERWISE

		#if defined DEBUG_DCF77
	//	  printf("%u-UP:%u-\r\n",dcfbitcount,dcfpulsetime);
		#endif
		
		if (dcfpulsetime > MIN_PULSETIME_NEWMINUTE) {  // LONG
		 dcfdecodenow = 1;
		} 
		
		dcfpulsetime = 0;
	}

}

/**********************************************************
/ 
/*********************************************************/
void dcfpulsedown()
{

	// #if defined DEBUG_DCF77
	if (debugmode == DEBUGMODE_ON) {
		DCF_LED = 0;
    }   
	// #endif
	
	//printf("DN %u\n",dcfpulsetime);
	//dcfpulsetime = 0;
	
	if (dcfpulsetime > 0) {
	
		if (!dcfdecodenow) {
	
			#if defined DEBUG_DCF77
				//printf("-DN-%u\r\n",dcfpulsetime);
			#endif
	
			if (dcfbitcount < MAX_BITCOUNT) {
				if (dcfpulsetime > MIN_PULSETIME_HIGH) {
					dcfbuffer[dcfbitcount/8] |= (1 << (dcfbitcount%8));
					#if defined DEBUG_DCF77
	                //	printf("bit %2u is 1 (time: %2u)\r\n",dcfbitcount,dcfpulsetime);
					#endif
				} else {	
					dcfbuffer[dcfbitcount/8] &= ~(1 << (dcfbitcount%8));
					#if defined DEBUG_DCF77
				 	//	printf("bit %2u is 0 (time: %2u)\r\n",dcfbitcount,dcfpulsetime);
	    			#endif
				}
	
			} else dcfbitcount = 0;
	
			dcfbitcount++;
	
			if (!dcfsyncstate) dcfsyncstate = 1;
	
		}
	
		dcfpulsetime = 0;
	}

}


