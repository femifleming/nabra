#include "Engine.h"
#include <cmath>

namespace nabra
{
namespace
{
constexpr float kTwoPi = 6.28318530718f;
constexpr float kPulseSec = 0.0015f;

struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed = 1) : s (seed * 2654435761u + 12345u) {}
    float uni() { s = s * 1664525u + 1013904223u; return (float) (s >> 8) * (1.0f / 16777216.0f); }
    float bip() { return uni() * 2.0f - 1.0f; }
    float gauss() { float a = 0; for (int i = 0; i < 4; ++i) a += uni(); return (a - 2.0f) * 1.732f; }
};

struct Schmitt
{
    bool st = false;
    bool rise (float v)
    {
        if (! st && v > 2.0f) { st = true; return true; }
        if (st && v < 1.0f) st = false;
        return false;
    }
};

struct PulseGen
{
    int n = 0;
    void fire (float fs) { n = (int) (kPulseSec * fs); }
    float tick() { if (n > 0) { --n; return 5.0f; } return 0.0f; }
};

inline float rateMul (float vert, float invert, float g, float lo = 0.03f, float hi = 12.0f)
{
    return juce::jlimit (lo, hi, 1.0f + g * (vert - invert));
}


// ======================= SLOPE =======================
struct SlopeM : Mod
{
    struct Osc { float x = -1, dir = 1, f = 1; PulseGen crest, trough; };
    Osc a, b;
    Schmitt sync;
    float lp = 0, lpc = 0;
    SlopeM (int k)
    {
        static const float r[12] = { 0.017f, 0.029f, 0.047f, 0.075f, 0.12f, 0.19f, 0.31f, 0.5f, 0.8f, 1.3f, 2.1f, 3.4f };
        a.f = r[k + 6]; b.f = r[k];
        Rng g ((uint32_t) (k + 11)); a.x = g.bip(); b.x = g.bip();
    }
    void prepare (double sr) override { Mod::prepare (sr); lpc = 1.0f - std::exp (-kTwoPi * 4.0f / fs); }
    void step (Osc& o, float m, float bnd, bool s)
    {
        if (s) { o.x = -bnd; o.dir = 1; return; }
        o.x += o.dir * 4.0f * o.f * m / fs;
        if (o.dir > 0 && o.x >= bnd) { o.x = bnd; o.dir = -1; o.crest.fire (fs); }
        else if (o.dir < 0 && o.x <= -bnd) { o.x = -bnd; o.dir = 1; o.trough.fire (fs); }
    }
    void process (const float* in, float* out) override
    {
        float ctl = in[0] - in[2];
        lp += (ctl - lp) * lpc;
        float hp = ctl - lp;                                   // fast part of the modulation...
        float bnd = juce::jlimit (0.45f, 1.4f, 1.0f - 0.07f * hp); // ...moves the comparator bounds (sync-lock character)
        float m = rateMul (in[0], in[2], 0.35f);
        bool s = sync.rise (in[1]);
        step (a, m, bnd, s);
        step (b, m, bnd, s);
        out[0] = a.dir > 0 ? 5.0f : 0.0f;
        out[1] = juce::jlimit (-0.5f, 6.0f, (a.x + 1.0f) * 2.5f);
        out[2] = b.dir > 0 ? 5.0f : 0.0f;
        out[3] = juce::jlimit (-0.5f, 6.0f, (b.x + 1.0f) * 2.5f);
        out[4] = a.crest.tick(); out[5] = a.trough.tick();
        out[6] = b.trough.tick(); out[7] = b.crest.tick();
    }
};

// ======================= PORT =======================
struct PortM : Mod
{
    float rA, rB, yA = 0, yB = 0;
    PortM (int k) { rA = k == 0 ? 25.0f : 80.0f; rB = k == 0 ? 6.0f : 2.0f; }
    static void slew (float& y, float x, float maxStep) { y += juce::jlimit (-maxStep, maxStep, x - y); }
    void process (const float* in, float* out) override
    {
        slew (yA, in[0], rA * rateMul (in[2], in[3], 0.35f, 0.02f, 30.0f) / fs);
        slew (yB, in[1], rB * rateMul (in[4], in[5], 0.35f, 0.02f, 30.0f) / fs);
        out[0] = yA; out[1] = yB;
    }
};

// ======================= SHOT =======================
struct ShotM : Mod
{
    struct One { float dur, t = 0; bool on = false; PulseGen st, en; };
    One a, b;
    Schmitt trig;
    ShotM (int k)
    {
        static const float d[4][2] = { { 0.03f, 0.12f }, { 0.08f, 0.33f }, { 0.22f, 0.68f }, { 0.47f, 1.5f } };
        a.dur = d[k][0]; b.dur = d[k][1];
    }
    void step (One& o, bool t, float m)
    {
        if (t && ! o.on) { o.on = true; o.t = 0; o.st.fire (fs); }
        if (o.on) { o.t += m / fs; if (o.t >= o.dur) { o.on = false; o.en.fire (fs); } }
    }
    void process (const float* in, float* out) override
    {
        bool t = trig.rise (in[1]);
        float m = rateMul (in[0], in[2], 0.35f);
        step (a, t, m); step (b, t, m);
        out[0] = a.st.tick(); out[1] = a.en.tick();
        out[2] = b.st.tick(); out[3] = b.en.tick();
        out[4] = a.on ? 5.0f : 0.0f; out[5] = b.on ? 5.0f : 0.0f;
    }
};

// ======================= DUST =======================
struct DustM : Mod
{
    struct Tank { float v = 0.5f, k = 1; PulseGen p; };
    Tank a, b;
    float ph = 0, phInc = 0;
    Rng rng;
    DustM (int k) : rng ((uint32_t) (k + 101))
    {
        // decay constant ~ density * ln(1/thr)/2  (thr = 0.02)
        static const float dens[2][2] = { { 1.2f, 4.0f }, { 0.4f, 10.0f } };
        a.k = dens[k][0] * 1.96f; b.k = dens[k][1] * 1.96f;
    }
    void prepare (double sr) override { Mod::prepare (sr); phInc = 1731.3f / fs; }
    float sampleOsc()
    {
        float t = 2.0f * std::abs (2.0f * (ph - std::floor (ph)) - 1.0f) - 1.0f; // fast triangle
        return juce::jlimit (0.021f, 1.0f, 0.5f + 0.5f * t + 0.03f * rng.bip());
    }
    void step (Tank& t, float m)
    {
        t.v *= 1.0f - t.k * m / fs;
        if (t.v < 0.02f) { t.p.fire (fs); t.v = sampleOsc(); }
    }
    void process (const float* in, float* out) override
    {
        ph += phInc;
        if (ph > 1.0f) ph -= 1.0f;
        step (a, juce::jlimit (0.02f, 30.0f, 1.0f + 0.4f * (in[0] + in[1])));
        step (b, juce::jlimit (0.02f, 30.0f, 1.0f + 0.4f * (in[2] + in[1])));
        out[0] = a.p.tick(); out[1] = b.p.tick();
    }
};

// ======================= STAIR =======================
struct StairM : Mod
{
    float stepBase, target = 0, y = 0;
    bool climbing = false, sq = false;
    Schmitt trig;
    PulseGen end, start;
    StairM (int k) { stepBase = k == 0 ? 0.55f : 1.1f; }
    void process (const float* in, float* out) override
    {
        if (trig.rise (in[1]))
        {
            sq = ! sq;
            float st = stepBase * juce::jlimit (0.05f, 8.0f, 1.0f + 0.35f * (in[0] - in[2]));
            if (target + st > 5.0f) { target = 0; y = 0; climbing = false; end.fire (fs); }
            else { if (! climbing) start.fire (fs); climbing = true; target += st; }
        }
        y += (target - y) * (1.0f - std::exp (-1.0f / (0.004f * fs)));
        out[0] = end.tick();
        out[1] = climbing ? 0.0f : 5.0f;
        out[2] = start.tick();
        out[3] = climbing ? 5.0f : 0.0f;
        out[4] = y;
        out[5] = sq ? 5.0f : 0.0f;
    }
};

// ======================= RAND / TRAND =======================
struct SH
{
    float ph = 0, inc = 0, held = 0;
    Rng rng;
    float sample()
    {
        float t = 2.0f * std::abs (2.0f * (ph - std::floor (ph)) - 1.0f) - 1.0f;
        held = juce::jlimit (0.0f, 5.0f, 2.5f + 2.5f * t + 0.15f * rng.bip());
        return held;
    }
    void tick() { ph += inc; if (ph > 1.0f) ph -= 1.0f; }
};

struct RandM : Mod
{
    float fA, fB, cA = 0, cB = 0;
    SH sa, sb;
    PulseGen pa, pb;
    RandM (int k)
    {
        fA = k == 0 ? 2.2f : 0.7f; fB = k == 0 ? 3.3f : 5.6f;
        sa.rng = Rng ((uint32_t) (k * 7 + 3)); sb.rng = Rng ((uint32_t) (k * 7 + 4));
        sa.held = sa.sample(); sb.held = 2.0f;
    }
    void prepare (double sr) override { Mod::prepare (sr); sa.inc = 2971.7f / fs; sb.inc = 3343.1f / fs; }
    void process (const float* in, float* out) override
    {
        sa.tick(); sb.tick();
        float m = juce::jlimit (0.03f, 12.0f, 1.0f + 0.35f * in[0]);
        cA += fA * m / fs; cB += fB * m / fs;
        if (cA >= 1.0f) { cA -= std::floor (cA); sa.sample(); pa.fire (fs); }
        if (cB >= 1.0f) { cB -= std::floor (cB); sb.sample(); pb.fire (fs); }
        out[0] = pa.tick(); out[1] = pb.tick();
        out[2] = sa.held; out[3] = sb.held;
    }
};

struct TrandM : Mod
{
    SH sa, sb;
    Schmitt ta, tb;
    TrandM (int k) { sa.rng = Rng ((uint32_t) (k * 13 + 5)); sb.rng = Rng ((uint32_t) (k * 13 + 6)); sa.held = 1.5f; sb.held = 3.0f; }
    void prepare (double sr) override { Mod::prepare (sr); sa.inc = 3119.3f / fs; sb.inc = 2789.9f / fs; }
    void process (const float* in, float* out) override
    {
        sa.tick(); sb.tick();
        if (ta.rise (in[0])) sa.sample();
        if (tb.rise (in[1])) sb.sample();
        out[0] = sa.held; out[1] = sb.held; out[2] = sa.held + sb.held;
    }
};

// ======================= SWOOP =======================
struct SwoopM : Mod
{
    float T, y = 0; int phase = 0; // 0 idle, 1 up, 2 down
    Schmitt trig;
    PulseGen crest;
    SwoopM (int k) { static const float t[4] = { 0.1f, 0.33f, 1.0f, 3.3f }; T = t[k]; }
    void process (const float* in, float* out) override
    {
        if (trig.rise (in[1]) && phase == 0) phase = 1;
        float d = 10.0f / T * rateMul (in[0], in[2], 0.35f) / fs;  // 5V up + 5V down over T
        if (phase == 1) { y += d; if (y >= 5.0f) { y = 5.0f; phase = 2; crest.fire (fs); } }
        else if (phase == 2) { y -= d; if (y <= 0.0f) { y = 0.0f; phase = 0; } }
        out[0] = phase != 0 ? 5.0f : 0.0f;
        out[1] = y;
        out[2] = crest.tick();
    }
};

// ======================= PULSE =======================
struct PulseM : Mod
{
    struct Ch { float v = 0; float cv[3] = {}; bool on[3] = {}; };
    Ch a, b;
    float tau0, att = 0;
    static constexpr float cmul[3] = { 2.0f, 5.0f, 12.0f };
    PulseM (int k) { static const float t[4] = { 0.012f, 0.03f, 0.08f, 0.2f }; tau0 = t[k]; }
    void prepare (double sr) override { Mod::prepare (sr); att = 1.0f - std::exp (-1.0f / (0.0007f * fs)); }
    void step (Ch& c, float x, const float* caps)
    {
        float C = 1.0f;
        for (int i = 0; i < 3; ++i)
        {
            bool want = caps[i] > 2.5f;
            if (want && ! c.on[i])   // charge sharing: the held charge glitches back in
                c.v = (c.v * C + c.cv[i] * cmul[i]) / (C + cmul[i]);
            c.on[i] = want;
            if (c.on[i]) C += cmul[i];
        }
        if (x > c.v) c.v += (x - c.v) * att;
        else c.v *= 1.0f - 1.0f / (tau0 * C * fs);
        for (int i = 0; i < 3; ++i) if (c.on[i]) c.cv[i] = c.v;
    }
    void process (const float* in, float* out) override
    {
        step (a, in[0], in + 2);
        step (b, in[1], in + 5);
        out[0] = a.v; out[1] = b.v;
    }
};

// shared VCA helper: env (0..5V) -> gain, lightly smoothed against zipper
struct Vca
{
    float g = 0, c = 0.05f;
    void prepare (float fs) { c = 1.0f - std::exp (-1.0f / (0.0004f * fs)); }
    float tick (float env) { g += (juce::jlimit (0.0f, 1.4f, env * 0.2f) - g) * c; return g; }
};

inline float keyedBase (float base, const uint8_t* conn, int keySlot, float keyV)
{
    if (conn && conn[keySlot]) return juce::jmax (0.0f, keyV * 100.0f);
    return base;
}

// ======================= TRANG =======================
struct TrangM : Mod
{
    float base, ratio, pa = 0, pb = 0;
    Vca va, vb;
    TrangM (int k)
    {
        static const float f[4] = { 65.4f, 98.0f, 147.0f, 220.0f };
        static const float drift[4] = { 0.031f, -0.022f, 0.047f, -0.013f };
        base = f[k]; ratio = 1.5f * (1.0f + drift[k]);
        pb = 0.37f;
    }
    void prepare (double sr) override { Mod::prepare (sr); va.prepare (fs); vb.prepare (fs); }
    static float tri (float p) { return 2.0f * std::abs (2.0f * p - 1.0f) - 1.0f; }
    void process (const float* in, float* out) override
    {
        float f = keyedBase (base, conn, 3, in[3]) * rateMul (in[0], in[2], 0.25f, 0.0f, 12.0f);
        f *= juce::jmax (0.0f, 1.0f + 0.12f * in[1]);
        float nyq = 0.45f * fs;
        pa += juce::jmin (f, nyq) / fs;           if (pa >= 1.0f) pa -= std::floor (pa);
        pb += juce::jmin (f * ratio, nyq) / fs;   if (pb >= 1.0f) pb -= std::floor (pb);
        float ta = 5.0f * tri (pa), tb = 5.0f * tri (pb);
        out[0] = ta; out[1] = tb;
        out[2] = ta * va.tick (in[4]);
        out[3] = tb * vb.tick (in[5]);
    }
};

// ======================= NOISE =======================
struct NoiseM : Mod
{
    float dens, state = 5.0f, hissLp = 0;
    Rng rng;
    Vca vca;
    NoiseM (int k) : rng ((uint32_t) (k + 501)) { static const float d[4] = { 25.0f, 220.0f, 1800.0f, 9000.0f }; dens = d[k]; }
    void prepare (double sr) override { Mod::prepare (sr); vca.prepare (fs); }
    void process (const float* in, float* out) override
    {
        float d = dens * rateMul (in[0], in[2], 0.35f, 0.02f, 25.0f);
        if (rng.uni() < d / fs) state = -state;
        float w = rng.gauss();
        hissLp += (w - hissLp) * 0.6f;   // zener hiss is slightly soft
        out[0] = 1.4f * hissLp;
        out[1] = state;
        out[2] = state * vca.tick (in[1]);
    }
};

// ======================= SINE (XR2206) =======================
struct SineM : Mod
{
    float base, ph = 0, amHp = 0, amX = 0, amA = 0.999f;
    Vca va, vb;
    SineM (int k)
    {
        static const float f[8] = { 40.8f, 60.0f, 86.8f, 123.6f, 185.4f, 272.0f, 408.0f, 600.0f };
        base = f[k];
        ph = 0.13f * (float) k;
    }
    void prepare (double sr) override
    {
        Mod::prepare (sr); va.prepare (fs); vb.prepare (fs);
        amA = std::exp (-kTwoPi * 15.0f / fs);
    }
    void process (const float* in, float* out) override
    {
        float f = keyedBase (base, conn, 4, in[4]) * rateMul (in[1], in[3], 0.25f, 0.0f, 12.0f);
        f *= juce::jmax (0.0f, 1.0f + 0.12f * in[2]);
        ph += juce::jmin (f, 0.45f * fs) / fs;
        if (ph >= 1.0f) ph -= std::floor (ph);
        float t = 2.0f * std::abs (2.0f * ph - 1.0f) - 1.0f;
        float s = std::sin (1.5f * t) / 0.99749f;   // 2206 shaper: a near-sine with a touch of flattening
        amHp = amA * (amHp + in[0] - amX); amX = in[0];
        out[0] = 5.0f * s;
        out[1] = ph < 0.5f ? 5.0f : -5.0f;
        out[2] = out[0] * va.tick (in[5]) * juce::jmax (0.0f, 1.0f + 0.2f * amHp);
        out[3] = out[1] * vb.tick (in[6]);
    }
};

// ======================= WAVE (SVF) =======================
struct WaveM : Mod
{
    float base, ic1 = 0, ic2 = 0;
    const float k = 0.12f; // 1/Q: resonant enough to ring when pinged
    WaveM (int idx) { static const float f[4] = { 180.0f, 520.0f, 1500.0f, 4200.0f }; base = f[idx]; }
    void process (const float* in, float* out) override
    {
        float f = keyedBase (base, conn, 3, in[3]) * rateMul (in[0], in[2], 0.35f, 0.02f, 20.0f);
        f *= juce::jmax (0.02f, 1.0f + 0.12f * in[1]);
        f = juce::jlimit (8.0f, 0.42f * fs, f);
        float g = std::tan (3.14159265f * f / fs);
        float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        float v0 = in[4] * 0.5f;
        float v3 = v0 - ic2;
        float v1 = a1 * ic1 + a2 * v3;
        float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = juce::jlimit (-40.0f, 40.0f, 2.0f * v1 - ic1);   // safety only - no per-sample damping
        ic2 = juce::jlimit (-40.0f, 40.0f, 2.0f * v2 - ic2);
        out[0] = v0 - k * v1 - v2;  // HP
        out[1] = v1;                // BP
        out[2] = v2;                // LP
    }
};

// ======================= POW / OUT =======================
struct PowM : Mod
{
    void process (const float*, float* out) override { out[0] = 5.0f; out[1] = -5.0f; out[2] = 0; out[3] = 0; }
};
struct OutM : Mod
{
    float* mixRef = nullptr;
    void process (const float*, float* out) override { out[0] = 5.0f; out[1] = -5.0f; out[2] = 0; out[3] = mixRef ? *mixRef : 0.0f; }
};

// ======================= MIDI strip =======================
struct KeeM : Mod
{
    int seenOn = 0, seenOff = 0;
    float key = 2.2f;
    PulseGen trig, off;
    void process (const float*, float* out) override
    {
        auto& m = *midi;
        if (m.noteOnCount != seenOn) { seenOn = m.noteOnCount; trig.fire (fs); }
        if (m.allOffCount != seenOff) { seenOff = m.allOffCount; off.fire (fs); }
        if (m.lastNote >= 0) key = m.keyVolts (m.lastNote);
        out[0] = key;
        out[1] = m.stackSize > 0 ? 5.0f : 0.0f;
        out[2] = trig.tick();
        out[3] = m.lastVel * 5.0f;
        out[4] = off.tick();
    }
};
struct DuoM : Mod
{
    float lo = 2.2f, hi = 2.2f;
    void process (const float*, float* out) override
    {
        auto& m = *midi;
        if (m.lowNote >= 0) lo = m.keyVolts (m.lowNote);
        if (m.highNote >= 0) hi = m.keyVolts (m.highNote);
        out[0] = lo; out[1] = m.stackSize > 0 ? 5.0f : 0.0f;
        out[2] = hi; out[3] = m.stackSize > 1 ? 5.0f : 0.0f;
        out[4] = (float) m.stackSize;
    }
};
struct WheelM : Mod
{
    float s[5] = {}, c = 0.01f;
    void prepare (double sr) override { Mod::prepare (sr); c = 1.0f - std::exp (-1.0f / (0.003f * fs)); }
    void process (const float*, float* out) override
    {
        auto& m = *midi;
        const float t[5] = { m.mod * 5, m.bend * 5, m.press * 5, m.brite * 5, m.expr * 5 };
        for (int i = 0; i < 5; ++i) { s[i] += (t[i] - s[i]) * c; out[i] = s[i]; }
    }
};
struct TickM : Mod
{
    double lastQ = -1;
    PulseGen beat, sixt, bar;
    float ramp = 0;
    void process (const float*, float* out) override
    {
        auto& m = *midi;
        out[3] = m.playing ? 5.0f : 0.0f;
        if (m.playing)
        {
            double q = m.ppq;
            if (lastQ >= 0 && q > lastQ)
            {
                if (std::floor (q) != std::floor (lastQ)) beat.fire (fs);
                if (std::floor (q * 4) != std::floor (lastQ * 4)) sixt.fire (fs);
                if (std::floor (q / m.barLenQ) != std::floor (lastQ / m.barLenQ)) bar.fire (fs);
            }
            else if (lastQ >= 0 && q >= lastQ - 0.05)
            {
                q = lastQ;   // host re-sync nudged us slightly backwards: hold, don't double-fire
            }
            else
            {
                // transport (re)started: fire on a grid line
                if (q - std::floor (q) < 1e-3) beat.fire (fs);
                if (q * 4 - std::floor (q * 4) < 4e-3) sixt.fire (fs);
                if (std::fmod (q, m.barLenQ) < 1e-3) bar.fire (fs);
            }
            lastQ = q;
            double b = q / m.barLenQ;
            ramp = (float) (b - std::floor (b)) * 5.0f;
        }
        else lastQ = -1;
        out[0] = beat.tick(); out[1] = sixt.tick(); out[2] = bar.tick(); out[4] = ramp;
    }
};
} // namespace

// ---------------- MidiState ----------------
float MidiState::keyVolts (int note) const
{
    float semis = (float) note - 69.0f + bend * bendRange;
    return 4.4f * std::pow (2.0f, semis / 12.0f);
}
void MidiState::recalc()
{
    lastNote = stackSize > 0 ? stack[(size_t) stackSize - 1] : lastNote;
    if (stackSize > 0)
    {
        int lo = 999, hi = -1;
        for (int i = 0; i < stackSize; ++i) { lo = std::min (lo, stack[(size_t) i]); hi = std::max (hi, stack[(size_t) i]); }
        lowNote = lo; highNote = hi;
    }
}
void MidiState::noteOn (int n, float v)
{
    for (int i = 0; i < stackSize; ++i)
        if (stack[(size_t) i] == n) { for (int j = i; j < stackSize - 1; ++j) stack[(size_t) j] = stack[(size_t) j + 1]; --stackSize; break; }
    if (stackSize < 128) stack[(size_t) stackSize++] = n;
    vel[(size_t) n] = v; lastVel = v;
    ++noteOnCount;
    recalc();
}
void MidiState::noteOff (int n)
{
    bool had = false;
    for (int i = 0; i < stackSize; ++i)
        if (stack[(size_t) i] == n) { for (int j = i; j < stackSize - 1; ++j) stack[(size_t) j] = stack[(size_t) j + 1]; --stackSize; had = true; break; }
    if (had && stackSize == 0) ++allOffCount;
    recalc();
}
void MidiState::allOff()
{
    if (stackSize > 0) ++allOffCount;
    stackSize = 0;
}

// ---------------- Engine ----------------
Engine::Engine()
{
    int nIn = 0, nOut = 0;
    const auto& cs = cells();
    jassert ((int) cs.size() == numCells);
    std::vector<int> outRefs;
    for (size_t i = 0; i < cs.size(); ++i)
    {
        const auto& c = cs[i];
        const auto& sp = spec (c.type);
        int k = c.num - 1;
        std::unique_ptr<Mod> m;
        switch (c.type)
        {
            case MT::Slope: m = std::make_unique<SlopeM> (k); break;
            case MT::Port:  m = std::make_unique<PortM> (k); break;
            case MT::Shot:  m = std::make_unique<ShotM> (k); break;
            case MT::Dust:  m = std::make_unique<DustM> (k); break;
            case MT::Stair: m = std::make_unique<StairM> (k); break;
            case MT::Rand:  m = std::make_unique<RandM> (k); break;
            case MT::Trand: m = std::make_unique<TrandM> (k); break;
            case MT::Swoop: m = std::make_unique<SwoopM> (k); break;
            case MT::Pulse: m = std::make_unique<PulseM> (k); break;
            case MT::Trang: m = std::make_unique<TrangM> (k); break;
            case MT::Noise: m = std::make_unique<NoiseM> (k); break;
            case MT::Sine:  m = std::make_unique<SineM> (k); break;
            case MT::Wave:  m = std::make_unique<WaveM> (k); break;
            case MT::Pow:   m = std::make_unique<PowM>(); break;
            case MT::Out:   m = std::make_unique<OutM>(); break;
            case MT::Kee:   m = std::make_unique<KeeM>(); break;
            case MT::Duo:   m = std::make_unique<DuoM>(); break;
            case MT::Wheel: m = std::make_unique<WheelM>(); break;
            case MT::Tick:  m = std::make_unique<TickM>(); break;
            case MT::Count: break;
        }
        m->midi = &midi;
        Inst in;
        in.mod = std::move (m);
        in.firstIn = nIn; in.firstOut = nOut;
        int lj = sp.jackIndex (sp.ledJack);
        in.ledSlot = lj >= 0 ? nOut + sp.outSlot[(size_t) lj] : -1;

        if (sp.hasKnob)
        {
            int knob = knobParamIndex ((int) i);
            for (size_t mj = 0; mj < sp.mixJacks.size(); ++mj)
            {
                int j = sp.jackIndex (sp.mixJacks[mj]);
                // stereo placement: a tapestry six feet wide
                float pan = 0;
                if (c.type == MT::Trang) pan = mj == 0 ? -0.45f : 0.45f;
                else if (c.type == MT::Sine) pan = (c.col == 10 ? -0.35f : 0.35f) * (c.row % 2 ? 0.6f : 1.0f);
                else if (c.type == MT::Noise) pan = (c.row % 2 ? 0.3f : -0.3f);
                else if (c.type == MT::Wave) pan = (c.row % 2 ? -0.2f : 0.2f);
                float a = (pan + 1.0f) * 0.25f * 3.14159265f;
                taps.push_back ({ nOut + sp.outSlot[(size_t) j], knob, std::cos (a) * 1.41421356f, std::sin (a) * 1.41421356f });
            }
        }
        if (c.type == MT::Out)
        {
            outInA = nIn; outInB = nIn + 1;
            static_cast<OutM*> (in.mod.get())->mixRef = &lastMix;
        }
        nIn += sp.numIn; nOut += sp.numOut;
        inst.push_back (std::move (in));
    }
    ins.assign ((size_t) nIn, 0.0f);
    outs.assign ((size_t) nOut, 0.0f);
    connected.assign ((size_t) nIn, 0);
    for (auto& i : inst) i.mod->conn = connected.data() + i.firstIn;
    conns.reserve (1024);
    knobs.assign ((size_t) knobParamCount(), nullptr);
}

int Engine::outSlot (int cell, int jack) const
{
    const auto& sp = spec (cells()[(size_t) cell].type);
    int s = sp.outSlot[(size_t) jack];
    return s < 0 ? -1 : inst[(size_t) cell].firstOut + s;
}
int Engine::inSlot (int cell, int jack) const
{
    const auto& sp = spec (cells()[(size_t) cell].type);
    int s = sp.inSlot[(size_t) jack];
    return s < 0 ? -1 : inst[(size_t) cell].firstIn + s;
}

void Engine::prepare (double r)
{
    rate = (float) r;
    for (auto& i : inst) i.mod->prepare (r);
    dcA = std::exp (-kTwoPi * 8.0f / rate);
    probeDecim = juce::jmax (1, (int) (rate / 190.0f));   // ~2.7 s of history in 512 bins
}

void Engine::setPatch (const std::vector<Cable>& cables)
{
    conns.clear();
    std::fill (connected.begin(), connected.end(), 0);
    const float capA = std::exp (-1.0f / (0.01f * rate));  // 10k * 1uF
    for (auto& c : cables)
    {
        int s = outSlot (c.src.cell, c.src.jack), d = inSlot (c.dst.cell, c.dst.jack);
        if (s < 0 || d < 0 || conns.size() >= conns.capacity()) continue;
        ConnRT r;
        r.src = s; r.dst = d; r.cord = c.cord;
        r.gain = c.cord == Cord::Resistor ? resistorGain (c.resVal) : (c.cord == Cord::Capacitor ? capA : 1.0f);
        conns.push_back (r);
        connected[(size_t) d] = 1;
    }
}

void Engine::render (float* L, float* R, int n)
{
    const int pk = probeKind.load (std::memory_order_relaxed), ps = probeSlot.load (std::memory_order_relaxed);
    if (pk != lastProbeKind || ps != lastProbeSlot)
    {
        lastProbeKind = pk; lastProbeSlot = ps; probeCounter = 0; pMin = 1e9f; pMax = -1e9f;
        for (int i = 0; i < probeLen; ++i) { probeMin[(size_t) i].store (0); probeMax[(size_t) i].store (0); }
    }
    float knobVals[64];
    for (size_t k = 0; k < knobs.size() && k < 64; ++k) knobVals[k] = knobs[k] ? knobs[k]->load() : 0.7f;
    const float mst = master ? master->load() : 0.8f;
    float peak = 0;

    for (int i = 0; i < n; ++i)
    {
        std::fill (ins.begin(), ins.end(), 0.0f);
        for (auto& c : conns)
        {
            float v = outs[(size_t) c.src];
            switch (c.cord)
            {
                case Cord::Wire: break;
                case Cord::Resistor: v *= c.gain; break;
                case Cord::Capacitor: c.y = c.gain * (c.y + v - c.xp); c.xp = v; v = c.y; break;
                case Cord::Diode: v = juce::jmax (0.0f, v - 0.6f); break;
            }
            ins[(size_t) c.dst] += v;
        }
        for (auto& m : inst) m.mod->process (ins.data() + m.firstIn, outs.data() + m.firstOut);

        float l = 0, r = 0;
        for (auto& t : taps)
        {
            float v = outs[(size_t) t.slot] * knobVals[t.knob];
            l += v * t.pl; r += v * t.pr;
        }
        float aux = ins[(size_t) outInA] + ins[(size_t) outInB];
        l += aux; r += aux;
        lastMix = 0.5f * (l + r);

        // DC block, gain, soft clip
        float yl = dcA * (dcL + l - dcxL); dcxL = l; dcL = yl;
        float yr = dcA * (dcR + r - dcxR); dcxR = r; dcR = yr;
        float g = 0.07f * mst;
        float ol = std::tanh (yl * g), orr = std::tanh (yr * g);
        L[i] += ol; R[i] += orr;
        peak = juce::jmax (peak, std::abs (ol), std::abs (orr));

        // MIDI clock advances with the samples
        if (midi.playing) midi.ppq += midi.bpm / 60.0 / (double) rate;

        if (pk != 0)
        {
            float v = pk == 1 ? outs[(size_t) ps] : ins[(size_t) ps];
            pMin = juce::jmin (pMin, v); pMax = juce::jmax (pMax, v);
            if (++probeCounter >= probeDecim)
            {
                int w = probeWrite.load (std::memory_order_relaxed);
                probeMin[(size_t) w].store (pMin, std::memory_order_relaxed);
                probeMax[(size_t) w].store (pMax, std::memory_order_relaxed);
                probeWrite.store ((w + 1) % probeLen, std::memory_order_release);
                probeCounter = 0; pMin = 1e9f; pMax = -1e9f;
            }
        }
        if ((++ledCounter & 15) == 0)
            for (size_t m = 0; m < inst.size(); ++m)
                if (inst[m].ledSlot >= 0)
                    ledAcc[m] = juce::jmax (ledAcc[m] * 0.995f, std::abs (outs[(size_t) inst[m].ledSlot]) * 0.2f);
    }
    for (size_t m = 0; m < inst.size(); ++m) led[m].store (juce::jlimit (0.0f, 1.0f, ledAcc[m]), std::memory_order_relaxed);
    outLevel.store (peak, std::memory_order_relaxed);
}
} // namespace nabra
