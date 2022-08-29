#include "DaisyDuino.h"

// Ranges and values
const bool DEBUG = false;
const int LFO_MAX_RATE = 20;
const int OSC_MIN_FREQ = 40;
const int OSC_MAX_FREQ = 2000;
const int FILTER_MAX_CUTOFF = 10000;
const float SUB_OSC_AMOUNT = 0.3; // 30% volume
const float INIT_LFO_AMP = 0.5;

// Pots & switches
int potPitch = A11;
int potPitchLFORate = A10;
int potPitchLFOAmount = A9;
int potNoise = A8;
int potCutOff = A7;
int potCutOffLFORate = A6;
int potCutOffLFOAmount = A5;
int potResonance = A4;
int potAmp = A3;
int potAmpLFORate = A2;
int potAmpLFOAmount = A1;
int potDriveCrush = A0;
int switchPitchLFO = 0;
int switchCutOffLFO = 1;
int switchAmpLFO = 2;

// Globals
DaisyHardware hw;
static Oscillator oscillator, subOscillator;
static Oscillator pitchLFO, cutOffLFO, ampLFO;
static WhiteNoise noise;
static Overdrive overdrive;
static Bitcrush bitcrush;
static MoogLadder filter;
float sampleRate;
float oscillatorFreq, oscillatorFreqMod, noiseAmount, filterFreq;
float pitchLFOAmount, cutOffLFOAmount, ampLFOAmount;
float driveCrushAmount, masterVolume;
bool debugMode = false;
unsigned long lastTime = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(switchPitchLFO, INPUT);
  pinMode(switchCutOffLFO, INPUT);
  pinMode(switchAmpLFO, INPUT);
  
  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  sampleRate = DAISY.get_samplerate();

  oscillator.Init(sampleRate);
  oscillator.SetWaveform(Oscillator::WAVE_SAW);
  oscillator.SetAmp(1.0);
  subOscillator.Init(sampleRate);
  subOscillator.SetWaveform(Oscillator::WAVE_SQUARE);
  subOscillator.SetAmp(1.0);

  noise.Init();
  noise.SetAmp(1.0);

  overdrive.Init();
  bitcrush.Init(sampleRate);
  
  pitchLFO.Init(sampleRate);
  pitchLFO.SetWaveform(Oscillator::WAVE_SIN);
  oscillator.SetAmp(INIT_LFO_AMP);

  cutOffLFO.Init(sampleRate);
  oscillator.SetAmp(INIT_LFO_AMP);

  ampLFO.Init(sampleRate);
  oscillator.SetAmp(INIT_LFO_AMP);

  filter.Init(sampleRate);

  DAISY.begin(AudioCallback);
}

void AudioCallback(float **in, float **out, size_t size) {
  float oscSample, noiseSample, mixSample, driveSample, crushSample, filterSample;
  float pitchLFOSample, SHSample, cutOffLFOSample, ampLFOSample;
  float filterFreqMod, volume;

  for (size_t i = 0; i < size; i++) {

    // Pitch LFO adds between 0Hz and 1000Hz.
    pitchLFOSample = fmap(pitchLFO.Process() + 0.5, 0, 1000);

    // We use this white noise generator in the mixer, but also
    //as a source of randomness for the Sample&Hold.
    noiseSample = noise.Process();
    
    // Sine-wave LFO mode decetced.
    if (digitalRead(switchPitchLFO)) {
      oscillatorFreqMod = oscillatorFreq + pitchLFOSample * pitchLFOAmount;
    
    // S/H mode detected.
    // Using the LFO end-of-cicle to initiate a "value hold".
    // Nothig else is changed here until the next cycle ends.
    } else if (pitchLFO.IsEOC()) {
      SHSample = fmap(noiseSample + 1.0, 0, 1000);
      oscillatorFreqMod = oscillatorFreq + SHSample * pitchLFOAmount;
    }

    // Mix main osc with sub-osc (2 octaves below!).
    oscillator.SetFreq(oscillatorFreqMod);
    subOscillator.SetFreq(oscillatorFreqMod / 4.0);
    oscSample = oscillator.Process() * (1 - SUB_OSC_AMOUNT) + subOscillator.Process() * SUB_OSC_AMOUNT;

    // Increasing the noise amount decreases the oscillators volume and nice-versa.
    mixSample = oscSample * (1 - noiseAmount) + noiseSample * noiseAmount;

    // Pre-filter distortion effects. I love how this sounds btw.
    driveSample = mixSample * (1 - driveCrushAmount) + overdrive.Process(mixSample) * driveCrushAmount;
    crushSample = driveSample * (1 - driveCrushAmount) + bitcrush.Process(driveSample) * driveCrushAmount;

    // Let's add some modulation to the filter.
    cutOffLFOSample = fmap(cutOffLFO.Process() + 0.5, 0, FILTER_MAX_CUTOFF);
    filterFreqMod = filterFreq + cutOffLFOSample * cutOffLFOAmount;
    filter.SetFreq(filterFreqMod);
    filterSample = filter.Process(crushSample);

    // Let's add some modulation to the volume.
    ampLFOSample = ampLFO.Process() + 0.5;
    volume = fmap(masterVolume * (1 - ampLFOAmount) + ampLFOSample * ampLFOAmount, 0.0, 1.0);
    
    out[0][i] = filterSample * volume;
  }
}

void loop() {
  float pitchLFORate, cutOffLFORate, ampLFORate;

  // When DEBUG=true, switch to debug mode (meaning "print to serial") every second.
  debugMode = false;
  if (DEBUG) {
    if (millis() - lastTime >= 1000) {
      debugMode = true;
      Serial.println();
      lastTime = millis();
    }
  }

  if (digitalRead(switchCutOffLFO)) {
    cutOffLFO.SetWaveform(Oscillator::WAVE_SIN);
  } else {
    cutOffLFO.SetWaveform(Oscillator::WAVE_SQUARE);
  }

  if (digitalRead(switchAmpLFO)) {
    ampLFO.SetWaveform(Oscillator::WAVE_SIN);
  } else {
    ampLFO.SetWaveform(Oscillator::WAVE_SAW);
  }
  
  // Set oscillator frequency and modulation values.
  oscillatorFreq = fmap(simpleAnalogRead(potPitch), OSC_MIN_FREQ, OSC_MAX_FREQ, Mapping::EXP);

  pitchLFORate = fmap(simpleAnalogRead(potPitchLFORate), 0, LFO_MAX_RATE, Mapping::EXP);
  pitchLFO.SetFreq(pitchLFORate);  
  pitchLFOAmount = fmap(simpleAnalogRead(potPitchLFOAmount), 0.0, 1.0, Mapping::EXP);

  noiseAmount = simpleAnalogRead(potNoise);

  // Set up distortion effects values.
  driveCrushAmount = fmap(simpleAnalogRead(potDriveCrush), 0.0, 1.0, Mapping::EXP);
  overdrive.SetDrive(1.0);
  bitcrush.SetBitDepth(4);
  bitcrush.SetCrushRate(sampleRate - driveCrushAmount * sampleRate * 0.9);

  // Set filter cut-off frequency and modulation values.
  filterFreq = fmap(simpleAnalogRead(potCutOff), 0, FILTER_MAX_CUTOFF, Mapping::EXP);
  filter.SetRes(simpleAnalogRead(potResonance) * 0.9);
  cutOffLFORate = fmap(simpleAnalogRead(potCutOffLFORate), 0, LFO_MAX_RATE, Mapping::EXP);
  cutOffLFO.SetFreq(cutOffLFORate);
  cutOffLFOAmount = fmap(simpleAnalogRead(potCutOffLFOAmount), 0.0, 1.0, Mapping::EXP);;

  // Set master volume and amplitude modulation values.
  masterVolume = simpleAnalogRead(potAmp);
  ampLFORate = fmap(simpleAnalogRead(potAmpLFORate), 0, LFO_MAX_RATE, Mapping::EXP);
  ampLFO.SetFreq(ampLFORate);
  ampLFOAmount = fmap(simpleAnalogRead(potAmpLFOAmount), 0.0, 1.0, Mapping::EXP);;
}

// Reads a Simple board pot (inverted) and normalizes it to values between 0 and 1.
// We also print from here when in debug mode.
float simpleAnalogRead(uint32_t pin) {
  float value = (1023.0 - (float)analogRead(pin)) / 1023.0;
  if (debugMode) {
    Serial.println(String(pin) + " = " + String(value));
  }
  return value;
}
