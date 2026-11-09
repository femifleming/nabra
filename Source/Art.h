#pragma once
// Everything that makes it look like the thesis: splatter-painted canvas, the photographed boards,
// braided copper loops, hand labels, Dymo tape, and alligator cords with in-line parts.
#include <juce_gui_basics/juce_gui_basics.h>
#include "Specs.h"

namespace art
{
using juce::Graphics;
using juce::Point;
using juce::Rectangle;

juce::Image photo (const juce::String& res);      // feathered board photo (ARGB)
juce::Image rawPhoto (const juce::String& res);   // as in the thesis
juce::Image diagram (const juce::String& res);    // the hand-drawn module diagram
juce::Image midiBoard (nabra::MT type);           // drawn stand-in board for the MIDI strip
juce::Image boardScan (nabra::MT type, int num);  // colour scan of a real board (alternate theme)

juce::Font font (float h, bool bold = false, bool italic = false);
juce::Font serif (float h, bool bold = false, bool italic = false);
juce::Font mono (float h, bool bold = false);

void paintCanvas (Graphics&, Rectangle<float> area, uint32_t seed, float density = 1.0f);
void paintSewnPatch (Graphics&, Rectangle<float> area);
void paintBoardShadow (Graphics&, Rectangle<float> board);

// a jack: the braided loop leaving the board edge at p; returns the point where clips bite
Point<float> loopTip (Point<float> edge, nabra::Side side);
void drawLoop (Graphics&, Point<float> edge, nabra::Side side, bool lit);
void drawTape (Graphics&, Rectangle<float> r, const juce::String& text, bool out, bool inv, float angle = 0.0f);
float tapeWidth (const juce::String& text, bool out);
void drawDymo (Graphics&, Point<float> centre, const juce::String& text);
float dymoWidth (const juce::String& text);

juce::Colour cordColour (int idx);
juce::Path cablePath (Point<float> a, Point<float> b);
void drawCable (Graphics&, Point<float> a, Point<float> b, int colour, nabra::Cord, int resVal, bool hover, float alpha = 1.0f,
                bool clipA = true, bool clipB = true);
void drawCordIcon (Graphics&, Rectangle<float> r, nabra::Cord, int resVal);
float distanceToCable (Point<float> a, Point<float> b, Point<float> p);

void drawKnob (Graphics&, Point<float> c, float r, float value01, bool hover);
juce::Colour ledColour (int idx);
void drawLed (Graphics&, Point<float> c, int colourIdx, float level);
}
