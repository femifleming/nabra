#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Engine.h"

class NabraProcessor : public juce::AudioProcessor, public juce::ChangeBroadcaster
{
public:
    NabraProcessor();
    ~NabraProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nabra"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int) override;
    const juce::String getProgramName (int) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- patch editing (message thread) ----
    const std::vector<nabra::Cable>& getCables() const { return cables; }
    int addCable (nabra::JackRef src, nabra::JackRef dst, nabra::Cord cord, int resVal);
    void removeCable (int id);
    void updateCable (const nabra::Cable&);
    void clearCables();
    void loadRecipe (int index);
    void setCables (std::vector<nabra::Cable>);

    nabra::Engine& getEngine() { return engine; }
    juce::AudioProcessorValueTreeState apvts;

    float uiScale = 1.0f;          // zoom
    float panX = 0.0f, panY = 64.0f;
    int uiW = 1300, uiH = 860;
    bool showLabels = true;
    bool colourBoards = false;   // alternate theme: colour scans of the real circuit boards

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout makeLayout();
    void pushPatch();

    nabra::Engine engine;
    std::vector<nabra::Cable> cables;
    int nextId = 1, nextColour = 0;
    int currentProgram = 0;

    juce::SpinLock patchLock;
    std::vector<nabra::Cable> pendingPatch;
    bool patchPending = false;

    std::unique_ptr<juce::dsp::Oversampling<float>> os;
    juce::AudioBuffer<float> work;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NabraProcessor)
};
