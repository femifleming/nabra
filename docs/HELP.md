# Nabra help

This is the full guide. The same text, with pictures, is inside the app: click **? HELP** in the top bar, or right-click any board.

- [Start here](#start-here)
- [Patching](#patching)
- [Cords with parts in them](#cords-with-parts-in-them)
- [MIDI to CV](#midi-to-cv)
- [Recipes](#recipes)
- [Themes and display](#themes-and-display)
- [Using the desktop app](#using-the-desktop-app)
- [Troubleshooting](#troubleshooting)
- [Module reference](#module-reference)

## Start here

Nabra rebuilds the **Nabra**, the hanging tapestry synthesizer described in Peter Blasser's thesis
**[Stores at the Mall](https://digitalcollections.wesleyan.edu/documents/detail/stores-at-the-mall/213035)** (Wesleyan, 2015).
Fifty-two circuit boards face out of a splatter-painted canvas. Loops of braided copper act as patch points, joined by alligator-clip cords.

> "It is basically a modular synthesizer, with a few differences... It has no knobs for tuning... tuning is a given for each module,
> and the only way to change it is by modulation. However, there are knobs for volume of the audio modules, so they can be mixed on site."

The columns run like a sentence, left to right: **control → process → generate → filter**. On the left, slow shapes (Slope, Shot, Dust,
Stair, Rand, Trand, Swoop, Pulse) drive the sound makers (Trang, Noise, Sine). Those can run through the filters (Wave) into the mix (Out, bottom right).
On the far left, a strip of raw canvas has been sewn on. This is the **MIDI strip**, which turns your notes into voltages.

### Your first patch, in 30 seconds

1. Click **UNPATCH ALL** in the top bar. The tapestry goes empty and silent.
2. Find **Slope 3** (second column from the left of the painted canvas, top). Press on the copper loop labelled **ANGLE** on its top edge and drag. A cord follows your mouse.
3. Let go on the loop labelled **env** at the top of **Sine 5**. The Sine now breathes in and out with the slow triangle.
4. Drag from Slope 3's **CREST** to **trig** on **Swoop 2**, then from Swoop 2's **SWOOP** to **env** on **Noise 3**. That's the first recipe in the thesis.
5. Hover over any loop to see what it does, with a live trace of its voltage. Hover over a module's black name tape to see its diagram from the thesis.

> Every audio module is **silent until something opens its env input**. If you hear nothing, an env is missing.
> The little knob on each audio board sets its volume in the mix.

## Patching

- **Make a connection:** press on a copper loop and drag to another loop. One end must be an **OUTPUT** (UPPERCASE, white tape) and the other an **input** (lowercase, cream tape). You can start from either end.
- **Black tape = invert.** Almost every module has a **vert** input (speeds it up or raises it) and an **invert** input (slows it down or lowers it). There are no knobs, so this pair is how you tune anything.
- **Many into one:** patch as many outputs into one input as you like. They add together.
- **One into many:** an output can feed any number of inputs. Drag from it again.
- **Move a cord:** press on an input loop that already has a cord and drag it away. You pick up the most recent cord. Drop it on another input to re-route it, or on bare canvas to remove it. Hold **Shift** to start a new cord from an occupied input instead.
- **Remove a cord:** double-click it, or right-click it for a menu. The menu also lets you change the cord's in-line part or resistor value. Right-clicking a loop lists all its cords.
- **Never cross two outputs.** The Nabra has hot outputs with no current-limiting resistors, so Nabra won't connect an output to another output.
- **Feedback is allowed.** Patch a module into itself, or the final **OUT** back into something.
- **Undo:** **⌘Z** on macOS, **Ctrl+Z** on Windows and Linux, or the **UNDO** button.

**Voltages.** Control signals swing **0..5V**. Pulses are **5V for 1.5 ms**, and anything above about 2V counts as a trigger. Audio swings about **±5V**.
All rate inputs are **linear**, as in the original. +5V into vert roughly doubles-to-triples a module's speed, and enough invert can stall it.
Pow gives you fixed +5V / −5V to bias things.

## Cords with parts in them

> "I designed the Nabra for patch cords with in-line passive elements, such as resistors, capacitors, or even diodes. I intended for compositional strategy
> to include the type of cord, and thus the type of mixing in the connection of outputs."

Choose the cord in your hand from the top bar before you patch. Right-click any existing cord to change it.

| Cord | Effect |
|---|---|
| **Wire** | Plain connection, full strength. |
| **Resistor** 1k / 10k / 100k | Scales the signal down: 1k passes ~90%, 10k half, 100k about a tenth. Click the resistor button again to change its value. Pow POS through 100k into a vert works as a fine-tuning knob. |
| **Capacitor** | Passes changes and blocks steady voltage. A square becomes spikes, and a gate becomes a single pulse. |
| **Diode** | Lets only positive voltage through, minus a 0.6V drop. It half-wave rectifies audio, and lets several envelopes share an input. |

## MIDI to CV

Put Nabra on an instrument track and send it MIDI, or pick a MIDI input in the desktop app. Four boards on the sewn-on strip turn it into voltages:

- **Kee:** monophonic, last-note priority. **KEY** gives pitch, **GATE** stays high while a key is held, **TRIG** fires on every note, plus **VELO**, and **OFF** on release.
- **Duo:** the **LOW**est and **HIGH**est held notes, each with its own gate.
- **Wheel:** mod wheel, pitch bend, aftertouch, CC74 and CC11, each as 0..5V.
- **Tick:** the host transport, with BEAT, SIXTeenth and BAR pulses, a RUN gate, and a RAMP across each bar.

**Playing in tune.** The oscillators respond linearly, so the strip uses **linear volts: 1V = 100Hz** (A440 = 4.4V).
Sine, Trang and Wave each have an extra **key** loop. Patch a KEY there and the module plays the note.

- **Simplest instrument:** Kee **KEY** → Sine **key**, then Kee **GATE** → Sine **env**.
- **Softer envelope:** route GATE through a **Port** before the env.
- **Glide:** route KEY through the other half of the Port.
- **Pluck:** Kee **TRIG** → Wave **in** (use a capacitor cord), then Kee **KEY** → Wave **key**.
- **Clocked:** Tick **BEAT** → Slope **sync**, or SIXT → Trand trig.

## Recipes

Pick one from **RECIPES** in the top bar. The thesis writes patches as **Module.input(Module.OUTPUT)**.
The first three recipes are from the thesis. The rest use the MIDI strip. Recipe 4 loads at startup.

**1. Slopes mark off sines in space**
`Sine.env(Slope.ANGLE); Swoop.trig(Slope.CREST); Noise.env(Swoop.SWOOP)`
A slow triangle breathes a Sine in and out. Each time the triangle peaks, its CREST pulse fires a Swoop, and the Swoop envelopes a Noise. Two events, one rhythm, split across crest and body.

**2. Dust, random melody, percussive sine**
`Trand.trig(Dust.DUST); Sine.vert(Trand.RAND); Pulse.in(Dust.DUST); Sine.env(Pulse.OUT)`
Erratic Dust pulses do two jobs at once: they pick a new random pitch (Trand into the Sine's vert) and they strike a Pulse envelope that plays the Sine. An infinite rambler with uneven timing.

**3. Trang FM, random-tuned**
`Trang.env(Slope.SQUARE); Trang.vert(Rand.RAND); Sine.fm(Trang.OBJET); Sine.env(Slope.ANGLE)`
A Slope square gates a Trang whose pitch wanders under a Rand. That Trang frequency-modulates a Sine which a slower Slope breathes open. 'Fm modulation is essential.'

**4. Kee sings (MIDI)**
`Sine.key(Kee.KEY); Trang.key(Kee.KEY); Port.in(Kee.GATE); Sine.env(Port.OUT); Trang.env(Port.OUT); Trang.env(Port.OUT ~10k~); Sine.fm(Trang.OBJET ~100k~)`
Play your keyboard. KEY tunes a Sine and a Trang (which adds its drifting fifth at half level, through a 10k resistor cord); GATE is smoothed by a Port into an attack/release envelope for both; a little of the Trang frequency-modulates the Sine through a 100k resistor cord.

**5. Duo over dust (MIDI)**
`Sine.key(Duo.LOW); Sine.key(Duo.HIGH); Port.in(Duo.LGATE / HGATE); Wave.in(Dust.DUST); Wave.key(Duo.HIGH)`
Hold intervals or chords. The lowest note plays Sine 3 with a quick envelope, the highest swells in slowly on Sine 7 only while you hold two or more notes. Meanwhile Dust pings Wave 3, tuned to your top note, like rain on a resonator.

**6. Tick: the host's clock**
`Swoop.trig(Tick.BEAT); Noise.env(Swoop.SWOOP); Trand.trig(Tick.SIXT); Sine.vert(Trand.RAND); Pulse.in(Tick.SIXT); Sine.env(Pulse.PULSE)`
Press play in your DAW. Beats swoop a noise; sixteenths pick random pitches and strike a Pulse on a Sine; once a bar a Shot switches an extra capacitor into the Pulse so the decays stretch.

**7. Pinged filter (MIDI)**
`Wave.in(Kee.TRIG ~cap~); Wave.key(Kee.KEY); Wave.vert(Wheel.MOD)`
Each note-on pulse, sharpened by a capacitor cord, strikes a resonant Wave tuned to the key: a plucked, woody tone. The mod wheel opens it up.

## Themes and display

- **COLOUR** switches the boards between the black-and-white photographs from the thesis (the default) and colour scans of the real circuit boards. Nabra remembers your choice with the project or session.
- **LABELS** shows or hides the tape labels on every loop. Turn them off for the unlabelled, "intentionally unintelligible" original.
- Resize the window from the bottom-right corner. Nabra re-renders sharply at any size.

## Using the desktop app

The desktop app (macOS `Nabra.app`, Windows `Nabra.exe`, Linux `Nabra`) is the full instrument. It doesn't need a DAW.

- Click **Options** (top-left of the window) to choose your audio output device, sample rate, buffer size and **MIDI input**.
- Without a MIDI keyboard you can still patch the free-running modules (Slopes, Dust, Rand...) and hear it.
- Your patch is saved automatically when you quit. Use **Options → Save/Load current state** to keep patches as files.

## Troubleshooting

| Problem | Fix |
|---|---|
| No sound | Check that an **env** input is patched, the board's volume knob and **VOLUME** are up, and (in the app) **Options** has an output device. |
| My DAW doesn't see the plug-in | Rescan plug-ins. The locations are: macOS `~/Library/Audio/Plug-Ins/{VST3,Components}`, Windows `C:\Program Files\Common Files\VST3`, Linux `~/.vst3` / `~/.lv2`. With Homebrew on Linux, run `nabra-link-plugins`. |
| macOS says "Nabra is damaged" | Run `xattr -dr com.apple.quarantine /Applications/Nabra.app` (and the same for the plug-ins). The Homebrew cask does this for you. |
| Windows SmartScreen warning | The builds aren't code-signed yet. Click **More info → Run anyway**. |
| Linux: no MIDI or audio | Make sure ALSA works or JACK is running, then pick devices under **Options**. |
| Sound is out of tune | Patch **KEY** into the **key** loop, not into **vert**. vert bends the pitch linearly around the board's own tuning. |

## Module reference

Each module type appears several times on the tapestry (Slope 1–6, Sine 1–8...). Same-numbered boards share a circuit but differ in capacitor tuning.
**UPPERCASE** loops are outputs and lowercase loops are inputs.

### Slope

A dual low-frequency triangle/square oscillator - the leftmost columns that 'control all the others with slow shapes'. Each Slope holds a faster upper oscillator and a slow lower one, tuned by fixed capacitors from minute-long periods (Slope 1 lower: ~1 minute) up to a few Hz (Slope 6 upper: ~3.4 Hz). vert/invert move both at once, linearly. Modulating it quickly makes it sync-lock rather than sweep smoothly - its triangle bounds wobble too, as in the LM324 circuit.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Speeds the module up. Linear response: +5V roughly doubles-to-triples the rate set by its capacitor. |
| `sync` | input | A pulse here resets both oscillators to their trough, locking them to another rhythm. Without a pulse they freewheel. |
| `invert` | input | The opposite of vert: slows the module down (and can stall it). Vert and invert together replace the knob the Nabra never had. |
| `SQUARE` | output | Upper oscillator square: 5V while the triangle rises, 0V while it falls. |
| `ANGLE` | output | Upper oscillator triangle, 0..5V. The classic slow shape to open an env input. |
| `SQUARE` | output | Lower (slow) oscillator square, 0/5V. |
| `ANGLE` | output | Lower (slow) oscillator triangle, 0..5V. |
| `CREST` | output | Pulse at the peak of the upper triangle. Trigger things from here. |
| `TROUGH` | output | Pulse at the bottom of the upper triangle: the other half of the rhythm. |
| `TROUGH` | output | Pulse at the bottom of the lower triangle. |
| `CREST` | output | Pulse at the peak of the lower triangle. |

### Port

Dual portamento / slew limiter: limits how fast a voltage may change. A square in becomes a trapezoid or triangle out. Feed it a MIDI GATE and it becomes an attack/release envelope; feed it a KEY and you get glide. Port 1: upper 25 V/s, lower 6 V/s. Port 2: upper 80 V/s, lower 2 V/s.

| Loop | Type | What it does |
|---|---|---|
| `in` | input | Upper input to be smoothed. |
| `in` | input | Lower input to be smoothed. |
| `vert` | input | Makes the upper slew faster (less smoothing). |
| `invert` | input | Makes the upper slew slower (more smoothing). |
| `vert` | input | Makes the lower slew faster. |
| `invert` | input | Makes the lower slew slower. |
| `OUT` | output | Smoothed upper signal. |
| `OUT` | output | Smoothed lower signal. |

### Shot

Dual 555 monostable. A trigger makes each half go high for a fixed time, then drop. Use SQUARE as a gate, END as a delayed trigger. Times (upper/lower): Shot 1 30/120 ms, Shot 2 80/330 ms, Shot 3 0.22/0.68 s, Shot 4 0.47/1.5 s.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Shortens both shots. |
| `trig` | input | A pulse fires both one-shots (ignored while a shot is already high). |
| `invert` | input | Lengthens both shots. |
| `START` | output | Pulse when the upper shot begins (redundant on purpose). |
| `END` | output | Pulse when the upper shot ends - a delayed trigger. |
| `START` | output | Pulse when the lower shot begins. |
| `END` | output | Pulse when the lower (longer) shot ends. |
| `SQUARE` | output | 5V gate for the duration of the upper shot. |
| `SQUARE` | output | 5V gate for the duration of the lower shot. |

### Dust

An analog version of SuperCollider's Dust: random-timed pulses with an average density. A fast internal oscillator is sampled into two capacitor tanks which decay at a fixed rate; when a tank empties it fires and resamples. Densities (upper/lower): Dust 1 ~1.2/s and ~4/s, Dust 2 ~0.4/s and ~10/s. Great for triggering Trand, Pulse, Swoop.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Raises the density of the upper dust. |
| `both` | input | Raises the density of both dusts. |
| `vert` | input | Raises the density of the lower dust. |
| `DUST` | output | Pulses at erratic, unevenly spaced times. |
| `DUST` | output | Pulses at erratic times, different density. |

### Stair

A variable step-height staircase. Every trigger steps the output up; past 5V it resets to zero and fires END. Stair 1 starts with ~0.55V steps, Stair 2 with ~1.1V steps. Patch STAIR into a vert for rising arpeggio-like patterns.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Bigger steps. |
| `trig` | input | Each pulse climbs one step. |
| `invert` | input | Smaller steps. |
| `END` | output | Pulse when the staircase reaches the top and resets. |
| `DOWN` | output | 5V while the stair is resting at the bottom. |
| `START` | output | Pulse when a new staircase begins. |
| `UP` | output | 5V while the staircase is climbing. |
| `STAIR` | output | The staircase voltage, 0..5V. |
| `SQUARE` | output | Flips on every step: the trigger divided by two. |

### Rand

Free-running random melody: two sample-and-holds clocked by their own square oscillators, sampling a fast internal triangle. Rand 1 clocks at ~2.2 and ~3.3 Hz, Rand 2 at ~0.7 and ~5.6 Hz. RAND into a Sine's vert = an infinite rambler.

| Loop | Type | What it does |
|---|---|---|
| `both` | input | Speeds up both internal clocks. Try feeding a RAND back in here. |
| `TRIG` | output | Pulse each time the upper clock samples. |
| `TRIG` | output | Pulse each time the lower clock samples. |
| `RAND` | output | Random stepped voltage 0..5V (upper). |
| `RAND` | output | Random stepped voltage 0..5V (lower). |

### Trand

Triggered random melody - 'the more essential random melody, inspired by Serge'. Feed it Dust, a Slope CREST, or MIDI TRIG.

| Loop | Type | What it does |
|---|---|---|
| `trig` | input | Each pulse takes a new random sample (upper). |
| `trig` | input | Each pulse takes a new random sample (lower). |
| `RAND` | output | Upper random stepped voltage 0..5V. |
| `RAND` | output | Lower random stepped voltage 0..5V. |
| `BOTH` | output | Sum of both RANDs (0..10V). |

### Swoop

The Nabra's envelope: when triggered it goes up, then down, then waits (like a Serge slope). Swoop 1 lasts ~0.1 s, Swoop 2 ~0.33 s, Swoop 3 ~1 s, Swoop 4 ~3.3 s. Patch SWOOP into any env input.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Faster swoop. |
| `trig` | input | Starts a swoop (ignored until the current one finishes). |
| `invert` | input | Slower swoop. |
| `SQUARE` | output | 5V for as long as the swoop is active. |
| `SWOOP` | output | The envelope: rises to 5V, falls back, waits. |
| `CREST` | output | Pulse at the peak - chain Swoops in a row. |

### Pulse

Dual percussive envelope (peak follower): quick attack, exponential decay. The cap inputs switch extra capacitors into the decay; charge left on a switched-out capacitor comes back as a little glitching murmur when it is switched in again. Base decay: Pulse 1 ~12 ms, Pulse 2 ~30 ms, Pulse 3 ~80 ms, Pulse 4 ~0.2 s. Pulses from Dust in, PULSE out to a Sine env.

| Loop | Type | What it does |
|---|---|---|
| `in` | input | Upper input: anything higher than the output charges it instantly. |
| `in` | input | Lower input. |
| `cap` | input | Above ~2.5V, gates a 2x capacitor into the upper decay. |
| `cap` | input | Gates a 5x capacitor into the upper decay. |
| `cap` | input | Gates a 12x capacitor into the upper decay. |
| `cap` | input | Gates a 2x capacitor into the lower decay. |
| `cap` | input | Gates a 5x capacitor into the lower decay. |
| `cap` | input | Gates a 12x capacitor into the lower decay. |
| `PULSE` | output | Percussive envelope: instant attack, RC decay. |
| `PULSE` | output | Lower percussive envelope. |

### Trang

Dual 566 triangle VCO pair tuned a fifth apart - power chords, now powerfully dissonant from 20 years of trimmer drift. Trang 1..4 sit near 65, 98, 147, 220 Hz. Both env inputs are required for sound. OBJET is mixed through the pot.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Raises both pitches (linear, not 1V/oct). |
| `fm` | input | Linear frequency modulation - patch audio here. |
| `invert` | input | Lowers both pitches. |
| `key` | input | (MIDI addition) Replaces the tuning capacitor: 1V = 100Hz. Patch a Kee/Duo KEY here to play it. |
| `env` | input | Opens the upper VCA. Silent without it. |
| `env` | input | Opens the lower (fifth) VCA. |
| `TRANG` | output | Raw upper triangle, always running. |
| `TRANG` | output | Raw lower triangle, a drifting fifth above. |
| `OBJET` | output | Upper triangle through its VCA. Also heard in the mix via the pot. |
| `OBJET` | output | The fifth through its VCA. Also in the mix. |

### Noise

Zener-diode noise squared off by a comparator. Its density is a timbre control: from ~10 crackles a second to near white. Noise 1..4 start around 25, 220, 1800 and 9000 flips/s.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Denser noise, towards white. |
| `env` | input | Opens the VCA. Required for sound. |
| `invert` | input | Sparser noise, towards crackle. |
| `HISS` | output | The gentler, linear zener hiss (always on). |
| `NOISE` | output | Fully amplified, comparator noise (always on). |
| `OBJET` | output | NOISE through the VCA. Mixed through the pot. |

### Sine

XR2206 function generator: a triangle shaped into a sine, plus a square, each with a VCA. Eight of them span bass to soprano: ~41, 60, 87, 124, 185, 272, 408, 600 Hz (Sine 1..8). Without an env they are silent.

| Loop | Type | What it does |
|---|---|---|
| `am` | input | Amplitude modulation by audio (AC coupled) - ring-mod-ish shimmer. |
| `vert` | input | Raises the pitch (linear). Patch a RAND here for a random melody. |
| `fm` | input | Linear FM. 'Fm modulation is essential.' Try a Trang OBJET. |
| `invert` | input | Lowers the pitch. |
| `key` | input | (MIDI addition) Replaces the tuning capacitor: 1V = 100Hz. Patch Kee KEY here. |
| `env` | input | Opens the sine VCA. Required for sound. |
| `env` | input | Opens the square VCA (only at its own OBJET, never the mix). |
| `SINE` | output | Raw shaped sine, always running. |
| `SQUARE` | output | Raw square, always running. |
| `OBJET` | output | Sine through its VCA - this is what reaches the mix. |
| `OBJET` | output | Square through its VCA (not mixed - patch it somewhere). |

### Wave

Resonant state-variable filter. No VCA - silent unless something comes in. Pulses make it ring into percussive blips. Wave 1..4 sit near 180, 520, 1500, 4200 Hz. The pot mixes the BP output.

| Loop | Type | What it does |
|---|---|---|
| `vert` | input | Raises the cutoff. |
| `fm` | input | Audio-rate cutoff modulation. |
| `invert` | input | Lowers the cutoff. |
| `key` | input | (MIDI addition) Sets the cutoff directly: 1V = 100Hz. With Kee KEY it rings at the played note. |
| `in` | input | Audio (or pulses, for pinged blips). |
| `HP` | output | High pass output. |
| `BP` | output | Band pass output - also mixed through the pot. |
| `LP` | output | Low pass output. |

### Pow

Power: positive, negative and ground potentials, to build bias voltages with resistor cords. Since nothing has a tuning knob, a POS through a 10k or 100k cord into vert is how you nudge a module's rate.

| Loop | Type | What it does |
|---|---|---|
| `POS` | output | +5V. A steady bias: patch into an env to drone, or through a resistor cord into vert to retune. |
| `NEG` | output | -5V. Into vert this slows / lowers a module (like invert). |
| `GND` | output | 0V. |

### Out

The buffered final mix, bottom right corner. Everything mixed by a pot arrives here; the two in jacks add anything else (e.g. a Wave LP or a Sine square OBJET) to the mix.

| Loop | Type | What it does |
|---|---|---|
| `in` | input | Auxiliary input straight into the main mix. |
| `POS` | output | +5V reference. |
| `NEG` | output | -5V reference. |
| `GND` | output | 0V. |
| `OUT` | output | The final mix itself - patch it back in for feedback. |

## The MIDI strip

These boards aren't in the 2000 tapestry. They're the bridge from your keyboard or DAW. They only have outputs.

### Kee

MIDI to CV, monophonic, last-note priority. KEY is made for the Nabra's linear world: patch it into the key input of a Sine, Trang or Wave and they play in tune. GATE into a Port gives an attack/release envelope; TRIG fires Swoops, Shots and Trands.

| Loop | Type | What it does |
|---|---|---|
| `KEY` | output | Pitch of the last-played note as linear volts: 1V = 100Hz (A440 = 4.4V). Includes pitch bend. Patch into a key input. |
| `GATE` | output | 5V while any key is held. |
| `TRIG` | output | Short pulse on every note-on. |
| `VELO` | output | Velocity of the last note, 0..5V. |
| `OFF` | output | Pulse when the last key is released. |

### Duo

MIDI to CV for two voices: the lowest and highest held notes. Play an interval and patch LOW and HIGH into two Sines. HGATE only opens when you hold a chord, so the second voice appears only then.

| Loop | Type | What it does |
|---|---|---|
| `LOW` | output | Lowest held note (1V = 100Hz). |
| `LGATE` | output | 5V while any note is held. |
| `HIGH` | output | Highest held note (1V = 100Hz). |
| `HGATE` | output | 5V while two or more notes are held. |
| `NOTES` | output | Number of held notes, 1V each. |

### Wheel

Continuous MIDI controllers as voltages. These are 'a multiplicity of outputs' for your hands: MOD into a Wave vert, PRESS into a Dust both, BEND into a Slope sync...

| Loop | Type | What it does |
|---|---|---|
| `MOD` | output | Mod wheel (CC1), 0..5V. |
| `BEND` | output | Pitch bend, -5..+5V. |
| `PRESS` | output | Aftertouch (channel or poly), 0..5V. |
| `BRITE` | output | CC74 (brightness / MPE slide), 0..5V. |
| `EXPR` | output | CC11 expression, 0..5V. |

### Tick

The host's clock as pulses. Only runs while the DAW is playing (free-runs at 120 BPM in the standalone app). BEAT into a Slope sync locks the slow shapes to your song.

| Loop | Type | What it does |
|---|---|---|
| `BEAT` | output | Pulse every quarter note of the host transport. |
| `SIXT` | output | Pulse every sixteenth note. |
| `BAR` | output | Pulse at every bar line. |
| `RUN` | output | 5V while the host is playing. |
| `RAMP` | output | Rises 0..5V across each bar. |

