/* *****************************************************************************************************
 * 
 * NSEQUENCER firmware v3L: (pronounc.:"ènzequènzer 3" in Ancona vernacular)
 *      for
 * Soundmachines NS-1 nanosynth, based on Arduino Leonardo
 *   authored by
 * Leonardo Gabrielli <leonardo.o.gabrielli@gmail.com>
 * 
 * Description:
 * MIDI+VOLTAGE TRIG Step sequencer for the NS-1 with variable stuttering and reset
 * v3L adds sequencer information with two RGBdigits (available online from www.rgbdigit.com)
 * 
 * Features:
 * Rising Edge Clock Input drives the step sequencer. (IN_CLK), or...
 * a MIDI DAW sends clock timestamps and notes.
 * Input pitch and mod (IN_PITCH, IN_MOD) are sampled and stored in the current step when IN_SAMPLE is high
 * The pitch is quantized to a musical scale (hard-coded and selectable between chromatic, major, minor, pentatonic)
 * The pitch is not quantized to a musical scale when the MIDI input is present
 * The clock tempo is estimated and a retrig button input (IN_RETRIG) is provided for stuttering at a multiple of the tempo (IN_TEMPOMUL)
 * Gate and trigger outs are provided (OUT_GATE OUT_TRIG) as well as the pitch and mod out (DAC0 DAC1 outs)
 * Driving IN_RST high prevents CLK triggering new steps
 * The Note On velocity message is mapped to the Mod value
 * MOD Wheel messages are stored as modulation values in the step sequencer
 * 
 * Technical note: no HW timers have been used on this new version. This makes the code 
 * look like dummy stuff with most timing embedded in the loop(). 
 * This is due to Leonardo timers libraries being faulty, crashing and smashing.
 * I just kept compatibility with previous code using timers by implementing a class for handling
 * a fake timer.
 * 
 * Suggested configuration (beginners):
 * pitch out to V/oct
 * mod to whatever you wish (e.g. filter)
 * Button 1 for TAPPING TEMPO or a LFO square to IN_CLK
 * Button 2 to IN_SAMPLE
 * Button 3 to IN_RETRIG or IN_RST
 * CTRL1 and CTRL2 to pitch and mod in, or...
 * a variable voltage of your choice to IN_TEMPOMUL for regulating stuttering interval
 * OUT_GATE or OUT_TRIG (or OUT_LED) to GATE input of the ADSR
 * Then create a simple VCO-VCF-VCA patch with your NS-1
 * 
 * Go creative:
 * turn a variable voltage into your IN_SAMPLE 
 * make the pitch out control your filter frequency with the filter in sel-oscillation
 * crank up your brain, this part is up to you!
 * 
 * ToDo:
 * Glide?
 * Selectable musical scale by external analog in
 * 
 * Bugs:
 * the top green led will flash on the first digit sometimes
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS 
 * SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL 
 * THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 * 
 */

#include <SPI.h>
#include <DAC_MCP49xx.h>
#include <Arduino.h>
#include <inttypes.h>
#include <MIDIUSB.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "NSequencerGuts.h"
#include "NSConfig.h"
#include "fakeTimer.h"
#include "RGBDigit.h"

/* GLOBALS */
uint8_t *scaleList[] = { chromSc, majSc, minSc, pentaSc };
uint8_t currScale = 3;
uint16_t pitch[STEPS] = { 0 };
uint16_t mod[STEPS] = { 0 };
uint32_t tsWin[TSWIN_SIZE] = { 0 };
uint8_t  tsWinIdx = 0;
uint32_t midiTsWin[MIDITSWIN_SIZE] = {0};
uint8_t  midiTsWinIdx = 0;
uint8_t  midiTsIdx = 0;
uint32_t lastTs = 0;
uint32_t lastMidiTs = 0;
uint32_t fireTs = 0;
uint32_t clkPeriod[TOT_SOURCES] = {0}; // [us]
uint32_t clkPeriod_ms[TOT_SOURCES] = {0}; // [ms]
uint32_t halfPeriod_ms[TOT_SOURCES] = {0}; // [ms]
uint32_t halfPeriod[TOT_SOURCES] = {0};
uint16_t tempoMul = 1;
#if EXT_CLOCK == 2
uint8_t  clockTap = CLOCK_TAP;
#else
uint8_t  clockTap = 0;
#endif
uint8_t  midiPresent = 0;
uint8_t  rcnt = 0;
int8_t  prevRcnt = -1;
uint8_t  wcnt = 0;
uint8_t  reset = 0;
int8_t  retrigFlag = 0;
uint8_t  clkFlag = 0;
uint8_t  clkStatus = 0;
uint8_t  trig = 0;
uint8_t  gate = 0;
uint8_t  sample = 0;
uint8_t  retrig = 0;
#define  SMP_SM_MASK 0x10 // MSNibble = prev state
uint16_t  tmpPitch, tmpMod;
DAC_MCP49xx dac(DAC_MCP49xx::MCP4922, 4, -1);   //NS1nanosynth has DAC SS on pin D4
int rtId;
uint32_t tmpMillis;
#define  MIDIREADBUFS 8
midiEventPacket_t  midiReadBuf[MIDIREADBUFS]; // da init a 0
uint8_t  midiReadBufIdx = 0;
uint8_t  midiReadBufSize = 0;
fakeTimer_t t1;
int8_t processTsFlag = -1;
uint8_t lastTempoMul;
uint8_t midiSync = 0;
uint8_t midiClkDivisor = TWELVE;
uint8_t following_CC_trigger = 0;

/******
   CODE CODE CODE
 ******/
void setup() {
  Serial.begin(115200);

  pinMode( 5, OUTPUT ); // set light pin to output mode
  digitalWrite( 5, LOW );
  dac.setGain(2);
  /*  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only. Useful to debug critical crashes
    }*/
  pinMode(IN_CLK, INPUT);
  pinMode(IN_RST, INPUT);
  pinMode(IN_TEMPOMUL, INPUT);
  pinMode(IN_PITCH, INPUT);
  pinMode(IN_MOD, INPUT);
  pinMode(IN_SAMPLE, INPUT);
  pinMode(IN_RETRIG, INPUT);
  //  pinMode(OUT_PITCH, OUTPUT);
  //  pinMode(OUT_MOD, OUTPUT);
  pinMode(OUT_GATE, OUTPUT);
  pinMode(OUT_TRIG, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(IN_CLK), onCLKInterrupt, RISING);
  attachInterrupt(digitalPinToInterrupt(IN_RST), onRSTInterrupt, RISING);
  
  RGB.begin();
  RGB.show();
  RGB.setBrightness(bright);
  delay(500);
  Wire.begin();

  t1.init(1000000, InternalClkInterrupt);

  clkPeriod[SRC_EXT] = 1000000;
  halfPeriod[SRC_EXT] = 500000;

  delay(3000);
  t1.start();
}

void dbgprint(const char * str, int32_t val) {
#ifdef DEBUG
  Serial.print(str); Serial.println(val);
#endif
}

uint8_t waitTurn = 0;
void processRGBStep() {
  waitTurn = (++waitTurn & 0xF);
  if ( prevRcnt != rcnt || waitTurn == 0) {
    //uint32_t meanColor = pitch[rcnt] << (PITCH_DIVISOR + 8); // map a 0-127 val to 24bit
    uint32_t meanColor = min(2 + (pitch[rcnt]), 63); // assumes pitch from 0-63
    uint8_t meanR = (meanColor & 0x7) << 5;
    uint8_t meanG = (meanColor & 0x18) << 3;
    uint8_t meanB = (meanColor & 0x30) << 2;
    digit1 = rcnt+1; // Hp: 8 steps! no more!
    digitToRGBDigit(digit1, DIGIT1, meanR+Rrand, meanG+Grand, meanB+Brand);
    if ( reset ) {
      digit2 = LOWERR;
    } else if ( retrig ) {
      digit2 = LOWERT;
    } else if ( sample ) {
      digit2 = UPPERS;
    } else if ( midiPresent ) {
      digit2 = UPPERM;
    } else {
      digit2 = UPPERI; // non sto gestendo il caso di clock esterno (ma se lo tratto come tap no problem)
    }
    digitToRGBDigit(digit2, DIGIT2, meanR+Rrand, meanG+Grand, meanB+Brand);
    uint8_t intens = 8 + mod[rcnt] / 32;
    RGB.setBrightness(min(intens,40));
    programRGB(&RGB);
 /*   Serial.print("COL: "); Serial.println(meanColor);
    Serial.print("R: "); Serial.println(meanR);
    Serial.print("G: "); Serial.println(meanG);
    Serial.print("B: "); Serial.println(meanB);
    Serial.print("INT: "); Serial.println(intens);*/
    prevRcnt = rcnt;
  }
}




void InternalClkInterrupt() {
  clkFlag = CLKSRC_INT;
}

/* 
 *  This is called when 12 MIDI clocks have been received
 * (or less for retrigs)
 */
void onMidiClkTwelve() {
  clkFlag = CLKSRC_MIDI;
  following_CC_trigger = 0;
}

void onRSTInterrupt() { // wasting an interrupt is overkill for the reset
  reset = 1;
  rcnt = 0;
  following_CC_trigger = 0;
}

void onCLKInterrupt() {
  setTimestamps(SRC_EXT);
#if EXT_CLOCK_MODE == EXT_CLOCK_MODE_FOLLOW
  clkFlag = CLKSRC_EXT;
#elif EXT_CLOCK_MODE == EXT_CLOCK_MODE_TAP
  clkFlag = CLKSRC_EXT; // fire a note AND reinit timer to latest estimate
  t1.reinit(clkPeriod[SRC_EXT], NULL);
#endif
}

void setTimestamps(uint8_t src) {
  uint32_t curr = micros();
  uint32_t elaps;

  if ( src == SRC_MIDI ) {
    elaps = curr - lastMidiTs;
    if ( elaps > 0 ) {
      midiTsWinIdx = (++midiTsWinIdx & (MIDITSWIN_SIZE - 1));
      midiTsWin[midiTsWinIdx] = elaps;
      lastMidiTs = curr;
      processTsFlag = SRC_MIDI; // ts acquired, process later
    }
  } else {
    elaps = curr - lastTs;
    if ( elaps > 0 ) {
      tsWinIdx = (++tsWinIdx & (TSWIN_SIZE - 1));
      tsWin[tsWinIdx] = elaps;
      lastTs = curr;
      processTsFlag = SRC_EXT;
    }
  }
}

uint32_t movAvg(uint32_t *vec, uint8_t len) {
  uint8_t i;
  uint32_t avg = 0;
  for ( i = 0; i < len; i++) {
    avg += vec[i];
  }
  return avg >> TSWIN_BS;
}

void processTimestamps() {

  if ( processTsFlag == SRC_MIDI ) {
    clkPeriod[SRC_MIDI] = movAvg(midiTsWin, MIDITSWIN_SIZE);
    clkPeriod[SRC_MIDI] = clkPeriod[SRC_MIDI] * 12; // MIDI sends 24 ticks per quarter note
    clkPeriod_ms[SRC_MIDI] = clkPeriod[SRC_MIDI] / 1000; // avoid multiplication ?
    halfPeriod[SRC_MIDI] = clkPeriod[SRC_MIDI] >> 1; // period /2
    halfPeriod_ms[SRC_MIDI] = clkPeriod_ms[SRC_MIDI] >> 1; // avoided mul
    /*if ( clkPeriod[SRC_MIDI] > 0 )
      dbgprint("MIDI PERIOD: ", clkPeriod[SRC_MIDI]);
    */
    processTsFlag = -1;
  }

  if ( processTsFlag == SRC_EXT ) {
    clkPeriod[SRC_EXT] = movAvg(tsWin, TSWIN_SIZE);
    clkPeriod_ms[SRC_EXT] = clkPeriod[SRC_EXT] / 1000; // avoid multiplication ?
    halfPeriod[SRC_EXT] = clkPeriod[SRC_EXT] >> 1; // period /2
    halfPeriod_ms[SRC_EXT] = clkPeriod_ms[SRC_EXT] >> 1; // avoided mul
    dbgprint("EXT PERIOD: ", clkPeriod[SRC_EXT]);
    processTsFlag = -1;
  }

}

void noteFire() {
  
  if ( reset == 1 ) return;

  if ( midiPresent ) {
    dac.outputA(VOct61[pitch[rcnt]]); // do not quantize
  } else {
    dac.outputA(VOct61[scaleQuantizer(pitch[rcnt])]);
  }
  dac.outputB(mod[rcnt]);
  gate = HIGH;
  digitalWrite(OUT_GATE, gate);
  trig = HIGH;
  digitalWrite(OUT_TRIG, trig);
  // digiout gate up (va bene abbassarlo appena arriva un clk o è troppo tardi?)
  // digi out trig up (abbassare dopo 5ms)
  fireTs = millis();
  lightRGBDot(1, 0);
  lightRGBDot(1, 1);
  dbgprint("\nFIRING ", rcnt); dbgprint("Pitch: ", pitch[rcnt]); dbgprint("Mod: ", mod[rcnt]);
  uint8_t i;
#ifdef SHOW_STEPS
  for ( i = 0; i < STEPS; i++ ) {
    Serial.print(pitch[i]); Serial.print(" ");
  }
  Serial.println(" ");
  for ( i = 0; i < rcnt; i++ ) {
    Serial.print("   ");
  }
  Serial.println("^");
#endif

}

void clkAction() {

  if ( retrig == 0 ) {
    rcnt = ++rcnt & (STEPS - 1);
  }

  if ( clkFlag == CLKSRC_EXT ) {
    noteFire();
    dbgprint("---FIRE EXT",0);
  } else if ( clkFlag == CLKSRC_MIDI ) {
    noteFire();
    dbgprint("---FIRE MIDI",0);
  } else if ( clkFlag == CLKSRC_INT ) {
    if ( midiPresent == 0 && !following_CC_trigger ) {
      noteFire();
      dbgprint("---FIRE INT",0);
    } else 
      midiPresent = max(--midiPresent, 0);
    // non cambiare sorgente t1.reinit(clkPeriod[SRC_EXT], NULL);
  }

  clkFlag = 0;

}

// MUST RUN FAST!
void MidiTimeRead() {
  midiEventPacket_t e;
  e.byte1 = 0; e.byte2 = 0; e.byte3 = 0;

//  if ( MidiUSB.available() > 0 ) {
    e = MidiUSB.read();
    if (e.byte1 == MIDI_TIMECLOCK) { // funzionerà???
      setTimestamps(SRC_MIDI);
      midiPresent = PRESENCE_COUNTER;
      midiSync++;
      if ( midiSync == midiClkDivisor ) {
        onMidiClkTwelve();
        midiSync = 0;
      }
    }
    else if ( e.byte1 == MIDI_START ) {
      ;  // TODO
    }
    else if ( e.byte1 == MIDI_STOP ) {
      ;  // TODO
    }
    else if ( e.byte1 == 0 ) {
      return;
    }
    else if ((e.byte1 & 0xF0) == CCHANGE && e.byte2 == CC_TRIGGER) {
      onCLKInterrupt(); // treat this like an external clock interrupt
      dbgprint("CC BPM", e.byte3);
      following_CC_trigger = 1;
    }
    else {
      // THIS MIDI PACKET IS NOT A MIDI-REALTIME, STORE IT FOR FUTURE PARSING
      midiReadBuf[midiReadBufIdx].byte1 = e.byte1;
      midiReadBuf[midiReadBufIdx].byte2 = e.byte2;
      midiReadBuf[midiReadBufIdx].byte3 = e.byte3;
      midiReadBufIdx = ++midiReadBufIdx & (MIDIREADBUFS-1);
      midiReadBufSize++;
    }
//  }
}

void MidiParsing(midiEventPacket_t * e) {
  //dbgprint("BYTE 1: ", e->byte1);
  //dbgprint("BYTE 2: ", e->byte2);
  //dbgprint("BYTE 3: ", e->byte3);
  if ( e->byte1 == (NOTEON + MIDI_CHANNEL) ) {
    if (e->byte3 > 0) {
      // STORE (DISCARD MSGS W VELOCITY == 0)
      pitch[wcnt] = e->byte2;
      mod[wcnt] = (e->byte3-1) * MIDI_NORM_FACT; // zeroes velocity == 1
      wcnt = (++wcnt & (STEPS - 1));
    }
  } else if (e->byte1 == CCHANGE && e->byte2 == CC_MODWHEEL) {
    mod[wcnt] = e->byte3 * MIDI_NORM_FACT;
  }
}

// NO HARD TIME CONSTRAINT
void MidiNoteRead() {
  midiEventPacket_t e;

  // PARSE MSGS READ BUT NOT PARSED BY THE REAL-TIME MIDI CALLBACK
  while ( midiReadBufSize > 0 ) {
    MidiParsing(&midiReadBuf[--midiReadBufIdx]);
    midiReadBufIdx = --midiReadBufIdx & (MIDIREADBUFS - 1);
    midiReadBufSize--;
  }

  // PARSE ANY NEW MIDI MSG
  while (MidiUSB.available() > 0) {
    e = MidiUSB.read();
    MidiParsing(&e);
  }
}

void inputSense() {
  // SAMPLE
  tmpPitch = analogRead(IN_PITCH);
  tmpPitch = tmpPitch >> PITCH_DIVISOR;
  tmpMod = analogRead(IN_MOD);
  reset = digitalRead(IN_RST);
  sample = sample << 4; // update info on previous state
  sample = (sample & SMP_SM_MASK) + digitalRead(IN_SAMPLE); // preserve previous state, update curr state
  clkStatus = digitalRead(IN_CLK);
  retrig = retrig << 4; // update info on previous state
  retrig = (retrig & SMP_SM_MASK) + digitalRead(IN_RETRIG);
  tempoMul = analogRead(IN_TEMPOMUL);
  tempoMul = tempoMul >> (ADC_BITS - 2); // reduce range to a few int
  // RETRIG ACTIONS
  if ( retrig == SMP_SM_MASK ) {
    dbgprint("RETRIG OFF ",0);
    retrigFlag = -1;
  } else if ( retrig == 1 ) {
    dbgprint("RETRIG ON ",1);
    retrigFlag = 1;
  }
  if ( tempoMul != lastTempoMul && retrig != 0 ) {
    dbgprint("CHANGE MUL ", tempoMul);
    retrigFlag = 1;
  }
  lastTempoMul = tempoMul;
}

void doSample() {
  dbgprint("SAMPLING: ", tmpPitch); dbgprint("", tmpMod);
  pitch[wcnt] = tmpPitch;
  mod[wcnt] = tmpMod;
  wcnt = (++wcnt & (STEPS - 1));
}

void unGateTrig() {
  tmpMillis = millis();

  // UNGATE
  if ( gate ) {
    if ( retrig == 0 ) {
      if ( tmpMillis - fireTs > clkPeriod_ms[midiPresent ? SRC_MIDI : SRC_EXT] - 30 ) { // 10 ms is ok?
        gate = 0;
        //dbgprint("UNGATE", 0);
        digitalWrite(OUT_GATE, LOW); // go low before going high again
        lightRGBDot(0, 0);
      }
    } else {
      if ( tmpMillis - fireTs > (clkPeriod_ms[midiPresent ? SRC_MIDI : SRC_EXT] >> (tempoMul + 1)) - 15 ) {
        gate = 0;
        //dbgprint("UNGATE", 0);
        //RGB.setBrightness(0);
        digitalWrite(OUT_GATE, LOW); // go low before going high again
        lightRGBDot(0, 0);
      }
    }
  }

  // UNTRIG
  if ( trig ) {
    if ( tmpMillis - fireTs > 5 ) {
      trig = 0;
      //dbgprint("UNTRIG", 0);
      digitalWrite(OUT_TRIG, LOW);
      lightRGBDot(0, 1);
    }
  }
}

void loop() {

  t1.check();

  // REACT TO CLK
  if ( clkFlag ) {
    digitalWrite( OUT_LED, HIGH );
    clkAction();
    // TODO: print STEP to 7SEGM (and change slightly curr color comb? or do it in NoteFire? change colour depending on pitch in NoteFire?)
    // SCHEDULE RETRIG IN SYNC WITH LAST CLK
    if ( retrigFlag == 1 ) {
      // TODO: print T to 7SEGM
      midiClkDivisor = TWELVE / (tempoMul+2);
      t1.reinit(clkPeriod[midiPresent ? SRC_MIDI : SRC_EXT] >> (tempoMul + 1), NULL);
      retrigFlag = 0;
    } else if ( retrigFlag == -1 ) {
      midiClkDivisor = TWELVE;
      t1.reinit(clkPeriod[midiPresent ? SRC_MIDI : SRC_EXT], NULL);
      retrigFlag = 0;
    }
  } else {
    //delayMicroseconds(4); // This function works very accurately in the range 4 microseconds and up. 4us = 250kHz rate
    digitalWrite( OUT_LED, LOW );
  }

  // READ USB MIDI
  MidiTimeRead();
  MidiNoteRead();

  // TODO: if MIDI Present write M on 2nd 7SEGM, else E (for EXT)

  // INPUT SENSE
  inputSense();

  // TODO: print R if reset

  // SAMPLE
  if ( sample == 1) {
    // TODO: QUI scrivi S su 7SEGM
    doSample();
  }

  // RGBDigit functions
  processRGBStep();

  // UNGATE-UNTRIG
  unGateTrig(); // TODO: also untrig the 7SEGM LEDs? Maybe for 2nd digit so I can see the tempo

  // PROCESS ANY TS GATHERED IF ANY
  processTimestamps();
}


uint8_t scaleQuantizer(uint8_t in) {
  in = min(in, 60);
  int oct = floor(in / 12);
  int grade = in % 12;
  grade = (scaleList[currScale])[grade];
  return grade = grade + (oct * 12);
}
