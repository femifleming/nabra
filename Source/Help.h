#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Specs.h"

// The help book: how to patch, the cords, MIDI->CV, the thesis recipes, and a page per module
// with its diagram, photo, the author's words and every copper loop explained.
class HelpPanel : public juce::Component
{
public:
    HelpPanel();
    ~HelpPanel() override;

    void showPage (int index);
    void showModule (nabra::MT);
    std::function<void (int)> onLoadRecipe;
    std::function<void()> onClose;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

    struct Block
    {
        enum Kind { Heading, Para, Quote, Image, Jacks, Recipe, Steps, Note } kind;
        juce::String text;
        juce::String image;     // "photo:slope", "diagram:slope", "tapestry", "midi:Kee"
        nabra::MT mt = nabra::MT::Slope;
        int recipe = -1;
        float imageH = 220.0f;
    };
    struct Page { juce::String title; juce::String group; std::vector<Block> blocks; };

private:
    class Content;
    std::vector<Page> pages;
    int current = 0, hoverNav = -1;
    std::unique_ptr<Content> content;
    juce::Viewport viewport;
    juce::Rectangle<float> navArea, closeBtn, panel;
    std::vector<juce::Rectangle<float>> navRects;
    void build();
};
