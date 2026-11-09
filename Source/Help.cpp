#include "Help.h"
#include "Art.h"

using namespace juce;
using namespace nabra;

namespace
{
const Colour paper (0xfff6f3ea), ink (0xff1b1a18), faint (0xff6d6a62), rule (0xffcfc9b9), accent (0xff8a4a22);

AttributedString styled (const String& text, const Font& base, Colour col, bool italic = false)
{
    // **bold** markup
    AttributedString as;
    as.setWordWrap (AttributedString::byWord);
    as.setLineSpacing (3.0f);
    auto bold = Font (base).boldened();
    auto normal = italic ? Font (base).italicised() : base;
    int i = 0;
    bool b = false;
    while (i < text.length())
    {
        int j = text.indexOf (i, "**");
        if (j < 0) j = text.length();
        if (j > i) as.append (text.substring (i, j), b ? bold : normal, col);
        b = ! b;
        i = j + 2;
    }
    return as;
}
} // namespace

// =====================================================================================
class HelpPanel::Content : public Component
{
public:
    explicit Content (HelpPanel& o) : owner (o) {}
    const Page* page = nullptr;

    struct Laid { const Block* b; Rectangle<float> r; TextLayout tl; Rectangle<float> button; };
    std::vector<Laid> laid;
    int hoverRecipe = -1;

    float layoutFor (float w)
    {
        laid.clear();
        if (page == nullptr) return 0;
        float y = 18;
        const float x = 28, tw = w - 56;
        for (auto& b : page->blocks)
        {
            Laid l; l.b = &b;
            switch (b.kind)
            {
                case Block::Heading:
                {
                    l.tl.createLayout (styled (b.text, art::serif (24, true), ink), tw);
                    y += 6;
                    l.r = { x, y, tw, l.tl.getHeight() };
                    y += l.tl.getHeight() + 10;
                    break;
                }
                case Block::Para: case Block::Note:
                {
                    l.tl.createLayout (styled (b.text, art::serif (15.5f), ink), b.kind == Block::Note ? tw - 30 : tw);
                    float pad = b.kind == Block::Note ? 12.0f : 0.0f;
                    l.r = { x, y, tw, l.tl.getHeight() + pad * 2 };
                    y += l.r.getHeight() + 12;
                    break;
                }
                case Block::Quote:
                {
                    l.tl.createLayout (styled (b.text, art::serif (15, false, true), Colour (0xff3d3a34), true), tw - 48);
                    l.r = { x, y, tw, l.tl.getHeight() + 8 };
                    y += l.r.getHeight() + 14;
                    break;
                }
                case Block::Steps:
                {
                    l.tl.createLayout (styled (b.text, art::serif (15.5f), ink), tw - 34);
                    l.r = { x, y, tw, l.tl.getHeight() };
                    y += l.tl.getHeight() + 8;
                    break;
                }
                case Block::Image:
                {
                    l.r = { x, y, tw, b.imageH };
                    y += b.imageH + 16;
                    break;
                }
                case Block::Jacks:
                {
                    auto& sp = spec (b.mt);
                    float h = 0;
                    for (auto& j : sp.jacks)
                    {
                        TextLayout t; t.createLayout (styled (j.help, art::serif (14.5f), ink), tw - 170);
                        h += juce::jmax (22.0f, t.getHeight() + 8);
                    }
                    l.r = { x, y, tw, h + 30 };
                    y += l.r.getHeight() + 12;
                    break;
                }
                case Block::Recipe:
                {
                    auto& r = recipes()[(size_t) b.recipe];
                    AttributedString as = styled (String ("**") + r.name + "**\n", art::serif (18), ink);
                    as.append (String (r.notation) + "\n", art::mono (13), accent);
                    as.append (r.explain, art::serif (15.5f), ink);
                    l.tl.createLayout (as, tw - 190);
                    l.r = { x, y, tw, juce::jmax (70.0f, l.tl.getHeight() + 24) };
                    l.button = { x + tw - 160, y + 14, 140, 34 };
                    y += l.r.getHeight() + 14;
                    break;
                }
            }
            laid.push_back (std::move (l));
        }
        return y + 40;
    }

    void paint (Graphics& g) override
    {
        g.fillAll (paper);
        for (auto& l : laid)
        {
            auto& b = *l.b;
            switch (b.kind)
            {
                case Block::Heading:
                    l.tl.draw (g, l.r);
                    g.setColour (rule);
                    g.drawHorizontalLine ((int) l.r.getBottom() + 3, l.r.getX(), l.r.getRight());
                    break;
                case Block::Para: l.tl.draw (g, l.r); break;
                case Block::Note:
                    g.setColour (Colour (0xffeee2c4)); g.fillRect (l.r);
                    g.setColour (Colour (0xffd2c49f)); g.drawRect (l.r, 1.0f);
                    l.tl.draw (g, l.r.reduced (15, 12));
                    break;
                case Block::Quote:
                    g.setColour (accent.withAlpha (0.6f));
                    g.fillRect (l.r.getX() + 8, l.r.getY(), 3.0f, l.r.getHeight());
                    l.tl.draw (g, l.r.withTrimmedLeft (30).withTrimmedTop (4));
                    break;
                case Block::Steps:
                    g.setColour (accent);
                    g.fillEllipse (l.r.getX() + 8, l.r.getY() + 7, 7, 7);
                    l.tl.draw (g, l.r.withTrimmedLeft (34));
                    break;
                case Block::Image: drawImageBlock (g, b, l.r); break;
                case Block::Jacks: drawJacks (g, b, l.r); break;
                case Block::Recipe:
                {
                    g.setColour (Colour (0xfffbfaf4)); g.fillRect (l.r);
                    g.setColour (rule); g.drawRect (l.r, 1.0f);
                    l.tl.draw (g, l.r.reduced (14, 12).withTrimmedRight (176));
                    bool h = hoverRecipe == b.recipe;
                    g.setColour (h ? Colour (0xff2a2a2c) : Colour (0xff141414));
                    g.fillRoundedRectangle (l.button, 3);
                    g.setColour (Colours::white);
                    g.setFont (Font (FontOptions ("Arial", 12.0f, Font::bold).withKerningFactor (0.1f)));
                    g.drawText ("LOAD THIS PATCH", l.button, Justification::centred);
                    break;
                }
            }
        }
    }

    void drawImageBlock (Graphics& g, const Block& b, Rectangle<float> r)
    {
        auto drawFit = [&] (const Image& img, Rectangle<float> area, const String& caption)
        {
            if (! img.isValid()) return;
            auto cap = area.removeFromBottom (20);
            auto placed = RectanglePlacement (RectanglePlacement::centred).appliedTo (img.getBounds().toFloat(), area);
            g.setColour (Colour (0x22000000)); g.fillRect (placed.translated (2, 3));
            g.setOpacity (1.0f);
            g.drawImage (img, placed);
            g.setColour (faint); g.setFont (art::serif (12.5f, false, true));
            g.drawText (caption, cap, Justification::centred);
        };
        if (b.image == "tapestry")
            drawFit (art::rawPhoto ("tapestry"), r, "Illustration 28: Nabra (2001) - 52 boards on splatter-painted canvas");
        else if (b.image == "dog")
            drawFit (art::rawPhoto ("dog"), r, "\"the dog\": the external transformer box that powers the hanging tapestry");
        else if (b.image.startsWith ("module:"))
        {
            auto& sp = spec (b.mt);
            auto left = r.removeFromLeft (r.getWidth() * 0.55f);
            if (sp.midi)
            {
                drawFit (art::midiBoard (b.mt), r.reduced (10, 0), "stand-in board (MIDI strip)");
                g.setColour (faint); g.setFont (art::serif (14, false, true));
                g.drawFittedText ("No diagram in the thesis: the MIDI strip is an addition for the plug-in. Outputs only, all UPPERCASE.",
                                  left.reduced (10).toNearestInt(), Justification::centred, 4);
            }
            else
            {
                g.setColour (Colours::white); g.fillRect (left.reduced (4, 0).withTrimmedBottom (20));
                drawFit (art::diagram (sp.res), left.reduced (10, 4), String ("diagram: ") + sp.name + " (lowercase = input, UPPERCASE = output)");
                drawFit (art::rawPhoto (sp.res), r.reduced (10, 0), String ("the ") + sp.name + " board on the tapestry");
            }
        }
    }

    void drawJacks (Graphics& g, const Block& b, Rectangle<float> r)
    {
        auto& sp = spec (b.mt);
        float y = r.getY();
        g.setColour (faint); g.setFont (art::font (11, true));
        g.drawText ("LOOP", Rectangle<float> (r.getX(), y, 90, 20), Justification::centredLeft);
        g.drawText ("WHERE", Rectangle<float> (r.getX() + 96, y, 70, 20), Justification::centredLeft);
        g.drawText ("WHAT IT DOES", Rectangle<float> (r.getX() + 170, y, 200, 20), Justification::centredLeft);
        y += 26;
        static const char* sideName[] = { "left", "right", "top", "bottom" };
        for (auto& j : sp.jacks)
        {
            TextLayout t; t.createLayout (styled (j.help, art::serif (14.5f), ink), r.getWidth() - 170);
            float h = juce::jmax (22.0f, t.getHeight() + 8);
            float w = art::tapeWidth (j.label, j.out);
            art::drawTape (g, { r.getX() + 2, y + 2, w, 13 }, j.label, j.out, j.inv);
            g.setColour (faint); g.setFont (art::serif (13, false, true));
            g.drawText (String (j.out ? "out, " : "in, ") + sideName[(int) j.side], Rectangle<float> (r.getX() + 96, y, 74, 18), Justification::centredLeft);
            t.draw (g, { r.getX() + 170, y, r.getWidth() - 170, t.getHeight() });
            y += h;
            g.setColour (rule.withAlpha (0.5f));
            g.drawHorizontalLine ((int) y - 4, r.getX(), r.getRight());
        }
    }

    void mouseMove (const MouseEvent& e) override
    {
        int h = -1;
        for (auto& l : laid) if (l.b->kind == Block::Recipe && l.button.contains (e.position)) h = l.b->recipe;
        if (h != hoverRecipe) { hoverRecipe = h; repaint(); }
        setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    }
    void mouseDown (const MouseEvent& e) override
    {
        for (auto& l : laid)
            if (l.b->kind == Block::Recipe && l.button.contains (e.position) && owner.onLoadRecipe)
            {
                owner.onLoadRecipe (l.b->recipe);
                return;
            }
    }

    HelpPanel& owner;
};

// =====================================================================================
HelpPanel::HelpPanel()
{
    content = std::make_unique<Content> (*this);
    viewport.setViewedComponent (content.get(), false);
    viewport.setScrollBarsShown (true, false);
    viewport.setScrollBarThickness (10);
    viewport.getVerticalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0x60000000));
    addAndMakeVisible (viewport);
    setWantsKeyboardFocus (true);
    build();
    showPage (0);
}

HelpPanel::~HelpPanel() = default;

void HelpPanel::build()
{
    using B = Block;
    auto H = [] (const String& t) { return B { B::Heading, t }; };
    auto P = [] (const String& t) { return B { B::Para, t }; };
    auto Q = [] (const String& t) { return B { B::Quote, t }; };
    auto S = [] (const String& t) { return B { B::Steps, t }; };
    auto N = [] (const String& t) { return B { B::Note, t }; };
    auto I = [] (const String& img, float h) { B b { B::Image, {} }; b.image = img; b.imageH = h; return b; };

    pages.push_back ({ "Start here", "GUIDE", {
        H ("Nabra: an analog brain"),
        I ("tapestry", 330),
        P ("This plug-in rebuilds the **Nabra**, the hanging tapestry synthesizer described in Peter Blasser's thesis "
           "**Stores at the Mall** (Wesleyan, 2015). Fifty-two circuit boards face out of a splatter-painted canvas, "
           "with loops of braided copper as patch points, joined by alligator-clip cords."),
        Q ("\"It is basically a modular synthesizer, with a few differences... It has no knobs for tuning... tuning is a given for each module, "
           "and the only way to change it is by modulation. However, there are knobs for volume of the audio modules, so they can be mixed on site.\""),
        P ("The columns run like a sentence, left to right: **control > process > generate > filter**. Slow shapes on the left (Slope, Shot, Dust, Stair, Rand, Trand, "
           "Swoop, Pulse) drive the sound makers (Trang, Noise, Sine), which can go on through the filters (Wave) into the mix (Out, bottom right). "
           "On the far left, a strip of raw canvas has been sewn on: the **MIDI strip**, which turns your DAW's notes into voltages."),
        H ("Your first patch, in 30 seconds"),
        S ("Click **unpatch all** in the top bar, so the tapestry is empty and silent. (See **Getting around** for panning and zooming the view.)"),
        S ("Find **Slope 3** (second column from the left of the painted canvas, top). Press on the copper loop labelled **ANGLE** on its top edge and drag. A cord follows your mouse."),
        S ("Let go on the loop labelled **env** at the top of **Sine 5** (top of the right-hand column of Sines). The Sine now breathes in and out with the slow triangle."),
        S ("Now drag from Slope 3's **CREST** (right edge, top) to **trig** on **Swoop 2**, and from Swoop 2's **SWOOP** to **env** on **Noise 3**. You have just built the first recipe in the thesis."),
        S ("Hover over any loop to see what it does and a live trace of its voltage. Hover over a module's black name tape to see its diagram from the thesis."),
        N ("Every audio module is **silent until something opens its env input**. If you hear nothing, an env is missing. "
           "The little knob on each audio board is its volume into the mix."),
    } });

    pages.push_back ({ "Getting around", "GUIDE", {
        H ("Getting around the tapestry"),
        P ("The window looks onto part of the tapestry, close enough to read on a laptop; move the view around it, or press **FIT** to see the whole thing at once."),
        S ("**Pan:** two-finger swipe on the trackpad or scroll wheel (shift-scroll goes sideways), or press and drag on any bare canvas or board. Arrow keys work too."),
        S ("**Zoom:** pinch on the trackpad, or hold **cmd** and scroll; the **-** and **+** buttons; or the - and = keys. Zoom goes from 'whole tapestry' up to 250%."),
        S ("**Minimap:** the little picture of the tapestry in the top bar shows where you are (red box) and every cord. Click or drag on it to go anywhere."),
        S ("**FIT** (or the 0 key) shows the whole tapestry at once, like the original Nabra window."),
        S ("**Long cords:** while dragging a cord, push the mouse to the edge of the window and the view scrolls along with you, so you can reach a board that's off screen."),
        S ("Resize the window freely - the view, zoom and window size are saved with your project."),
        N ("Order of the columns, left to right: **MIDI strip** | Slope Slope | Shot | Dust/Stair | Rand/Trand | Swoop | Pulse | Trang | Noise | Sine Sine | Wave | Pow/Out."),
    } });

    pages.push_back ({ "Patching", "GUIDE", {
        H ("How to patch"),
        P ("**Make a connection:** press on a copper loop and drag to another loop. One end must be an **OUTPUT** (UPPERCASE, white tape) and the other an **input** "
           "(lowercase, cream tape). You can start from either end."),
        P ("**Black tape = invert.** In the diagrams, invert inputs have a solid black arrow; here they wear black tape. Almost every module has a "
           "**vert** (speeds it up / raises it) and an **invert** (slows it down / lowers it). With no knobs, this pair is how you tune anything."),
        P ("**Many into one:** patch as many outputs into one input as you like; they add together. That is the Nabra way: "
           "\"relying on a multiplicity of inputs to balance a multiplicity of outputs.\""),
        P ("**One into many:** an output can feed any number of inputs. Drag from it again."),
        P ("**Move a cord:** press on an input loop that already has a cord and drag it away; you pick up the most recent cord. Drop it on another input to re-route, "
           "or drop it on bare canvas to remove it. Hold **shift** to start a new cord from an occupied input instead."),
        P ("**Remove a cord:** double-click it, or right-click it for a menu (change its in-line part, change resistor value, remove). Right-clicking a loop lists all its cords."),
        P ("**Never cross two outputs.** The Nabra has hot outputs with no current-limiting resistors; the plug-in refuses output-to-output connections, as the author would want."),
        P ("**Feedback is allowed.** Patch a module into itself, or the final **OUT** back into something. Expect chaos; that's the point."),
        P ("**Undo:** cmd-Z (or the undo button) steps back through your patching."),
        H ("Reading the board"),
        P ("Each board shows its **name** on black Dymo tape, a random vintage **LED** (red, green or amber, \"placed into the instrument at random without respect to any function\") "
           "that flickers with its main output, and copper loops around its edge, each labelled with tape. Turn tape labels off with **labels** in the top bar if you want the "
           "unlabelled, \"intentionally unintelligible\" original. **colour** in the top bar swaps the black-and-white thesis photographs for colour scans of the real circuit boards; click it again to go back."),
        P ("**Hover a loop** for its description and a ~3 second oscilloscope of the actual voltage there. **Hover a name tape** to see the module's diagram. "
           "**Right-click a board** to open its help page."),
        H ("Voltages"),
        P ("Control signals swing **0..5V**. Pulses are **5V for 1.5 ms** - anything above about 2V counts as a trigger. Audio swings about **+-5V**. "
           "All rate inputs are **linear** (no exponential converters, as in the original): +5V into vert roughly doubles-to-triples a module's speed, "
           "and enough invert can stall it. Pow gives you fixed +5V / -5V to bias things."),
    } });

    pages.push_back ({ "Cords", "GUIDE", {
        H ("Cords with parts in them"),
        Q ("\"I designed the Nabra for patch cords with in-line passive elements, such as resistors, capacitors, or even diodes. I intended for compositional strategy "
           "to include the type of cord, and thus the type of mixing in the connection of outputs.\""),
        P ("Choose the cord in your hand from the top bar before you patch. Change any existing cord with right-click."),
        P ("**Wire** - the plain connection, full strength."),
        P ("**Resistor** (1k / 10k / 100k, click the resistor button again to change value) - scales the signal down: 1k passes ~90%, 10k half, 100k about a tenth. "
           "Use it to apportion how much of a control voltage reaches an input, or for gentle FM. Pow POS through a 100k resistor into a vert is a fine-tuning knob."),
        P ("**Capacitor** - passes changes, blocks steady voltage. A square becomes a pair of spikes (a trigger on every edge), a slow ramp almost disappears, "
           "a gate becomes a single pulse. Great for turning gates into triggers and for removing offsets."),
        P ("**Diode** - only lets positive voltage through, minus a 0.6V drop. Half-wave rectifies audio into something buzzier; keeps negative swings "
           "of a modulator from reaching an input; lets several envelopes share an input without pulling each other down."),
    } });

    pages.push_back ({ "MIDI to CV", "GUIDE", {
        H ("The MIDI strip"),
        P ("The far-left strip of raw canvas is not part of the 2000 tapestry: it is the bridge from your DAW. Put Nabra on an instrument track, send it MIDI, "
           "and four boards turn it into voltages you patch like anything else."),
        P ("**Kee** - monophonic, last-note priority. **KEY** pitch, **GATE** while held, **TRIG** on every note, **VELO**, **OFF** on release."),
        P ("**Duo** - the **LOW**est and **HIGH**est held notes with their own gates: two voices from intervals and chords."),
        P ("**Wheel** - mod wheel, pitch bend, aftertouch, CC74 and CC11 as 0..5V."),
        P ("**Tick** - the host transport: BEAT, SIXTeenth and BAR pulses, RUN gate, and a RAMP across each bar."),
        H ("Playing in tune on a linear synth"),
        P ("The Nabra's oscillators respond linearly, so ordinary 1V/octave would not work. Instead the strip speaks **linear volts: 1V = 100Hz** "
           "(A440 = 4.4V). The Sine, Trang and Wave boards have one extra loop, **key**, which replaces the tuning capacitor: patch a KEY there and the module "
           "plays the note. vert, invert and fm still bend it around that note."),
        S ("**Simplest instrument:** Kee **KEY** > Sine **key**. Kee **GATE** > Sine **env**. Play."),
        S ("**Softer envelope:** route GATE through a **Port** (in > OUT) before the env. Port smooths the gate into an attack and release."),
        S ("**Glide:** route KEY through the other half of the Port before the key input."),
        S ("**Pluck:** Kee **TRIG** > Wave **in** (a capacitor cord sharpens it), Kee **KEY** > Wave **key**. The resonant filter rings at the note."),
        S ("**Clocked:** Tick **BEAT** > Slope **sync** to lock the slow shapes to the song, or SIXT > Trand trig for a random melody in time."),
        N ("Recipe **4. Kee sings** is loaded when the plug-in starts: just play. See the Recipes page for more."),
    } });

    {
        Page r { "Recipes", "GUIDE", {
            H ("Recipes"),
            P ("The thesis writes patches in a nested notation: **Module.input(Module.OUTPUT)**. So Sine.env(Slope.ANGLE) means: from Slope's ANGLE loop to Sine's env loop. "
               "\"I like to use slow slopes to mark off sines in space, then modulate those sines with random melodies.\" The first three are his; the rest use the MIDI strip."),
        } };
        for (int i = 0; i < (int) recipes().size(); ++i)
        {
            B b { B::Recipe, {} }; b.recipe = i; r.blocks.push_back (b);
        }
        pages.push_back (r);
    }

    for (int t = 0; t < (int) MT::Count; ++t)
    {
        auto mt = (MT) t;
        auto& sp = spec (mt);
        Page p { sp.name, sp.midi ? "MIDI STRIP" : "MODULES", {} };
        p.blocks.push_back (H (sp.name));
        B img { B::Image, {} }; img.image = "module:"; img.mt = mt; img.imageH = 300; p.blocks.push_back (img);
        p.blocks.push_back (P (sp.summary));
        p.blocks.push_back (Q (sp.thesis));
        if (mt == MT::Pow) p.blocks.push_back (I ("dog", 200));
        B j { B::Jacks, {} }; j.mt = mt; p.blocks.push_back (j);
        pages.push_back (p);
    }
}

void HelpPanel::showPage (int i)
{
    current = juce::jlimit (0, (int) pages.size() - 1, i);
    content->page = &pages[(size_t) current];
    resized();
    viewport.setViewPosition (0, 0);
    repaint();
}

void HelpPanel::showModule (MT t)
{
    for (int i = 0; i < (int) pages.size(); ++i)
        if (pages[(size_t) i].title == spec (t).name) { showPage (i); return; }
}

void HelpPanel::resized()
{
    panel = getLocalBounds().toFloat().reduced (40, 18);
    navArea = panel.withWidth (210).reduced (0, 50);
    closeBtn = Rectangle<float> (34, 34).withPosition (panel.getRight() - 46, panel.getY() + 10);
    auto vp = panel.withTrimmedLeft (220).reduced (8, 0).withTrimmedTop (52).withTrimmedBottom (12);
    viewport.setBounds (vp.toNearestInt());
    float w = (float) viewport.getMaximumVisibleWidth();
    float h = content->layoutFor (w);
    content->setSize ((int) w, (int) h);
    content->repaint();

    navRects.clear();
    float y = navArea.getY();
    String group;
    for (auto& p : pages)
    {
        if (p.group != group) { group = p.group; y += 22; }
        navRects.push_back ({ navArea.getX() + 14, y, navArea.getWidth() - 20, 21 });
        y += 21;
    }
}

void HelpPanel::paint (Graphics& g)
{
    g.fillAll (Colour (0x99000000));
    g.setColour (Colour (0x55000000)); g.fillRect (panel.translated (4, 6));
    g.setColour (paper); g.fillRect (panel);
    // the sidebar is a strip of manila
    auto side = panel.withWidth (220);
    g.setColour (Colour (0xffebe3cf)); g.fillRect (side);
    g.setColour (rule); g.drawVerticalLine ((int) side.getRight(), side.getY(), side.getBottom());

    g.setColour (ink); g.setFont (art::serif (26, true));
    g.drawText ("Nabra", side.withHeight (50).reduced (18, 0), Justification::centredLeft);
    g.setColour (faint); g.setFont (art::serif (13, false, true));
    g.drawText ("the help book", side.withHeight (50).reduced (18, 0).translated (78, 3), Justification::centredLeft);

    String group;
    for (size_t i = 0; i < pages.size(); ++i)
    {
        auto& p = pages[i];
        auto r = navRects[i];
        if (p.group != group)
        {
            group = p.group;
            g.setColour (faint); g.setFont (art::font (10.5f, true));
            g.drawText (group, r.translated (0, -20), Justification::centredLeft);
        }
        if ((int) i == current) { g.setColour (Colour (0xff1d1d1f)); g.fillRect (r.expanded (6, 0)); }
        else if ((int) i == hoverNav) { g.setColour (Colour (0x18000000)); g.fillRect (r.expanded (6, 0)); }
        g.setColour ((int) i == current ? Colours::white : ink);
        g.setFont (art::serif (15, (int) i == current));
        g.drawText (p.title, r, Justification::centredLeft);
    }

    // page title strip
    auto top = panel.withTrimmedLeft (220).withHeight (50);
    g.setColour (faint); g.setFont (art::serif (13, false, true));
    g.drawText ("press Esc or ? to return to the tapestry", top.reduced (28, 0), Justification::centredLeft);
    g.setColour (Colour (0xff1d1d1f)); g.fillEllipse (closeBtn);
    g.setColour (Colours::white);
    auto c = closeBtn.getCentre();
    g.drawLine (c.x - 7, c.y - 7, c.x + 7, c.y + 7, 2.2f);
    g.drawLine (c.x + 7, c.y - 7, c.x - 7, c.y + 7, 2.2f);
}

void HelpPanel::mouseDown (const MouseEvent& e)
{
    if (closeBtn.contains (e.position) || ! panel.contains (e.position)) { if (onClose) onClose(); return; }
    for (size_t i = 0; i < navRects.size(); ++i)
        if (navRects[i].expanded (6, 0).contains (e.position)) { showPage ((int) i); return; }
}

void HelpPanel::mouseMove (const MouseEvent& e)
{
    int h = -1;
    for (size_t i = 0; i < navRects.size(); ++i)
        if (navRects[i].expanded (6, 0).contains (e.position)) h = (int) i;
    if (h != hoverNav) { hoverNav = h; repaint(); }
}

bool HelpPanel::keyPressed (const KeyPress& k)
{
    if (k == KeyPress::escapeKey || k.getTextCharacter() == '?') { if (onClose) onClose(); return true; }
    if (k == KeyPress::downKey) { showPage (current + 1); return true; }
    if (k == KeyPress::upKey) { showPage (current - 1); return true; }
    return false;
}
