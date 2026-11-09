#pragma once
// Nabra DSP engine: every board on the tapestry is a little analog-ish model running per sample.
// Signals are "volts": control 0..5V, pulses 5V/4ms, audio roughly +-5V. Rates respond LINEARLY
// to vert/invert (the Nabra has no exponential converters), around a base set by a "capacitor".
#include "Specs.h"
#include <atomic>
#include <array>
#include <memory>

namespace nabra
{
struct MidiState
{
    // note stack for last-note priority
    std::array<int, 128> stack {};
    int stackSize = 0;
    std::array<float, 128> vel {};
    float lastVel = 0.0f;
    int lastNote = -1, lowNote = -1, highNote = -1;
    int noteOnCount = 0, allOffCount = 0;
    float bend = 0.0f;   // -1..1
    float mod = 0, press = 0, brite = 0, expr = 0;  // 0..1
    float bendRange = 2.0f;

    // transport
    bool hasHostClock = false, playing = false;
    double ppq = 0.0, bpm = 120.0;
    double barLenQ = 4.0;

    void noteOn (int n, float v);
    void noteOff (int n);
    void allOff();
    void recalc();
    float keyVolts (int note) const;
};

struct Mod
{
    virtual ~Mod() = default;
    virtual void prepare (double sampleRate) { fs = (float) sampleRate; }
    virtual void process (const float* in, float* out) = 0;
    float fs = 96000.0f;
    const uint8_t* conn = nullptr;   // per-input "something is patched here"
    MidiState* midi = nullptr;
};

struct ConnRT
{
    int src = 0, dst = 0;   // global output / input slots
    Cord cord = Cord::Wire;
    float gain = 1.0f;
    float y = 0, xp = 0;    // capacitor cord state
};

class Engine
{
public:
    Engine();
    void prepare (double internalRate);
    void setPatch (const std::vector<Cable>& cables);   // audio thread only (call via processor handoff)
    // render n internal samples into L/R (added)
    void render (float* L, float* R, int n);

    MidiState midi;
    std::vector<std::atomic<float>*> knobs;   // per knob param index (raw APVTS pointers)
    std::atomic<float>* master = nullptr;

    // ---- telemetry for the GUI ----
    static constexpr int numCells = 56;
    std::array<std::atomic<float>, numCells> led {};
    // probe: kind 0 = none, 1 = output slot, 2 = input slot
    std::atomic<int> probeKind { 0 }, probeSlot { 0 };
    static constexpr int probeLen = 512;
    std::array<std::atomic<float>, probeLen> probeMin {}, probeMax {};
    std::atomic<int> probeWrite { 0 };
    std::atomic<float> outLevel { 0 };

    int outSlot (int cell, int jack) const;
    int inSlot (int cell, int jack) const;

private:
    struct Inst { std::unique_ptr<Mod> mod; int firstIn = 0, firstOut = 0, ledSlot = -1; };
    std::vector<Inst> inst;
    struct MixTap { int slot; int knob; float pl, pr; };
    std::vector<MixTap> taps;
    int outInA = -1, outInB = -1, outOutSlot = -1;

    std::vector<float> ins, outs;
    std::vector<uint8_t> connected;
    std::vector<ConnRT> conns;
    float rate = 96000.0f;
    float dcL = 0, dcR = 0, dcxL = 0, dcxR = 0, dcA = 0.999f;
    int ledCounter = 0;
    std::array<float, numCells> ledAcc {};
    int probeCounter = 0, probeDecim = 64;
    float pMin = 1e9f, pMax = -1e9f;
    int lastProbeKind = 0, lastProbeSlot = -1;
    float lastMix = 0;
};
} // namespace nabra
