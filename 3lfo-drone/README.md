# 3lfo-drone

3lfo-drone is a digital drone synthesizer powered by a <a href="https://www.electro-smith.com/daisy/daisy">Daisy Seed</a> microcontroller on a <a href="https://www.synthux.academy/simple">Simple board</a>. This prototype was built during the <a href="https://www.synthux.academy/workshops">Synth Design Academy course</a> in Berlin on Summer 2022.

## Architecture and design

The synthesizer has 3 LFOs, each with rate and amount control, and each modulating a separate component of a basic substractive synth arquitecture: pitch, filter cut-off and volume. Additionally to a sine wave, each LFO includes a secondary wave form: pitch has a noise sourced sampe & hold, cut-off has a square wave and volume has a saw (emulating a decay-only envelope generator).

The mixer includes a main oscillator, a super sub-oscillator (hardcoded 2 octaves below), a white noise generator and a distortion effects chain with overdrive and bit-crushing that alters the resulting signal before entering the filter.

## Demo

<a href="https://youtu.be/Lgjg0PZAc4Q">Demo video</a>
