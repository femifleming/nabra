#pragma once
// Nabra — static description of the tapestry: module types, their copper-loop jacks,
// where every board hangs, cords, and the recipes from the thesis.
#include <juce_core/juce_core.h>
#include <vector>

namespace nabra
{
enum class Side { L, R, T, B };

enum class MT
{
    Slope, Port, Shot, Dust, Stair, Rand, Trand, Swoop, Pulse,
    Trang, Noise, Sine, Wave, Pow, Out,
    Kee, Duo, Wheel, Tick,
    Count
};

struct JackSpec
{
    const char* id;      // unique within a module, used in saved patches  ("angleA")
    const char* label;   // what's written on the tape ("ANGLE"); lowercase = input, UPPERCASE = output
    bool out;
    Side side;
    float pos;           // 0..1 along that edge (top->bottom, left->right)
    const char* help;
    bool inv = false;    // "invert" inputs get the black arrow / black tape, as in the diagrams
};

struct TypeSpec
{
    MT type;
    const char* name;      // "Slope"
    const char* res;       // resource base name ("slope") — photo/diagram
    std::vector<JackSpec> jacks;  // inputs first (DSP order), then outputs (DSP order)
    bool hasKnob;                 // audio modules carry a volume pot into the main mix
    std::vector<const char*> mixJacks;  // outputs that feed the main mix through the pot
    const char* ledJack;
    const char* summary;   // plain-language how-to
    const char* thesis;    // the author's own words (short excerpt)
    bool midi = false;     // part of the sewn-on MIDI strip (not in the original tapestry)

    // derived
    std::vector<int> inSlot, outSlot; // per jack: slot among inputs / outputs, or -1
    int numIn = 0, numOut = 0;
    int jackIndex (const juce::String& id) const;
};

const TypeSpec& spec (MT);

struct Cell
{
    MT type;
    int col, row;   // col 0 = MIDI strip, 1..13 = the original twelve-and-a-bit columns
    int num;        // 1-based instance number within its type
};

const std::vector<Cell>& cells();
juce::String cellName (int cellIdx);  // "Slope 3"
juce::String cellKey  (int cellIdx);  // "slope3"
int findCell (const juce::String& key);
int knobParamCount();
int knobParamIndex (int cellIdx);     // -1 if no pot

// ---------- cords ----------
enum class Cord { Wire = 0, Resistor, Capacitor, Diode };
// resistor values: 1k / 10k / 100k, into a nominal 10k input
inline float resistorGain (int v) { static const float g[] = { 0.91f, 0.5f, 0.09f }; return g[juce::jlimit (0, 2, v)]; }
inline const char* resistorName (int v) { static const char* n[] = { "1k", "10k", "100k" }; return n[juce::jlimit (0, 2, v)]; }
const char* cordName (Cord);

struct JackRef
{
    int cell = -1, jack = -1;
    bool valid() const { return cell >= 0 && jack >= 0; }
    bool operator== (const JackRef& o) const { return cell == o.cell && jack == o.jack; }
};

struct Cable
{
    int id = 0;
    JackRef src, dst;   // src is always an OUTPUT, dst an input
    Cord cord = Cord::Wire;
    int resVal = 1;
    int colour = 0;
};

// ---------- recipes ----------
struct Recipe
{
    const char* name;
    const char* notation;   // the thesis' own dot-notation
    const char* explain;
    // "slope4.angleA>sine5.envA[:r1|:c|:d]" ...
    std::vector<const char*> links;
    bool fromThesis;
};
const std::vector<Recipe>& recipes();
std::vector<Cable> buildRecipe (const Recipe&);

// design-space layout (the editor scales this whole thing)
namespace layout
{
    constexpr float topH = 64, cellW = 140, cellH = 188, margin = 26, gap = 30, padTop = 14;
    constexpr float designW = margin + cellW + gap + 13 * cellW + margin;
    constexpr float designH = topH + padTop + 4 * cellH + padTop;
}
} // namespace nabra
