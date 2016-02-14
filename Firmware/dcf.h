
#define DEBUG_DCF77 1

#define DEBUGMODE_OFF 0x0
#define DEBUGMODE_ON 0x2

#define DCF_LED PORTCbits.RC1

// Values for 10msec timer
#define MAX_BITCOUNT 59
#define MIN_PULSETIME_HIGH 30
#define MIN_PULSETIME_NEWMINUTE 240
#define MIN_PLAUSIBLE_PULSETIME 10

extern unsigned int dcfpulsetime;

extern unsigned char dcfbuffer[8];
extern unsigned char dcfbitcount;
extern unsigned char dcfsequence;
extern unsigned char dcfdecodenow;
extern unsigned char dcfsavetimenow;
extern unsigned char dcfsyncstate;

extern unsigned int debugmode;

struct dcftimestamp {
 unsigned char min;
 unsigned char hour;
 unsigned char wday;
 unsigned char day;
 unsigned char month;
 unsigned char year;
 unsigned char utcoffset;
 unsigned char alert;
 unsigned char offsetchange;
 unsigned char leapsecond;
};

extern struct dcftimestamp dcf1;
extern struct dcftimestamp dcf2;

unsigned char do_dcf_decode(struct dcftimestamp* dcf);
void dcfdecode();

void dcfpulsedown();
void dcfpulseup();

#define DCFTICK_INCREMENT dcfpulsetime++;



// void dcfsavetime()
