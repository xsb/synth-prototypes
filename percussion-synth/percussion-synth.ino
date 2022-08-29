#include "DaisyDuino.h"

// Ranges and values
const int OSC_MIN_FREQ = 30;
const int OSC_MAX_FREQ = 400;
const int FILTER_MAX_CUTOFF = 8000;
const float PITCH_MOD_MAX = 300.0;
const float CUTOFF_MOD_MAX = 2000.0;
const float TEMPO_MIN_HZ = 0.3; // = intervals of 3.3~s
const float TEMPO_MAX_HZ = 20.0; // = intervals of 0.1s
const float ATTACK_TIME = 0.03;
const float DECAY_MAX_TIME = 1.5;

// Pots & switches
int potPitch = A11;
int potPitchEGAmount = A10;
int potPitchDecay = A9;
int potNoise = A8;
int potCutOff = A7;
int potCutOffEGAmount = A6;
int potCutOffDecay = A5;
int potResonance = A4;
int potAmp = A3;
int potAmpEGAmount = A2;
int potAmpDecay = A1;
int potTempo = A0;
int switchOscillatorWaveform = 0;
int switchFilterMode = 1;
int switchOverdrive = 2;
int switchPlayStop = 12;

// Globals
DaisyHardware hw;
static Oscillator oscillator;
static WhiteNoise noise;
static Overdrive overdrive;
static Svf filter;
static AdEnv pitchEG, cutOffEG, ampEG;
static Metro metro;
float volumeLevel, oscFreq, cutOffFreq, noiseAmount;
bool lowPassMode;
bool overdriveEnabled;
bool playModeEnabled;

void setup() {
  float sampleRate;

  pinMode(switchOscillatorWaveform, INPUT_PULLUP);
  pinMode(switchFilterMode, INPUT_PULLUP);
  pinMode(switchOverdrive, INPUT_PULLUP);
  pinMode(switchPlayStop, INPUT_PULLUP);

  hw = DAISY.init(DAISY_SEED, AUDIO_SR_48K);
  sampleRate = DAISY.get_samplerate();

  oscillator.Init(sampleRate);
  oscillator.SetWaveform(Oscillator::WAVE_SAW);
  oscillator.SetAmp(1.0);

  noise.Init();
  noise.SetAmp(1.0);

  overdrive.Init();
  overdrive.SetDrive(0.5);

  filter.Init(sampleRate);

  pitchEG.Init(sampleRate);
  pitchEG.SetMin(0.0);
  pitchEG.SetMax(1.0);
  pitchEG.SetTime(ADENV_SEG_ATTACK, 0.01);
  pitchEG.SetCurve(0);

  cutOffEG.Init(sampleRate);
  cutOffEG.SetMin(0.0);
  cutOffEG.SetMax(1.0);
  cutOffEG.SetTime(ADENV_SEG_ATTACK, 0.01);
  cutOffEG.SetCurve(0);

  ampEG.Init(sampleRate);
  ampEG.SetMin(0.0);
  ampEG.SetMax(1.0);
  ampEG.SetTime(ADENV_SEG_ATTACK, 0.01);
  ampEG.SetCurve(0);

  metro.Init(2, sampleRate);

  DAISY.begin(AudioCallback);
}

void AudioCallback(float **in, float **out, size_t size) {
  float oscSample, noiseSample, mixSample, filterSample, outputSample ;

  for (size_t i = 0; i < size; i++) {
    
    // Trigger all envelopes if the metronome ticks while on play mode.
    if(metro.Process() && playModeEnabled) {
      pitchEG.Trigger();
      cutOffEG.Trigger();
      ampEG.Trigger();
    }

    // Add pitch envelope amount to pitch frequency.
    // Envelope values between 0Hz and PITCH_MOD_MAX.
    oscillator.SetFreq(oscFreq + pitchEG.Process());
    oscSample = oscillator.Process();

    // Increasing the noise amount decreases the oscillator volume and nice-versa
    noiseSample = noise.Process();
    mixSample = oscSample * (1 - noiseAmount) + noiseSample * noiseAmount;
    
    // Add cut-off envelope amount to cut-off frequency.
    // Envelope values between 0Hz and CUTOFF_MOD_MAX.
    // But when on High-Pass mode, we substract the value instead, making it a negative envelope.
    if (lowPassMode) {
      filter.SetFreq(cutOffFreq + cutOffEG.Process());
      filter.Process(mixSample);
      filterSample = filter.Low();
    } else {
      filter.SetFreq(cutOffFreq - cutOffEG.Process());
      filter.Process(mixSample);
      filterSample = filter.High();
    }

    if (overdriveEnabled) {
      outputSample = overdrive.Process(filterSample);
    } else {
      outputSample = filterSample;
    }

    // Volume is affected by amplitude envelope.
    out[0][i] = outputSample * volumeLevel * ampEG.Process();
  }
}

void loop() {
  if (digitalRead(switchOscillatorWaveform)) {
    oscillator.SetWaveform(Oscillator::WAVE_SQUARE);
  } else {
    oscillator.SetWaveform(Oscillator::WAVE_SAW);
  }

  if (digitalRead(switchFilterMode)) {
    lowPassMode = true;
  } else {
    lowPassMode = false;
  }

  if (digitalRead(switchOverdrive)) {
    overdriveEnabled = false;
  } else {
    overdriveEnabled = true;
  }

  if (digitalRead(switchPlayStop)) {
    playModeEnabled = false;
  } else {
    playModeEnabled = true;
  }

  // Set sequencer tempo by changing the "metronome" frequency (in Hz).
  metro.SetFreq(fmap(simpleAnalogRead(potTempo), TEMPO_MIN_HZ, TEMPO_MAX_HZ, Mapping::EXP));

  // Set oscillator frequency, decay time and envelope amount.
  oscFreq = fmap(simpleAnalogRead(potPitch), OSC_MIN_FREQ, OSC_MAX_FREQ, Mapping::EXP);
  pitchEG.SetTime(ADENV_SEG_DECAY, fmap(simpleAnalogRead(potPitchDecay), ATTACK_TIME, DECAY_MAX_TIME / 2, Mapping::EXP));
  pitchEG.SetMax(fmap(simpleAnalogRead(potPitchEGAmount), 0.0, PITCH_MOD_MAX, Mapping::EXP));

  noiseAmount = fmap(simpleAnalogRead(potNoise), 0.0, 1.0, Mapping::EXP);

  // Set filter cut-off frequency, resonance, decay time and envelope amount.
  cutOffFreq = fmap(simpleAnalogRead(potCutOff), 0, FILTER_MAX_CUTOFF, Mapping::EXP);
  filter.SetRes(simpleAnalogRead(potResonance) * 0.9);
  cutOffEG.SetTime(ADENV_SEG_DECAY, fmap(simpleAnalogRead(potCutOffDecay), ATTACK_TIME, DECAY_MAX_TIME, Mapping::EXP));
  cutOffEG.SetMax(fmap(simpleAnalogRead(potCutOffEGAmount), 0.0, CUTOFF_MOD_MAX, Mapping::EXP));

  // Set volume level, decay time and envelope amount.
  volumeLevel = simpleAnalogRead(potAmp);
  ampEG.SetTime(ADENV_SEG_DECAY, fmap(simpleAnalogRead(potAmpDecay), ATTACK_TIME, DECAY_MAX_TIME, Mapping::EXP));
  ampEG.SetMax(fmap(simpleAnalogRead(potAmpEGAmount), 0.0, 1.0, Mapping::EXP));
}

// Reads a Simple board pot (inverted) and normalizes it to values between 0 and 1.
float simpleAnalogRead(uint32_t pin) {
  return (1023.0 - (float)analogRead(pin)) / 1023.0;
}
