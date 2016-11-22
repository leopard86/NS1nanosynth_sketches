NSEQUENCER firmware v3: (pronounc.:"ènzequènzer 3" in Ancona vernacular)
     for
Soundmachines NS-1 nanosynth, based on Arduino Leonardo
  authored by
Leonardo Gabrielli <leonardo.o.gabrielli@gmail.com>

Description:
MIDI+VOLTAGE TRIG Step sequencer for the NS-1 with variable stuttering and reset

Features:
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

Technical note: no HW timers have been used on this new version. This makes the code 
look like dummy stuff with most timing embedded in the loop(). 
This is due to Leonardo timers libraries being faulty, crashing and smashing.
I just kept compatibility with previous code using timers by implementing a class for handling
a fake timer.

Suggested configuration (beginners):
 * pitch out to V/oct
 * mod to whatever you wish (e.g. filter)
 * Button 1 for TAPPING TEMPO or a LFO square to IN_CLK
 * Button 2 to IN_SAMPLE
 * Button 3 to IN_RETRIG or IN_RST
 * CTRL1 and CTRL2 to pitch and mod in, or...
 * a variable voltage of your choice to IN_TEMPOMUL for regulating stuttering interval
 * OUT_GATE or OUT_TRIG (or OUT_LED) to GATE input of the ADSR
 * Then create a simple VCO-VCF-VCA patch with your NS-1
 
Go creative:
 * turn a variable voltage into your IN_SAMPLE 
 * make the pitch out control your filter frequency with the filter in sel-oscillation
 * crank up your brain, this part is up to you!
 
ToDo:
 * Glide?
 * Selectable musical scale by external analog in
