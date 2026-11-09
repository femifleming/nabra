// Renders the real editor offscreen (main view, help pages) to PNGs for visual checking.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/Art.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    juce::File out (argc > 1 ? argv[1] : "/tmp");
    NabraProcessor p;
    p.setPlayConfigDetails (0, 2, 48000.0, 512);
    p.prepareToPlay (48000.0, 512);
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer mb;
    mb.addEvent (juce::MidiMessage::noteOn (1, 57, 0.8f), 0);
    for (int i = 0; i < 120; ++i) { p.processBlock (buf, mb); mb.clear(); }
    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    auto* e = dynamic_cast<NabraEditor*> (ed.get());
    e->setSize ((int) nabra::layout::designW, (int) nabra::layout::designH);
    auto save = [&] (const juce::String& name, float scale)
    {
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, scale);
        auto f = out.getChildFile (name);
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat().writeImageToStream (img, os);
        printf ("wrote %s %dx%d\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    };
    for (auto [n, img] : { std::pair<const char*, juce::Image> { "dbg_raw.png", art::rawPhoto ("slope") }, { "dbg_photo.png", art::photo ("slope") }, { "dbg_midi.png", art::midiBoard (nabra::MT::Kee) } })
    {
        printf ("%s valid %d fmt %d %dx%d px(200,250)=%s\n", n, img.isValid(), (int) img.getFormat(), img.getWidth(), img.getHeight(), img.isValid() ? img.getPixelAt (200, 250).toDisplayString (true).toRawUTF8() : "-");
        auto f = out.getChildFile (n); f.deleteFile(); juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    save ("main.png", 1.0f);
    save ("main2x.png", 2.0f);
    e->toggleBoards();
    save ("main_colour.png", 1.0f);
    e->toggleBoards();
    e->toggleHelp();
    save ("help_start.png", 1.0f);
    e->openHelpFor (nabra::MT::Slope);
    save ("help_slope.png", 1.0f);
    e->openHelpFor (nabra::MT::Kee);
    save ("help_kee.png", 1.0f);
    { // roundtrip
        p.loadRecipe (5);
        juce::MemoryBlock mbk; p.getStateInformation (mbk);
        NabraProcessor q; q.setStateInformation (mbk.getData(), (int) mbk.getSize());
        bool same = q.getCables().size() == p.getCables().size();
        for (size_t i = 0; same && i < q.getCables().size(); ++i)
            same = q.getCables()[i].src == p.getCables()[i].src && q.getCables()[i].dst == p.getCables()[i].dst && q.getCables()[i].cord == p.getCables()[i].cord;
        printf ("state roundtrip (%d cords): %s\n", (int) p.getCables().size(), same ? "ok" : "FAIL");
    }
    return 0;
}
