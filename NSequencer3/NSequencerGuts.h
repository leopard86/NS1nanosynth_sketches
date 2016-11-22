#ifndef NSEQUENCER_GUTS_H
#define NSEQUENCER_GUTS_H

#include "fakeTimer.h"

/****
 * HARDWARE STUFF
 ****/
#define IN_CLK      7
#define IN_RST      3 // muxed to SCL line, be careful
#define IN_RETRIG   8
#define IN_TEMPOMUL A0
#define IN_PITCH    A1
#define IN_MOD      A2
#define IN_SAMPLE   5
#define OUT_LED     13
//#define OUT_PITCH   DAC0
//#define OUT_MOD     DAC1
#define OUT_GATE    11
#define OUT_TRIG    12
#define DAC_RANGE   4095
#define TSWIN_SIZE  8 // allows efficient divisions
#define MIDITSWIN_SIZE 16 // idem
#define TSWIN_BS    3 // bitshift factor for division. Change it with TSWIN_SIZE
#define PITCH_DIVISOR 4 // divides by 2^4, bitshift division to reduce pitch excursion
#define ADC_BITS    10 // analogIn reads 1024 values
 

/****
 * MIDI STUFF
 ****/
enum {
  NOTEON = 0x90,
  NOTEOFF = 0x80,
  CCHANGE = 0xB0,
  PITCHBEND = 0xE0,
};

enum {
  CLKSRC_EXT = 0x1,
  CLKSRC_INT = 0x2,
  CLKSRC_MIDI = 0x4,
};

enum {
  SRC_MIDI = 0,
  SRC_EXT,
  TOT_SOURCES,
};

#define MIDI_TIMECLOCK  0xF8
#define MIDI_START      0xFA
#define MIDI_STOP       0xFC
#define CC_MODWHEEL 1
#define MIDI_NORM_FACT 32 // DAC_RANGE / 128, normalize modulation 0-127 values to DAC_RANGE
#define TWELVE 12 // midi timestamps are sent 12 per 1/8 note. This can be changed for tricks
#define PRESENCE_COUNTER 2 // if 2 beats are missed (i.e. 2*12 timestamps) the midi clock has vanished

/****
 * MUSICAL STUFF
 ****/
 
 uint16_t VOct61[61] = {0, 68, 137, 205, 273, 341, 410, 478, 546, 614, 683, 751, 819, 887, 956, 1024, 
                      1092, 1160, 1229, 1297, 1365, 1433, 1502, 1570, 1638, 1706, 1775, 1843, 1911, 
                      1979, 2048, 2116, 2184, 2252, 2321, 2389, 2457, 2525, 2594, 2662, 2730, 2798, 
                      2867, 2935, 3003, 3071, 3140, 3208, 3276, 3344, 3413, 3481, 3549, 3617, 3686, 
                      3754, 3822, 3890, 3959, 4027, 4095};

// music scales
uint8_t chromSc[]=  {0 ,1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
uint8_t majSc[] =   {0, 0, 2, 2, 4, 5, 5, 7, 7, 9, 9, 11};
uint8_t minSc[] =   {0, 0, 2, 3, 3, 5, 5, 7, 8, 8, 10, 10};
uint8_t pentaSc[] = {0, 0, 0, 3, 3, 5, 5, 7, 7, 9, 9, 9};


#endif // NSEQUENCER_GUTS_H
