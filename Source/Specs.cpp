#include "Specs.h"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"

namespace nabra
{
static const char* VERT = "Speeds the module up. Linear response: +5V roughly doubles-to-triples the rate set by its capacitor.";
static const char* INVERT = "The opposite of vert: slows the module down (and can stall it). Vert and invert together replace the knob the Nabra never had.";

static std::vector<TypeSpec> makeSpecs()
{
    std::vector<TypeSpec> s;
    using S = Side;

    s.push_back ({ MT::Slope, "Slope", "slope", {
        { "vert",   "vert",   false, S::L, .22f, VERT },
        { "sync",   "sync",   false, S::L, .50f, "A pulse here resets both oscillators to their trough, locking them to another rhythm. Without a pulse they freewheel." },
        { "invert", "invert", false, S::L, .78f, INVERT, true },
        { "squareA","SQUARE", true,  S::T, .30f, "Upper oscillator square: 5V while the triangle rises, 0V while it falls." },
        { "angleA", "ANGLE",  true,  S::T, .70f, "Upper oscillator triangle, 0..5V. The classic slow shape to open an env input." },
        { "squareB","SQUARE", true,  S::B, .30f, "Lower (slow) oscillator square, 0/5V." },
        { "angleB", "ANGLE",  true,  S::B, .70f, "Lower (slow) oscillator triangle, 0..5V." },
        { "crestA", "CREST",  true,  S::R, .16f, "Pulse at the peak of the upper triangle. Trigger things from here." },
        { "troughA","TROUGH", true,  S::R, .39f, "Pulse at the bottom of the upper triangle: the other half of the rhythm." },
        { "troughB","TROUGH", true,  S::R, .61f, "Pulse at the bottom of the lower triangle." },
        { "crestB", "CREST",  true,  S::R, .84f, "Pulse at the peak of the lower triangle." } },
        false, {}, "angleA",
        "A dual low-frequency triangle/square oscillator - the leftmost columns that 'control all the others with slow shapes'. "
        "Each Slope holds a faster upper oscillator and a slow lower one, tuned by fixed capacitors from minute-long periods "
        "(Slope 1 lower: ~1 minute) up to a few Hz (Slope 6 upper: ~3.4 Hz). vert/invert move both at once, linearly. "
        "Modulating it quickly makes it sync-lock rather than sweep smoothly - its triangle bounds wobble too, as in the LM324 circuit.",
        "\"Slope is a dual triangle and square wave generator at low frequencies. It has two complementary inputs, vert and invert, "
        "that control the speed of both oscillators... The outputs are simply a square and triangle for each oscillator, plus two pulses "
        "that mark the crest and trough of the waveform. Thus the two equal halves of the rhythm can be sent to separate processors.\"" });

    s.push_back ({ MT::Port, "Port", "port", {
        { "inA",     "in",     false, S::L, .30f, "Upper input to be smoothed." },
        { "inB",     "in",     false, S::L, .70f, "Lower input to be smoothed." },
        { "vertA",   "vert",   false, S::T, .30f, "Makes the upper slew faster (less smoothing)." },
        { "invertA", "invert", false, S::T, .70f, "Makes the upper slew slower (more smoothing).", true },
        { "vertB",   "vert",   false, S::B, .30f, "Makes the lower slew faster." },
        { "invertB", "invert", false, S::B, .70f, "Makes the lower slew slower.", true },
        { "outA",    "OUT",    true,  S::R, .30f, "Smoothed upper signal." },
        { "outB",    "OUT",    true,  S::R, .70f, "Smoothed lower signal." } },
        false, {}, "outA",
        "Dual portamento / slew limiter: limits how fast a voltage may change. A square in becomes a trapezoid or triangle out. "
        "Feed it a MIDI GATE and it becomes an attack/release envelope; feed it a KEY and you get glide. "
        "Port 1: upper 25 V/s, lower 6 V/s. Port 2: upper 80 V/s, lower 2 V/s.",
        "\"Port is short for portamento. This one is a simple dual processor, with two inputs and two outputs. It limits the velocity "
        "of changes, thus smoothing any hard edges. Its effect would be most dramatic with a square wave as input, transforming it into a triangular shape.\"" });

    s.push_back ({ MT::Shot, "Shot", "shot", {
        { "vert",   "vert",   false, S::L, .22f, "Shortens both shots." },
        { "trig",   "trig",   false, S::L, .50f, "A pulse fires both one-shots (ignored while a shot is already high)." },
        { "invert", "invert", false, S::L, .78f, "Lengthens both shots.", true },
        { "startA", "START",  true,  S::T, .30f, "Pulse when the upper shot begins (redundant on purpose)." },
        { "endA",   "END",    true,  S::T, .70f, "Pulse when the upper shot ends - a delayed trigger." },
        { "startB", "START",  true,  S::B, .30f, "Pulse when the lower shot begins." },
        { "endB",   "END",    true,  S::B, .70f, "Pulse when the lower (longer) shot ends." },
        { "squareA","SQUARE", true,  S::R, .30f, "5V gate for the duration of the upper shot." },
        { "squareB","SQUARE", true,  S::R, .70f, "5V gate for the duration of the lower shot." } },
        false, {}, "squareA",
        "Dual 555 monostable. A trigger makes each half go high for a fixed time, then drop. Use SQUARE as a gate, END as a delayed trigger. "
        "Times (upper/lower): Shot 1 30/120 ms, Shot 2 80/330 ms, Shot 3 0.22/0.68 s, Shot 4 0.47/1.5 s.",
        "\"Shot is an example of a triggered module. Its name refers to its function as a one-shot. It shamefully uses the 555 monostable circuit, "
        "in a dual configuration. The trigger input causes each to go high for a period of time, and then reset to a low state.\"" });

    s.push_back ({ MT::Dust, "Dust", "dust", {
        { "vertA", "vert", false, S::T, .50f, "Raises the density of the upper dust." },
        { "both",  "both", false, S::L, .50f, "Raises the density of both dusts." },
        { "vertB", "vert", false, S::B, .50f, "Raises the density of the lower dust." },
        { "dustA", "DUST", true,  S::R, .30f, "Pulses at erratic, unevenly spaced times." },
        { "dustB", "DUST", true,  S::R, .70f, "Pulses at erratic times, different density." } },
        false, {}, "dustA",
        "An analog version of SuperCollider's Dust: random-timed pulses with an average density. A fast internal oscillator is sampled "
        "into two capacitor tanks which decay at a fixed rate; when a tank empties it fires and resamples. "
        "Densities (upper/lower): Dust 1 ~1.2/s and ~4/s, Dust 2 ~0.4/s and ~10/s. Great for triggering Trand, Pulse, Swoop.",
        "\"Supercollider ... included a useful opcode, called Dust, for generating pulses of uneven timing, but with an input for their average density "
        "over time... The vert inputs control the speed of decay to generate different densities of dust.\"" });

    s.push_back ({ MT::Stair, "Stair", "stair", {
        { "vert",   "vert",   false, S::L, .22f, "Bigger steps." },
        { "trig",   "trig",   false, S::L, .50f, "Each pulse climbs one step." },
        { "invert", "invert", false, S::L, .78f, "Smaller steps.", true },
        { "end",    "END",    true,  S::T, .30f, "Pulse when the staircase reaches the top and resets." },
        { "down",   "DOWN",   true,  S::T, .70f, "5V while the stair is resting at the bottom." },
        { "start",  "START",  true,  S::B, .30f, "Pulse when a new staircase begins." },
        { "up",     "UP",     true,  S::B, .70f, "5V while the staircase is climbing." },
        { "stair",  "STAIR",  true,  S::R, .30f, "The staircase voltage, 0..5V." },
        { "square", "SQUARE", true,  S::R, .70f, "Flips on every step: the trigger divided by two." } },
        false, {}, "stair",
        "A variable step-height staircase. Every trigger steps the output up; past 5V it resets to zero and fires END. "
        "Stair 1 starts with ~0.55V steps, Stair 2 with ~1.1V steps. Patch STAIR into a vert for rising arpeggio-like patterns.",
        "\"Stair is a triggered module, a variable step-height staircase generator... It has a trigger input that causes the output to step up an "
        "arbitrary amount until reaching a high point at which it is reset. The step amount is controlled by separate vert and invert inputs.\"" });

    s.push_back ({ MT::Rand, "Rand", "rand", {
        { "both",  "both", false, S::L, .50f, "Speeds up both internal clocks. Try feeding a RAND back in here." },
        { "trigA", "TRIG", true,  S::T, .50f, "Pulse each time the upper clock samples." },
        { "trigB", "TRIG", true,  S::B, .50f, "Pulse each time the lower clock samples." },
        { "randA", "RAND", true,  S::R, .30f, "Random stepped voltage 0..5V (upper)." },
        { "randB", "RAND", true,  S::R, .70f, "Random stepped voltage 0..5V (lower)." } },
        false, {}, "randA",
        "Free-running random melody: two sample-and-holds clocked by their own square oscillators, sampling a fast internal triangle. "
        "Rand 1 clocks at ~2.2 and ~3.3 Hz, Rand 2 at ~0.7 and ~5.6 Hz. RAND into a Sine's vert = an infinite rambler.",
        "\"Rand runs freely and Trand needs a triggering signal. Both types are of the sample and hold sort, with internal oscillator of arbitrary frequency... "
        "A RAND can be fed back into its both input to produce random voltages at random timings.\"" });

    s.push_back ({ MT::Trand, "Trand", "trand", {
        { "trigA", "trig", false, S::L, .30f, "Each pulse takes a new random sample (upper)." },
        { "trigB", "trig", false, S::L, .70f, "Each pulse takes a new random sample (lower)." },
        { "randA", "RAND", true,  S::T, .50f, "Upper random stepped voltage 0..5V." },
        { "randB", "RAND", true,  S::B, .50f, "Lower random stepped voltage 0..5V." },
        { "both",  "BOTH", true,  S::R, .50f, "Sum of both RANDs (0..10V)." } },
        false, {}, "both",
        "Triggered random melody - 'the more essential random melody, inspired by Serge'. Feed it Dust, a Slope CREST, or MIDI TRIG.",
        "\"Listening to a random melody, or infinite rambler, as controlling the frequency of a triangle oscillator, is a simple pleasure, as it activates "
        "melodic tropes in the listener, but instantly demolishes them or overlays them.\"" });

    s.push_back ({ MT::Swoop, "Swoop", "swoop", {
        { "vert",   "vert",   false, S::L, .22f, "Faster swoop." },
        { "trig",   "trig",   false, S::L, .50f, "Starts a swoop (ignored until the current one finishes)." },
        { "invert", "invert", false, S::L, .78f, "Slower swoop.", true },
        { "square", "SQUARE", true,  S::T, .50f, "5V for as long as the swoop is active." },
        { "swoop",  "SWOOP",  true,  S::R, .50f, "The envelope: rises to 5V, falls back, waits." },
        { "crest",  "CREST",  true,  S::B, .50f, "Pulse at the peak - chain Swoops in a row." } },
        false, {}, "swoop",
        "The Nabra's envelope: when triggered it goes up, then down, then waits (like a Serge slope). Swoop 1 lasts ~0.1 s, Swoop 2 ~0.33 s, "
        "Swoop 3 ~1 s, Swoop 4 ~3.3 s. Patch SWOOP into any env input.",
        "\"Swoop deconstructs the stereotypical ADSR... when triggered, it goes up, and then goes down, and then waits to be triggered again. "
        "It outputs a triangular shape for each swoop event, a square wave for the period of activity, and CREST pulses marking the peak moment.\"" });

    s.push_back ({ MT::Pulse, "Pulse", "pulse", {
        { "inA",   "in",  false, S::L, .30f, "Upper input: anything higher than the output charges it instantly." },
        { "inB",   "in",  false, S::L, .70f, "Lower input." },
        { "capA1", "cap", false, S::T, .20f, "Above ~2.5V, gates a 2x capacitor into the upper decay." },
        { "capA2", "cap", false, S::T, .50f, "Gates a 5x capacitor into the upper decay." },
        { "capA3", "cap", false, S::T, .80f, "Gates a 12x capacitor into the upper decay." },
        { "capB1", "cap", false, S::B, .20f, "Gates a 2x capacitor into the lower decay." },
        { "capB2", "cap", false, S::B, .50f, "Gates a 5x capacitor into the lower decay." },
        { "capB3", "cap", false, S::B, .80f, "Gates a 12x capacitor into the lower decay." },
        { "pulseA","PULSE", true, S::R, .30f, "Percussive envelope: instant attack, RC decay." },
        { "pulseB","PULSE", true, S::R, .70f, "Lower percussive envelope." } },
        false, {}, "pulseA",
        "Dual percussive envelope (peak follower): quick attack, exponential decay. The cap inputs switch extra capacitors into the decay; "
        "charge left on a switched-out capacitor comes back as a little glitching murmur when it is switched in again. "
        "Base decay: Pulse 1 ~12 ms, Pulse 2 ~30 ms, Pulse 3 ~80 ms, Pulse 4 ~0.2 s. Pulses from Dust in, PULSE out to a Sine env.",
        "\"I decided to gate different capacitors into the decay, giving a crude control of different decay times... charges are held on the gated capacitors "
        "when they are off, and when turned on again, they cause short glitching murmurs in the output.\"" });

    s.push_back ({ MT::Trang, "Trang", "trang", {
        { "vert",   "vert",   false, S::L, .20f, "Raises both pitches (linear, not 1V/oct)." },
        { "fm",     "fm",     false, S::L, .44f, "Linear frequency modulation - patch audio here." },
        { "invert", "invert", false, S::L, .68f, "Lowers both pitches.", true },
        { "key",    "key",    false, S::L, .90f, "(MIDI addition) Replaces the tuning capacitor: 1V = 100Hz. Patch a Kee/Duo KEY here to play it." },
        { "envA",   "env",    false, S::T, .70f, "Opens the upper VCA. Silent without it." },
        { "envB",   "env",    false, S::B, .70f, "Opens the lower (fifth) VCA." },
        { "trangA", "TRANG",  true,  S::T, .30f, "Raw upper triangle, always running." },
        { "trangB", "TRANG",  true,  S::B, .30f, "Raw lower triangle, a drifting fifth above." },
        { "objetA", "OBJET",  true,  S::R, .30f, "Upper triangle through its VCA. Also heard in the mix via the pot." },
        { "objetB", "OBJET",  true,  S::R, .70f, "The fifth through its VCA. Also in the mix." } },
        true, { "objetA", "objetB" }, "objetA",
        "Dual 566 triangle VCO pair tuned a fifth apart - power chords, now powerfully dissonant from 20 years of trimmer drift. "
        "Trang 1..4 sit near 65, 98, 147, 220 Hz. Both env inputs are required for sound. OBJET is mixed through the pot.",
        "\"On Trang, I wanted to make dual modules, that were tuned to a basic fifths relationship, for easy implementation of power chords. Thus there is an "
        "internal trim pot for each, that has gotten horribly off pitch over the years, but this only adds to its charm.\"" });

    s.push_back ({ MT::Noise, "Noise", "noise", {
        { "vert",   "vert",   false, S::L, .25f, "Denser noise, towards white." },
        { "env",    "env",    false, S::L, .50f, "Opens the VCA. Required for sound." },
        { "invert", "invert", false, S::L, .75f, "Sparser noise, towards crackle.", true },
        { "hiss",   "HISS",   true,  S::T, .50f, "The gentler, linear zener hiss (always on)." },
        { "noise",  "NOISE",  true,  S::B, .50f, "Fully amplified, comparator noise (always on)." },
        { "objet",  "OBJET",  true,  S::R, .50f, "NOISE through the VCA. Mixed through the pot." } },
        true, { "objet" }, "objet",
        "Zener-diode noise squared off by a comparator. Its density is a timbre control: from ~10 crackles a second to near white. "
        "Noise 1..4 start around 25, 220, 1800 and 9000 flips/s.",
        "\"The Noise modules are like Dust modules, but with a higher density, that ranges between ten per second up until it sounds somewhat like white noise... "
        "HISS differs from NOISE in that the former is the lesser amplified, more linear result from the noisy zener.\"" });

    s.push_back ({ MT::Sine, "Sine", "sine", {
        { "am",     "am",     false, S::L, .14f, "Amplitude modulation by audio (AC coupled) - ring-mod-ish shimmer." },
        { "vert",   "vert",   false, S::L, .34f, "Raises the pitch (linear). Patch a RAND here for a random melody." },
        { "fm",     "fm",     false, S::L, .54f, "Linear FM. 'Fm modulation is essential.' Try a Trang OBJET." },
        { "invert", "invert", false, S::L, .74f, "Lowers the pitch.", true },
        { "key",    "key",    false, S::L, .93f, "(MIDI addition) Replaces the tuning capacitor: 1V = 100Hz. Patch Kee KEY here." },
        { "envA",   "env",    false, S::T, .30f, "Opens the sine VCA. Required for sound." },
        { "envB",   "env",    false, S::B, .30f, "Opens the square VCA (only at its own OBJET, never the mix)." },
        { "sine",   "SINE",   true,  S::T, .70f, "Raw shaped sine, always running." },
        { "square", "SQUARE", true,  S::B, .70f, "Raw square, always running." },
        { "objetA", "OBJET",  true,  S::R, .30f, "Sine through its VCA - this is what reaches the mix." },
        { "objetB", "OBJET",  true,  S::R, .70f, "Square through its VCA (not mixed - patch it somewhere)." } },
        true, { "objetA" }, "objetA",
        "XR2206 function generator: a triangle shaped into a sine, plus a square, each with a VCA. Eight of them span bass to soprano: "
        "~41, 60, 87, 124, 185, 272, 408, 600 Hz (Sine 1..8). Without an env they are silent.",
        "\"Each OBJET is composed of a raw waveform with infinite duration, that requires an envelope; all modules by default are silent, requiring an "
        "activating signal on their VCAs... Besides shaping tone with vert and invert inputs, there is an fm input for frequency modulation, and an am input.\"" });

    s.push_back ({ MT::Wave, "Wave", "wave", {
        { "vert",   "vert",   false, S::L, .22f, "Raises the cutoff." },
        { "fm",     "fm",     false, S::L, .45f, "Audio-rate cutoff modulation." },
        { "invert", "invert", false, S::L, .68f, "Lowers the cutoff.", true },
        { "key",    "key",    false, S::L, .90f, "(MIDI addition) Sets the cutoff directly: 1V = 100Hz. With Kee KEY it rings at the played note." },
        { "in",     "in",     false, S::B, .50f, "Audio (or pulses, for pinged blips)." },
        { "hp",     "HP",     true,  S::T, .50f, "High pass output." },
        { "bp",     "BP",     true,  S::R, .30f, "Band pass output - also mixed through the pot." },
        { "lp",     "LP",     true,  S::R, .70f, "Low pass output." } },
        true, { "bp" }, "bp",
        "Resonant state-variable filter. No VCA - silent unless something comes in. Pulses make it ring into percussive blips. "
        "Wave 1..4 sit near 180, 520, 1500, 4200 Hz. The pot mixes the BP output.",
        "\"The final column is of the typical voltage controlled state variable filter found in many synthesizers. It is a filter, so it has an input, and it "
        "lacks a VCA section... It can also be triggered with pulses from the control modules, for short percussive blips.\"" });

    s.push_back ({ MT::Pow, "Pow", "pow", {
        { "pos",  "POS", true, S::T, .50f, "+5V. A steady bias: patch into an env to drone, or through a resistor cord into vert to retune." },
        { "neg",  "NEG", true, S::B, .50f, "-5V. Into vert this slows / lowers a module (like invert)." },
        { "gndL", "GND", true, S::L, .50f, "0V." },
        { "gndR", "GND", true, S::R, .50f, "0V." } },
        false, {}, "pos",
        "Power: positive, negative and ground potentials, to build bias voltages with resistor cords. Since nothing has a tuning knob, "
        "a POS through a 10k or 100k cord into vert is how you nudge a module's rate.",
        "\"They provide positive, negative, and ground outputs to create various bias signals with resistor-inline patch cords.\"" });

    s.push_back ({ MT::Out, "Out", "out", {
        { "inA", "in",  false, S::L, .30f, "Auxiliary input straight into the main mix." },
        { "inB", "in",  false, S::L, .70f, "Auxiliary input straight into the main mix." },
        { "pos", "POS", true,  S::T, .50f, "+5V reference." },
        { "neg", "NEG", true,  S::B, .50f, "-5V reference." },
        { "gnd", "GND", true,  S::R, .30f, "0V." },
        { "out", "OUT", true,  S::R, .70f, "The final mix itself - patch it back in for feedback." } },
        false, {}, "out",
        "The buffered final mix, bottom right corner. Everything mixed by a pot arrives here; the two in jacks add anything else "
        "(e.g. a Wave LP or a Sine square OBJET) to the mix.",
        "\"The Out module, in the bottom right corner of the tapestry, is where it sends sound to an external amplifier... It gives out positive and negative "
        "references, and has two auxiliary inputs.\"" });

    // ---------------- MIDI strip (new) ----------------
    s.push_back ({ MT::Kee, "Kee", "", {
        { "key",  "KEY",  true, S::R, .14f, "Pitch of the last-played note as linear volts: 1V = 100Hz (A440 = 4.4V). Includes pitch bend. Patch into a key input." },
        { "gate", "GATE", true, S::R, .38f, "5V while any key is held." },
        { "trig", "TRIG", true, S::R, .62f, "Short pulse on every note-on." },
        { "velo", "VELO", true, S::R, .86f, "Velocity of the last note, 0..5V." },
        { "off",  "OFF",  true, S::B, .50f, "Pulse when the last key is released." } },
        false, {}, "gate",
        "MIDI to CV, monophonic, last-note priority. KEY is made for the Nabra's linear world: patch it into the key input of a Sine, "
        "Trang or Wave and they play in tune. GATE into a Port gives an attack/release envelope; TRIG fires Swoops, Shots and Trands.",
        "(Not part of the 2000 tapestry - a sewn-on strip so your DAW's MIDI can join the patch.)", true });

    s.push_back ({ MT::Duo, "Duo", "", {
        { "low",   "LOW",   true, S::R, .20f, "Lowest held note (1V = 100Hz)." },
        { "lgate", "LGATE", true, S::R, .40f, "5V while any note is held." },
        { "high",  "HIGH",  true, S::R, .60f, "Highest held note (1V = 100Hz)." },
        { "hgate", "HGATE", true, S::R, .80f, "5V while two or more notes are held." },
        { "notes", "NOTES", true, S::B, .50f, "Number of held notes, 1V each." } },
        false, {}, "lgate",
        "MIDI to CV for two voices: the lowest and highest held notes. Play an interval and patch LOW and HIGH into two Sines. "
        "HGATE only opens when you hold a chord, so the second voice appears only then.",
        "(MIDI addition.)", true });

    s.push_back ({ MT::Wheel, "Wheel", "", {
        { "mod",   "MOD",   true, S::R, .20f, "Mod wheel (CC1), 0..5V." },
        { "bend",  "BEND",  true, S::R, .40f, "Pitch bend, -5..+5V." },
        { "press", "PRESS", true, S::R, .60f, "Aftertouch (channel or poly), 0..5V." },
        { "brite", "BRITE", true, S::R, .80f, "CC74 (brightness / MPE slide), 0..5V." },
        { "expr",  "EXPR",  true, S::B, .50f, "CC11 expression, 0..5V." } },
        false, {}, "mod",
        "Continuous MIDI controllers as voltages. These are 'a multiplicity of outputs' for your hands: MOD into a Wave vert, "
        "PRESS into a Dust both, BEND into a Slope sync...",
        "(MIDI addition.)", true });

    s.push_back ({ MT::Tick, "Tick", "", {
        { "beat", "BEAT", true, S::R, .20f, "Pulse every quarter note of the host transport." },
        { "sixt", "SIXT", true, S::R, .40f, "Pulse every sixteenth note." },
        { "bar",  "BAR",  true, S::R, .60f, "Pulse at every bar line." },
        { "run",  "RUN",  true, S::R, .80f, "5V while the host is playing." },
        { "ramp", "RAMP", true, S::B, .50f, "Rises 0..5V across each bar." } },
        false, {}, "beat",
        "The host's clock as pulses. Only runs while the DAW is playing (free-runs at 120 BPM in the standalone app). "
        "BEAT into a Slope sync locks the slow shapes to your song.",
        "(MIDI addition.)", true });

    for (auto& t : s)
    {
        for (auto& j : t.jacks)
        {
            t.inSlot.push_back (j.out ? -1 : t.numIn++);
            t.outSlot.push_back (j.out ? t.numOut++ : -1);
        }
    }
    return s;
}

static const std::vector<TypeSpec>& allSpecs()
{
    static const std::vector<TypeSpec> s = makeSpecs();
    return s;
}

const TypeSpec& spec (MT t) { return allSpecs()[(size_t) t]; }

int TypeSpec::jackIndex (const juce::String& id) const
{
    for (size_t i = 0; i < jacks.size(); ++i)
        if (id.equalsIgnoreCase (jacks[i].id)) return (int) i;
    return -1;
}

static std::vector<Cell> makeCells()
{
    std::vector<Cell> c;
    int count[(int) MT::Count] = {};
    auto add = [&] (MT t, int col, int row) { c.push_back ({ t, col, row, ++count[(int) t] }); };

    const MT grid[13][4] = {
        { MT::Slope, MT::Slope, MT::Port,  MT::Port  },
        { MT::Slope, MT::Slope, MT::Slope, MT::Slope },
        { MT::Shot,  MT::Shot,  MT::Shot,  MT::Shot  },
        { MT::Dust,  MT::Dust,  MT::Stair, MT::Stair },
        { MT::Rand,  MT::Rand,  MT::Trand, MT::Trand },
        { MT::Swoop, MT::Swoop, MT::Swoop, MT::Swoop },
        { MT::Pulse, MT::Pulse, MT::Pulse, MT::Pulse },
        { MT::Trang, MT::Trang, MT::Trang, MT::Trang },
        { MT::Noise, MT::Noise, MT::Noise, MT::Noise },
        { MT::Sine,  MT::Sine,  MT::Sine,  MT::Sine  },
        { MT::Sine,  MT::Sine,  MT::Sine,  MT::Sine  },
        { MT::Wave,  MT::Wave,  MT::Wave,  MT::Wave  },
        { MT::Pow,   MT::Pow,   MT::Pow,   MT::Out   } };

    add (MT::Kee, 0, 0); add (MT::Duo, 0, 1); add (MT::Wheel, 0, 2); add (MT::Tick, 0, 3);
    for (int col = 0; col < 13; ++col)
        for (int row = 0; row < 4; ++row)
            add (grid[col][row], col + 1, row);
    return c;
}

const std::vector<Cell>& cells()
{
    static const std::vector<Cell> c = makeCells();
    return c;
}

juce::String cellName (int i) { auto& c = cells()[(size_t) i]; return juce::String (spec (c.type).name) + " " + juce::String (c.num); }
juce::String cellKey (int i)  { auto& c = cells()[(size_t) i]; return juce::String (spec (c.type).name).toLowerCase() + juce::String (c.num); }

int findCell (const juce::String& key)
{
    for (int i = 0; i < (int) cells().size(); ++i)
        if (cellKey (i).equalsIgnoreCase (key)) return i;
    return -1;
}

int knobParamIndex (int cellIdx)
{
    int k = 0;
    for (int i = 0; i < (int) cells().size(); ++i)
    {
        bool has = spec (cells()[(size_t) i].type).hasKnob;
        if (i == cellIdx) return has ? k : -1;
        if (has) ++k;
    }
    return -1;
}

int knobParamCount()
{
    int k = 0;
    for (auto& c : cells()) if (spec (c.type).hasKnob) ++k;
    return k;
}

const char* cordName (Cord c)
{
    switch (c)
    {
        case Cord::Wire: return "wire";
        case Cord::Resistor: return "resistor";
        case Cord::Capacitor: return "capacitor";
        case Cord::Diode: return "diode";
    }
    return "wire";
}

const std::vector<Recipe>& recipes()
{
    static const std::vector<Recipe> r = {
        { "1. Slopes mark off sines in space",
          "Sine.env(Slope.ANGLE); Swoop.trig(Slope.CREST); Noise.env(Swoop.SWOOP)",
          "A slow triangle breathes a Sine in and out. Each time the triangle peaks, its CREST pulse fires a Swoop, and the Swoop envelopes a Noise. "
          "Two events, one rhythm, split across crest and body.",
          { "slope3.angleA>sine5.envA", "slope3.crestA>swoop2.trig", "swoop2.swoop>noise3.env" }, true },
        { "2. Dust, random melody, percussive sine",
          "Trand.trig(Dust.DUST); Sine.vert(Trand.RAND); Pulse.in(Dust.DUST); Sine.env(Pulse.OUT)",
          "Erratic Dust pulses do two jobs at once: they pick a new random pitch (Trand into the Sine's vert) and they strike a Pulse envelope that plays the Sine. "
          "An infinite rambler with uneven timing.",
          { "dust1.dustB>trand1.trigA", "trand1.randA>sine6.vert", "dust1.dustB>pulse2.inA", "pulse2.pulseA>sine6.envA" }, true },
        { "3. Trang FM, random-tuned",
          "Trang.env(Slope.SQUARE); Trang.vert(Rand.RAND); Sine.fm(Trang.OBJET); Sine.env(Slope.ANGLE)",
          "A Slope square gates a Trang whose pitch wanders under a Rand. That Trang frequency-modulates a Sine which a slower Slope breathes open. "
          "'Fm modulation is essential.'",
          { "slope5.squareA>trang2.envA", "rand1.randA>trang2.vert", "trang2.objetA>sine4.fm", "slope2.angleA>sine4.envA" }, true },
        { "4. Kee sings (MIDI)",
          "Sine.key(Kee.KEY); Trang.key(Kee.KEY); Port.in(Kee.GATE); Sine.env(Port.OUT); Trang.env(Port.OUT); Trang.env(Port.OUT ~10k~); Sine.fm(Trang.OBJET ~100k~)",
          "Play your keyboard. KEY tunes a Sine and a Trang (which adds its drifting fifth at half level, through a 10k resistor cord); GATE is smoothed by a Port into an attack/release envelope "
          "for both; a little of the Trang frequency-modulates the Sine through a 100k resistor cord.",
          { "kee1.key>sine5.key", "kee1.key>trang1.key", "kee1.gate>port1.inA", "port1.outA>sine5.envA", "port1.outA>trang1.envA", "port1.outA>trang1.envB:r1",
            "trang1.objetA>sine5.fm:r2" }, false },
        { "5. Duo over dust (MIDI)",
          "Sine.key(Duo.LOW); Sine.key(Duo.HIGH); Port.in(Duo.LGATE / HGATE); Wave.in(Dust.DUST); Wave.key(Duo.HIGH)",
          "Hold intervals or chords. The lowest note plays Sine 3 with a quick envelope, the highest swells in slowly on Sine 7 only while you hold two or more notes. "
          "Meanwhile Dust pings Wave 3, tuned to your top note, like rain on a resonator.",
          { "duo1.low>sine3.key", "duo1.high>sine7.key", "duo1.lgate>port2.inA", "port2.outA>sine3.envA", "duo1.hgate>port2.inB", "port2.outB>sine7.envA",
            "dust2.dustB>wave3.in", "duo1.high>wave3.key" }, false },
        { "6. Tick: the host's clock",
          "Swoop.trig(Tick.BEAT); Noise.env(Swoop.SWOOP); Trand.trig(Tick.SIXT); Sine.vert(Trand.RAND); Pulse.in(Tick.SIXT); Sine.env(Pulse.PULSE)",
          "Press play in your DAW. Beats swoop a noise; sixteenths pick random pitches and strike a Pulse on a Sine; once a bar a Shot switches an extra capacitor "
          "into the Pulse so the decays stretch.",
          { "tick1.beat>swoop1.trig", "swoop1.swoop>noise4.env", "tick1.sixt>trand2.trigA", "trand2.randA>sine5.vert", "tick1.sixt>pulse1.inA",
            "pulse1.pulseA>sine5.envA", "tick1.bar>shot3.trig", "shot3.squareA>pulse1.capA2" }, false },
        { "7. Pinged filter (MIDI)",
          "Wave.in(Kee.TRIG ~cap~); Wave.key(Kee.KEY); Wave.vert(Wheel.MOD)",
          "Each note-on pulse, sharpened by a capacitor cord, strikes a resonant Wave tuned to the key: a plucked, woody tone. The mod wheel opens it up.",
          { "kee1.trig>wave2.in:c", "kee1.key>wave2.key", "wheel1.mod>wave2.vert:r1" }, false },
    };
    return r;
}

std::vector<Cable> buildRecipe (const Recipe& r)
{
    std::vector<Cable> out;
    int id = 1, colour = 0;
    for (auto* l : r.links)
    {
        juce::String s (l);
        juce::String opts = s.fromFirstOccurrenceOf (":", false, false);
        s = s.upToFirstOccurrenceOf (":", false, false);
        auto a = s.upToFirstOccurrenceOf (">", false, false), b = s.fromFirstOccurrenceOf (">", false, false);
        auto parse = [] (const juce::String& x) -> JackRef {
            JackRef j;
            j.cell = findCell (x.upToFirstOccurrenceOf (".", false, false));
            if (j.cell >= 0) j.jack = spec (cells()[(size_t) j.cell].type).jackIndex (x.fromFirstOccurrenceOf (".", false, false));
            return j;
        };
        Cable c;
        c.src = parse (a);
        c.dst = parse (b);
        jassert (c.src.valid() && c.dst.valid());
        if (! c.src.valid() || ! c.dst.valid()) continue;
        c.id = id++;
        c.colour = colour++;
        if (opts.startsWith ("r")) { c.cord = Cord::Resistor; c.resVal = opts.substring (1).getIntValue(); }
        else if (opts == "c") c.cord = Cord::Capacitor;
        else if (opts == "d") c.cord = Cord::Diode;
        out.push_back (c);
    }
    return out;
}
} // namespace nabra
