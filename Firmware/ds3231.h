
struct atimestamp {
    unsigned char Hundredth;
	unsigned char Second;
	unsigned char Minute;
    unsigned char Hour;
	unsigned char Day;
	unsigned char Month;
    unsigned char Weekday;
    unsigned char Year;
    unsigned char Updated;
};


extern void initRTC(void);
extern unsigned int RTCGetTemp(void);
extern void RTCGetTime(struct atimestamp* timestamp);
extern void RTCSetTime(struct atimestamp* timestamp);

extern struct atimestamp CurrentTime;

unsigned char bcddecode(unsigned char  bcddata);
unsigned char bcdencode(unsigned char decdata);

#define DEBUG_GENERAL 1