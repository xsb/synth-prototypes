# percussion-synth

percussion-synth is a digital percussion synthesizer powered by a <a href="https://www.electro-smith.com/daisy/daisy">Daisy Seed</a> microcontroller on a <a href="https://www.synthux.academy/simple">Simple board</a>. This prototype was built during the <a href="https://www.synthux.academy/workshops">Synth Design Academy course</a> in Berlin on Summer 2022.

## Arcuitecture and design

The synthesizer is slightly inspired by the <a href="https://www.moogmusic.com/products/dfam-drummer-another-mother">Moog DFAM</a>, one of my favourite instruments of all time. Although lacking the pair of sequencers, the 2nd oscillator and the semi-modular capabilities, my design still includes the characteristic 3 envelope generators: for pitch, cut-off and amplitude.

Each of the envelopes has control over its decay time and modulation amount. Overall, a harsher sound is obtained by a combination of more harmonically rich oscillators (a saw instead of a triangle) and a pre-filter overdrive (here called "boost").

Envelopes' decay can be as long as 1.5s, and tempo clock can go as fast as 20 Hz with a trigger every 0.1 seconds.

## Demo

<a href="https://youtu.be/OEi-Y9DU0-A">Demo video</a>
