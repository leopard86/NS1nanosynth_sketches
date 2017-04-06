#ifndef NSCONFIG_H
#define NSCONFIG_H

/* COMMENT THESE LINES AS YOU WISH */
#define MIDI_CHANNEL 0
//#define DEBUG
//#define SHOW_STEPS
//#define EXT_CLOCK_MODE_FOLLOW 1
#define EXT_CLOCK_MODE_TAP 2

/* *** DEFINES *** */
#define STEPS       8 // must be power of 2

/* DO NOT TOUCH */
#ifdef EXT_CLOCK_MODE_FOLLOW
#define EXT_CLOCK_MODE EXT_CLOCK_MODE_FOLLOW
#else
#define EXT_CLOCK_MODE EXT_CLOCK_MODE_TAP
#endif

#endif // NSCONFIG_H
