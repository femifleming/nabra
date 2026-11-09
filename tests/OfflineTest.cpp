// Renders each recipe offline (with MIDI played into the strip), checks the audio is sane, writes WAVs.
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/Engine.h"
#include <chrono>

using namespace nabra;

int main (int argc, char** argv)
{
    juce::File outDir (argc > 1 ? argv[1] : "/tmp");
    const double hostRate = 48000.0, rate = hostRate * 2;
    const float seconds = 8.0f;
    int failures = 0;
    for (int r = 0; r < (int) recipes().size(); ++r)
    {
        Engine e;
        std::atomic<float> master { 0.8f };
        std::vector<std::atomic<float>> knobs ((size_t) knobParamCount());
        for (size_t k = 0; k < knobs.size(); ++k) { knobs[k] = 0.7f; e.knobs[k] = &knobs[k]; }
        e.master = &master;
        e.prepare (rate);
        e.setPatch (buildRecipe (recipes()[(size_t) r]));
        e.midi.playing = true; e.midi.bpm = 120;

        const int total = (int) (seconds * rate), block = 128;
        std::vector<float> L ((size_t) total, 0.0f), R ((size_t) total, 0.0f);
        // a little melody: notes every 0.5 s, chords on odd beats
        const int notes[] = { 57, 60, 64, 62, 67, 65, 60, 55 };
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int pos = 0; pos < total; pos += block)
        {
            int beat = pos / (int) (rate * 0.5);
            int inBeat = pos % (int) (rate * 0.5);
            if (inBeat < block)
            {
                e.midi.allOff();
                int n = notes[beat % 8];
                e.midi.noteOn (n, 0.8f);
                if (beat % 2) e.midi.noteOn (n + 7, 0.6f);
                e.midi.mod = 0.5f + 0.5f * std::sin ((float) beat);
            }
            if (inBeat >= (int) (rate * 0.35) && inBeat < (int) (rate * 0.35) + block) e.midi.allOff();
            e.render (L.data() + pos, R.data() + pos, juce::jmin (block, total - pos));
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        double secs = std::chrono::duration<double> (t1 - t0).count();

        if (getenv ("NABRA_DEBUG"))
        {
            for (auto probe : { std::pair<int,int>{1, e.outSlot (findCell ("wave2"), spec (MT::Wave).jackIndex ("bp"))},
                                std::pair<int,int>{2, e.inSlot (findCell ("wave2"), spec (MT::Wave).jackIndex ("in"))},
                                std::pair<int,int>{1, e.outSlot (findCell ("kee1"), spec (MT::Kee).jackIndex ("trig"))} })
            {
                e.probeKind = probe.first; e.probeSlot = probe.second;
                std::vector<float> dl (4096), dr (4096);
                for (int k = 0; k < 200; ++k) { if (k % 50 == 0) e.midi.noteOn (60, 1.0f); e.render (dl.data(), dr.data(), 4096); }
                float mx = -1e9f, mn = 1e9f;
                for (int k = 0; k < Engine::probeLen; ++k) { mx = std::max (mx, e.probeMax[(size_t) k].load()); mn = std::min (mn, e.probeMin[(size_t) k].load()); }
                printf ("   probe %d/%d: min %.3f max %.3f\n", probe.first, probe.second, mn, mx);
            }
        }
        double sum = 0; float peak = 0; bool bad = false;
        for (int i = 0; i < total; ++i)
        {
            if (! std::isfinite (L[(size_t) i]) || ! std::isfinite (R[(size_t) i])) bad = true;
            sum += L[(size_t) i] * L[(size_t) i] + R[(size_t) i] * R[(size_t) i];
            peak = juce::jmax (peak, std::abs (L[(size_t) i]), std::abs (R[(size_t) i]));
        }
        double rms = std::sqrt (sum / (2.0 * total));
        bool ok = ! bad && rms > 0.005 && peak < 1.0f;
        if (! ok) ++failures;
        printf ("%-42s rms %.4f  peak %.3f  realtime x%.0f  %s\n", recipes()[(size_t) r].name, rms, peak, seconds / secs, ok ? "ok" : "FAIL");

        // write at host rate (simple 2:1 average)
        juce::AudioBuffer<float> buf (2, total / 2);
        for (int i = 0; i < total / 2; ++i)
        {
            buf.setSample (0, i, 0.5f * (L[(size_t) 2 * i] + L[(size_t) 2 * i + 1]));
            buf.setSample (1, i, 0.5f * (R[(size_t) 2 * i] + R[(size_t) 2 * i + 1]));
        }
        auto f = outDir.getChildFile ("recipe" + juce::String (r + 1) + ".wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        if (auto os = std::unique_ptr<juce::FileOutputStream> (f.createOutputStream()))
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), hostRate, 2, 24, {}, 0)))
            {
                os.release();
                w->writeFromAudioSampleBuffer (buf, 0, buf.getNumSamples());
            }
    }
    printf (failures ? "%d FAILURES\n" : "all recipes ok\n", failures);
    return failures ? 1 : 0;
}
