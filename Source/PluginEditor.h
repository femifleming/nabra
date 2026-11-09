#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Help.h"

class NabraEditor;

// ---------------------------------------------------------------------------------------------
// The hanging canvas with all 56 boards, their loops, and the cords between them.
class Tapestry : public juce::Component, private juce::Timer
{
public:
    Tapestry (NabraProcessor&, NabraEditor&);
    ~Tapestry() override;

    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

    void invalidateCache() { cache = {}; repaint(); }
    void flash (const juce::String& msg);

    nabra::Cord handCord = nabra::Cord::Wire;
    int handRes = 1;

    // geometry (design space)
    static juce::Rectangle<float> cellRect (int cell);
    static juce::Rectangle<float> boardRect (int cell);
    static juce::Point<float> jackEdge (int cell, int jack);
    static juce::Point<float> jackTip (int cell, int jack);
    static juce::Point<float> knobPos (int cell);
    static juce::Point<float> dymoPos (int cell);

private:
    void timerCallback() override;
    void renderStatic (juce::Graphics&);
    void paintCard (juce::Graphics&);
    void paintDiagramCard (juce::Graphics&, int cell);

    nabra::JackRef jackAt (juce::Point<float>) const;
    int cableAt (juce::Point<float>) const;
    int knobAt (juce::Point<float>) const;
    int dymoAt (juce::Point<float>) const;
    int boardAt (juce::Point<float>) const;
    void setHoverJack (nabra::JackRef);
    void cableMenu (int id);
    void jackMenu (nabra::JackRef);
    juce::RangedAudioParameter* knobParam (int cell) const;

    NabraProcessor& proc;
    NabraEditor& editor;

    juce::Image cache;
    float cacheScale = 0;
    bool cacheLabels = true;
    bool cacheBoards = false;
    int rebuildCountdown = 0;

    nabra::JackRef hoverJack;
    juce::uint32 hoverJackTime = 0;
    int hoverCable = -1, hoverKnob = -1, hoverDymo = -1;
    juce::uint32 hoverDymoTime = 0;

    struct Drag
    {
        bool active = false;
        nabra::JackRef fixed;       // the end held still
        bool fixedIsOut = true;
        juce::Point<float> pos;
        nabra::Cord cord = nabra::Cord::Wire;
        int resVal = 1, colour = 0;
        bool pickedUp = false;
        nabra::JackRef pickedFrom;
    } drag;

    struct KnobDrag { int cell = -1; float startVal = 0; float startY = 0; } knobDrag;

    juce::String flashMsg;
    juce::uint32 flashTime = 0;
    std::array<int, 56> ledColour {};
};

// ---------------------------------------------------------------------------------------------
class TopBar : public juce::Component, private juce::Timer
{
public:
    TopBar (NabraProcessor&, NabraEditor&);
    ~TopBar() override;
    void paint (juce::Graphics&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override { repaint(); }
    int itemAt (juce::Point<float>) const;
    NabraProcessor& proc;
    NabraEditor& editor;
    std::vector<juce::Rectangle<float>> cordRects;
    std::vector<std::pair<juce::String, juce::Rectangle<float>>> buttons;
    juce::Point<float> masterPos;
    int hover = -1;
    float dragStart = 0, dragVal = 0;
    bool draggingMaster = false;
};

// ---------------------------------------------------------------------------------------------
class NabraEditor : public juce::AudioProcessorEditor, private juce::ChangeListener
{
public:
    explicit NabraEditor (NabraProcessor&);
    ~NabraEditor() override;
    void resized() override;
    void paint (juce::Graphics&) override {}
    bool keyPressed (const juce::KeyPress&) override;

    void pushUndo();
    void undo();
    void toggleHelp();
    void openHelpFor (nabra::MT);
    void showRecipeMenu (juce::Rectangle<float> near);
    void unpatchAll();
    void setHand (nabra::Cord, int resVal);
    void toggleLabels();
    void toggleBoards();

    Tapestry tapestry;
    TopBar topBar;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    NabraProcessor& proc;
    juce::Component root;
    HelpPanel help;
    std::vector<std::vector<nabra::Cable>> undoStack;
};
