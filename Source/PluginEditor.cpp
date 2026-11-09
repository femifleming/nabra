#include "PluginEditor.h"
#include "Art.h"
#include <map>

using namespace juce;
using namespace nabra;
namespace L = nabra::layout;

// =============================================================================================
// geometry
Rectangle<float> Tapestry::cellRect (int i)
{
    auto& c = cells()[(size_t) i];
    float x = c.col == 0 ? L::margin : L::margin + L::cellW + L::gap + (float) (c.col - 1) * L::cellW;
    float y = L::topH + L::padTop + (float) c.row * L::cellH;
    return { x, y, L::cellW, L::cellH };
}

Rectangle<float> Tapestry::boardRect (int i) { return cellRect (i).reduced (16.0f, 18.0f); }

Point<float> Tapestry::jackEdge (int cell, int jack)
{
    auto b = boardRect (cell);
    auto& j = spec (cells()[(size_t) cell].type).jacks[(size_t) jack];
    switch (j.side)
    {
        case Side::L: return { b.getX(), b.getY() + j.pos * b.getHeight() };
        case Side::R: return { b.getRight(), b.getY() + j.pos * b.getHeight() };
        case Side::T: return { b.getX() + j.pos * b.getWidth(), b.getY() };
        case Side::B: return { b.getX() + j.pos * b.getWidth(), b.getBottom() };
    }
    return {};
}

Point<float> Tapestry::jackTip (int cell, int jack)
{
    return art::loopTip (jackEdge (cell, jack), spec (cells()[(size_t) cell].type).jacks[(size_t) jack].side);
}

// the name tape (plus its LED) goes at the first height, nearest the middle, where it fits
// horizontally between that row's side labels
static Point<float> dymoOffset (int cell)
{
    static std::map<int, Point<float>> cache;
    if (auto it = cache.find (cell); it != cache.end()) return it->second;
    auto& sp = spec (cells()[(size_t) cell].type);
    auto b = Tapestry::boardRect (cell);
    const float need = art::dymoWidth (cellName (cell).toUpperCase()) + 16.0f;
    Point<float> result { b.getWidth() * 0.5f, b.getHeight() * 0.5f };
    for (float f : { 0.5f, 0.44f, 0.56f, 0.38f, 0.62f, 0.32f, 0.68f, 0.26f, 0.74f })
    {
        float y = f * b.getHeight(), lo = 3.0f, hi = b.getWidth() - 3.0f;
        for (auto& j : sp.jacks)
        {
            if (std::abs (j.pos * b.getHeight() - y) > 15.0f) continue;
            float w = art::tapeWidth (j.label, j.out) + 6.0f;
            if (j.side == Side::L) lo = juce::jmax (lo, 4.0f + w);
            if (j.side == Side::R) hi = juce::jmin (hi, b.getWidth() - 4.0f - w);
        }
        if (hi - lo >= need)
        {
            float cx = juce::jlimit (lo + need * 0.5f - 8.0f, hi - need * 0.5f - 8.0f, b.getWidth() * 0.5f);
            result = { cx, y };
            break;
        }
    }
    return cache[cell] = result;
}

Point<float> Tapestry::dymoPos (int cell)
{
    auto b = boardRect (cell);
    return b.getTopLeft() + dymoOffset (cell);
}

Point<float> Tapestry::knobPos (int cell)
{
    auto d = dymoPos (cell);
    auto b = boardRect (cell);
    return { b.getCentreX(), d.y < b.getCentreY() + 10 ? d.y + 27.0f : d.y - 27.0f };
}

// =============================================================================================
Tapestry::Tapestry (NabraProcessor& p, NabraEditor& e) : proc (p), editor (e)
{
    Random r (1999);
    for (auto& c : ledColour) c = r.nextInt (3);
    startTimerHz (30);
}

Tapestry::~Tapestry() = default;

RangedAudioParameter* Tapestry::knobParam (int cell) const
{
    int k = knobParamIndex (cell);
    return k < 0 ? nullptr : proc.apvts.getParameter ("vol" + String (k));
}

void Tapestry::timerCallback()
{
    if (rebuildCountdown > 0 && --rebuildCountdown == 0) cache = {};
    repaint();
}

void Tapestry::flash (const String& m)
{
    flashMsg = m;
    flashTime = Time::getMillisecondCounter();
}

// ---------------------------------------------------------------------------------------------
void Tapestry::renderStatic (Graphics& g)
{
    // the canvas itself
    Rectangle<float> canvas (0, L::topH, L::designW, L::designH - L::topH);
    art::paintCanvas (g, canvas, 2000);
    // grommets along the top hem, where it hangs from the wall
    for (float x = 40; x < L::designW - 20; x += 190)
    {
        auto c = Point<float> (x, L::topH + 7);
        g.setColour (Colour (0xff9c9a92)); g.fillEllipse (Rectangle<float> (11, 11).withCentre (c));
        g.setColour (Colour (0xff3a3936)); g.fillEllipse (Rectangle<float> (5, 5).withCentre (c));
    }
    // the sewn-on MIDI strip
    auto strip = Rectangle<float> (L::margin - 12, L::topH + 6, L::cellW + 24, L::designH - L::topH - 12);
    art::paintSewnPatch (g, strip);
    g.setColour (Colour (0xff3b3a36)); g.setFont (art::mono (9.5f, true));
    g.drawText ("MIDI > CV", strip.withHeight (12).translated (0, 2), Justification::centred);

    for (int i = 0; i < (int) cells().size(); ++i)
    {
        auto& c = cells()[(size_t) i];
        auto& sp = spec (c.type);
        auto b = boardRect (i);
        art::paintBoardShadow (g, b);
        const bool scan = proc.colourBoards && ! sp.midi;
        Image img = sp.midi ? art::midiBoard (c.type) : scan ? art::boardScan (c.type, c.num) : art::photo (sp.res);
        auto pr = scan ? b.expanded (b.getWidth() * 0.03f, b.getHeight() * 0.025f)
                       : b.expanded (b.getWidth() * 0.09f, b.getHeight() * 0.07f);
        // repeated module types are the same circuit, but not the same photograph: mirror some
        Graphics::ScopedSaveState ss (g);
        if (! sp.midi && (c.num % 2 == 0))
            g.addTransform (AffineTransform::scale (-1.0f, 1.0f, pr.getCentreX(), pr.getCentreY()));
        if (! sp.midi && (c.num % 3 == 0))
            g.addTransform (AffineTransform::scale (1.0f, -1.0f, pr.getCentreX(), pr.getCentreY()));
        g.setOpacity (1.0f);
        g.drawImage (img, pr, RectanglePlacement::fillDestination);
    }

    for (int i = 0; i < (int) cells().size(); ++i)
    {
        auto& sp = spec (cells()[(size_t) i].type);
        for (int j = 0; j < (int) sp.jacks.size(); ++j)
            art::drawLoop (g, jackEdge (i, j), sp.jacks[(size_t) j].side, false);
    }

    if (proc.showLabels)
    {
        Random rr (42);
        for (int i = 0; i < (int) cells().size(); ++i)
        {
            auto& sp = spec (cells()[(size_t) i].type);
            auto b = boardRect (i);
            for (int j = 0; j < (int) sp.jacks.size(); ++j)
            {
                auto& jk = sp.jacks[(size_t) j];
                auto e = jackEdge (i, j);
                float w = art::tapeWidth (jk.label, jk.out), h = 13;
                Rectangle<float> r;
                switch (jk.side)
                {
                    case Side::L: r = { b.getX() + 4, e.y - h * 0.5f, w, h }; break;
                    case Side::R: r = { b.getRight() - 4 - w, e.y - h * 0.5f, w, h }; break;
                    case Side::T: r = { e.x - w * 0.5f, b.getY() + 4, w, h }; break;
                    case Side::B: r = { e.x - w * 0.5f, b.getBottom() - 4 - h, w, h }; break;
                }
                art::drawTape (g, r, jk.label, jk.out, jk.inv, (rr.nextFloat() - 0.5f) * 0.07f);
            }
        }
    }
    for (int i = 0; i < (int) cells().size(); ++i)
        art::drawDymo (g, dymoPos (i), cellName (i).toUpperCase());
}

// ---------------------------------------------------------------------------------------------
void Tapestry::paint (Graphics& g)
{
    const float scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (! cache.isValid() || cacheLabels != proc.showLabels || cacheBoards != proc.colourBoards)
    {
        const float s = juce::jlimit (0.3f, 3.0f, scale);
        cache = Image (Image::RGB, (int) std::ceil (L::designW * s), (int) std::ceil (L::designH * s), false);
        Graphics cg (cache);
        cg.addTransform (AffineTransform::scale (s));
        renderStatic (cg);
        cacheScale = s;
        cacheLabels = proc.showLabels;
        cacheBoards = proc.colourBoards;
    }
    else if (std::abs (scale - cacheScale) > 0.01f && rebuildCountdown == 0)
        rebuildCountdown = 6;   // re-render crisp once resizing settles

    g.setOpacity (1.0f);
    g.drawImage (cache, Rectangle<float> (0, 0, L::designW, L::designH));

    auto& eng = proc.getEngine();
    // LEDs and pots
    for (int i = 0; i < (int) cells().size(); ++i)
    {
        auto dp = dymoPos (i);
        float dw = art::dymoWidth (cellName (i).toUpperCase());
        art::drawLed (g, { dp.x + dw * 0.5f + 8, dp.y }, ledColour[(size_t) i], eng.led[(size_t) i].load());
        if (auto* p = knobParam (i))
            art::drawKnob (g, knobPos (i), 9.0f, p->getValue(), hoverKnob == i);
    }

    // hover loop glow
    if (hoverJack.valid() && ! drag.active)
        art::drawLoop (g, jackEdge (hoverJack.cell, hoverJack.jack), spec (cells()[(size_t) hoverJack.cell].type).jacks[(size_t) hoverJack.jack].side, true);

    // cords
    const auto& cables = proc.getCables();
    for (auto& c : cables)
        if (c.id != hoverCable)
            art::drawCable (g, jackTip (c.src.cell, c.src.jack), jackTip (c.dst.cell, c.dst.jack), c.colour, c.cord, c.resVal, false);
    for (auto& c : cables)
        if (c.id == hoverCable)
            art::drawCable (g, jackTip (c.src.cell, c.src.jack), jackTip (c.dst.cell, c.dst.jack), c.colour, c.cord, c.resVal, true);

    if (drag.active)
    {
        auto a = jackTip (drag.fixed.cell, drag.fixed.jack);
        auto b = drag.pos;
        if (hoverJack.valid())
        {
            bool tOut = spec (cells()[(size_t) hoverJack.cell].type).jacks[(size_t) hoverJack.jack].out;
            art::drawLoop (g, jackEdge (hoverJack.cell, hoverJack.jack), spec (cells()[(size_t) hoverJack.cell].type).jacks[(size_t) hoverJack.jack].side, tOut != drag.fixedIsOut);
            if (tOut != drag.fixedIsOut) b = jackTip (hoverJack.cell, hoverJack.jack);
        }
        if (drag.fixedIsOut) art::drawCable (g, a, b, drag.colour, drag.cord, drag.resVal, false, 0.9f);
        else art::drawCable (g, b, a, drag.colour, drag.cord, drag.resVal, false, 0.9f);
    }

    paintCard (g);

    // flash message on a slip of paper
    if (flashMsg.isNotEmpty())
    {
        auto age = Time::getMillisecondCounter() - flashTime;
        if (age > 4000) flashMsg.clear();
        else
        {
            float a = age < 3200 ? 1.0f : 1.0f - (float) (age - 3200) / 800.0f;
            auto f = art::serif (17, false, true);
            float w = GlyphArrangement::getStringWidth (f, flashMsg) + 40;
            Rectangle<float> r (w, 38);
            r.setCentre (L::designW * 0.5f, L::designH - 40);
            g.setColour (Colour (0x55000000).withMultipliedAlpha (a)); g.fillRect (r.translated (3, 4));
            g.setColour (Colour (0xfffbf7e8).withMultipliedAlpha (a)); g.fillRect (r);
            g.setColour (Colour (0xff8a2a1a).withMultipliedAlpha (a)); g.drawRect (r, 1.5f);
            g.setColour (Colour (0xff1b1a18).withMultipliedAlpha (a)); g.setFont (f);
            g.drawText (flashMsg, r, Justification::centred);
        }
    }
}

void Tapestry::paintCard (Graphics& g)
{
    auto now = Time::getMillisecondCounter();
    if (! drag.active && hoverDymo >= 0 && now - hoverDymoTime > 250)
    {
        paintDiagramCard (g, hoverDymo);
        return;
    }
    if (drag.active || ! hoverJack.valid() || now - hoverJackTime < 300) return;

    auto& sp = spec (cells()[(size_t) hoverJack.cell].type);
    auto& j = sp.jacks[(size_t) hoverJack.jack];
    const float W = 300;
    TextLayout tl;
    {
        AttributedString as;
        as.setWordWrap (AttributedString::byWord);
        as.append (j.help, art::serif (14), Colour (0xff1b1a18));
        tl.createLayout (as, W - 24);
    }
    float H = 36 + tl.getHeight() + 10 + 92 + 12;
    auto tip = jackTip (hoverJack.cell, hoverJack.jack);
    Rectangle<float> r (W, H);
    r.setPosition (tip.x + 18, tip.y + 14);
    if (r.getRight() > L::designW - 8) r.setX (tip.x - W - 18);
    if (r.getBottom() > L::designH - 8) r.setY (tip.y - H - 14);
    r.setY (juce::jmax (L::topH + 4, r.getY()));

    g.setColour (Colour (0x50000000)); g.fillRect (r.translated (3, 5));
    g.setColour (Colour (0xfff9f6ec)); g.fillRect (r);
    g.setColour (Colour (0xffc9c1ab)); g.drawRect (r, 1.0f);
    auto in = r.reduced (12, 10);
    auto head = in.removeFromTop (20);
    art::drawTape (g, head.withWidth (art::tapeWidth (j.label, j.out)).withHeight (14).translated (0, 3), j.label, j.out, j.inv);
    g.setColour (Colour (0xff6d6a62)); g.setFont (art::serif (14, false, true));
    g.drawText (cellName (hoverJack.cell) + (j.out ? "  - output" : "  - input"), head.withTrimmedLeft (art::tapeWidth (j.label, j.out) + 8),
                Justification::centredLeft);
    in.removeFromTop (6);
    tl.draw (g, in.removeFromTop (tl.getHeight()));
    in.removeFromTop (10);

    // the scope: ~2.7 s of min/max history
    auto sc = in.removeFromTop (92);
    g.setColour (Colour (0xff141414)); g.fillRoundedRectangle (sc, 3);
    auto& eng = proc.getEngine();
    const int N = Engine::probeLen;
    int w0 = eng.probeWrite.load (std::memory_order_acquire);
    float lo = 1e9f, hi = -1e9f;
    std::vector<float> mn ((size_t) N), mx ((size_t) N);
    for (int i = 0; i < N; ++i)
    {
        int k = (w0 + i) % N;
        mn[(size_t) i] = eng.probeMin[(size_t) k].load (std::memory_order_relaxed);
        mx[(size_t) i] = eng.probeMax[(size_t) k].load (std::memory_order_relaxed);
        lo = juce::jmin (lo, mn[(size_t) i]); hi = juce::jmax (hi, mx[(size_t) i]);
    }
    lo = juce::jmin (lo, 0.0f); hi = juce::jmax (hi, 0.0f);
    if (hi - lo < 1.0f) hi = lo + 1.0f;
    auto plot = sc.reduced (6, 8);
    auto yOf = [&] (float v) { return plot.getBottom() - (v - lo) / (hi - lo) * plot.getHeight(); };
    g.setColour (Colour (0xff3a3a3a));
    g.drawHorizontalLine ((int) yOf (0), plot.getX(), plot.getRight());
    Path band;
    for (int i = 0; i < N; ++i)
    {
        float x = plot.getX() + plot.getWidth() * (float) i / (float) (N - 1);
        float y1 = yOf (mx[(size_t) i]), y2 = yOf (mn[(size_t) i]);
        if (y2 - y1 < 1.2f) { float m = (y1 + y2) * 0.5f; y1 = m - 0.6f; y2 = m + 0.6f; }
        band.addRectangle (x - 0.4f, y1, plot.getWidth() / (float) N + 0.8f, y2 - y1);
    }
    g.setColour (Colour (0xff9dff7a));
    g.fillPath (band);
    g.setColour (Colour (0xff7d7d7d)); g.setFont (art::mono (10));
    g.drawText (String (hi, 1) + "V", sc.reduced (6, 2), Justification::topRight);
    g.drawText (String (lo, 1) + "V", sc.reduced (6, 2), Justification::bottomRight);
    g.drawText (String (mx[(size_t) N - 1], 2) + "V now", sc.reduced (6, 2), Justification::topLeft);
}

void Tapestry::paintDiagramCard (Graphics& g, int cell)
{
    auto& sp = spec (cells()[(size_t) cell].type);
    auto b = boardRect (cell);
    const float W = 300, H = sp.midi ? 150.0f : 330.0f;
    Rectangle<float> r (W, H);
    r.setPosition (b.getRight() + 24, b.getY() - 10);
    if (r.getRight() > L::designW - 8) r.setX (b.getX() - W - 24);
    r.setY (juce::jlimit (L::topH + 4, L::designH - H - 8, r.getY()));
    g.setColour (Colour (0x50000000)); g.fillRect (r.translated (3, 5));
    g.setColour (Colours::white); g.fillRect (r);
    g.setColour (Colour (0xffc9c1ab)); g.drawRect (r, 1.0f);
    auto in = r.reduced (12, 10);
    g.setColour (Colour (0xff1b1a18)); g.setFont (art::serif (19, true));
    g.drawText (cellName (cell), in.removeFromTop (22), Justification::centredLeft);
    auto foot = in.removeFromBottom (16);
    g.setColour (Colour (0xff6d6a62)); g.setFont (art::serif (12.5f, false, true));
    g.drawText ("right-click the board for its help page", foot, Justification::centredLeft);
    if (sp.midi)
    {
        g.setColour (Colour (0xff1b1a18)); g.setFont (art::serif (14));
        g.drawFittedText (sp.summary, in.toNearestInt(), Justification::topLeft, 6);
    }
    else
    {
        auto img = art::diagram (sp.res);
        g.setOpacity (1.0f);
        g.drawImage (img, in.reduced (4), RectanglePlacement::centred);
    }
}

// ---------------------------------------------------------------------------------------------
// hit testing
JackRef Tapestry::jackAt (Point<float> p) const
{
    JackRef best;
    float bd = 12.5f;
    for (int i = 0; i < (int) cells().size(); ++i)
    {
        if (! cellRect (i).expanded (14).contains (p)) continue;
        auto& sp = spec (cells()[(size_t) i].type);
        for (int j = 0; j < (int) sp.jacks.size(); ++j)
        {
            auto e = jackEdge (i, j), t = jackTip (i, j);
            float d = juce::jmin (p.getDistanceFrom (t), p.getDistanceFrom ((e + t) * 0.5f) + 1.0f);
            if (d < bd) { bd = d; best = { i, j }; }
        }
    }
    return best;
}

int Tapestry::cableAt (Point<float> p) const
{
    int best = -1;
    float bd = 6.0f;
    for (auto& c : proc.getCables())
    {
        float d = art::distanceToCable (jackTip (c.src.cell, c.src.jack), jackTip (c.dst.cell, c.dst.jack), p);
        if (d < bd) { bd = d; best = c.id; }
    }
    return best;
}

int Tapestry::knobAt (Point<float> p) const
{
    for (int i = 0; i < (int) cells().size(); ++i)
        if (spec (cells()[(size_t) i].type).hasKnob && p.getDistanceFrom (knobPos (i)) < 13.0f) return i;
    return -1;
}

int Tapestry::dymoAt (Point<float> p) const
{
    for (int i = 0; i < (int) cells().size(); ++i)
    {
        Rectangle<float> r (art::dymoWidth (cellName (i).toUpperCase()), 14.0f);
        r.setCentre (dymoPos (i));
        if (r.expanded (2).contains (p)) return i;
    }
    return -1;
}

int Tapestry::boardAt (Point<float> p) const
{
    for (int i = 0; i < (int) cells().size(); ++i)
        if (boardRect (i).contains (p)) return i;
    return -1;
}

void Tapestry::setHoverJack (JackRef j)
{
    if (j == hoverJack) return;
    hoverJack = j;
    hoverJackTime = Time::getMillisecondCounter();
    auto& eng = proc.getEngine();
    if (j.valid())
    {
        bool out = spec (cells()[(size_t) j.cell].type).jacks[(size_t) j.jack].out;
        eng.probeSlot.store (out ? eng.outSlot (j.cell, j.jack) : eng.inSlot (j.cell, j.jack));
        eng.probeKind.store (out ? 1 : 2);
    }
    else eng.probeKind.store (0);
}

// ---------------------------------------------------------------------------------------------
// mouse
void Tapestry::mouseMove (const MouseEvent& e)
{
    auto p = e.position;
    setHoverJack (jackAt (p));
    int hk = hoverJack.valid() ? -1 : knobAt (p);
    int hc = (hoverJack.valid() || hk >= 0) ? -1 : cableAt (p);
    int hd = (hoverJack.valid() || hk >= 0 || hc >= 0) ? -1 : dymoAt (p);
    if (hd != hoverDymo) { hoverDymo = hd; hoverDymoTime = Time::getMillisecondCounter(); }
    hoverKnob = hk; hoverCable = hc;
    setMouseCursor (hoverJack.valid() ? MouseCursor::CrosshairCursor
                    : hk >= 0 ? MouseCursor::UpDownResizeCursor
                    : hc >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
}

void Tapestry::mouseExit (const MouseEvent&)
{
    if (! drag.active) setHoverJack ({});
    hoverCable = hoverKnob = hoverDymo = -1;
}

void Tapestry::mouseDown (const MouseEvent& e)
{
    auto p = e.position;
    auto j = jackAt (p);
    if (e.mods.isPopupMenu())
    {
        if (j.valid()) { jackMenu (j); return; }
        if (auto c = cableAt (p); c >= 0) { cableMenu (c); return; }
        if (auto b = boardAt (p); b >= 0)
        {
            PopupMenu m;
            m.addSectionHeader (cellName (b));
            m.addItem (1, String ("Help: ") + spec (cells()[(size_t) b].type).name);
            m.addItem (2, "Unpatch this board");
            m.showMenuAsync (PopupMenu::Options(), [this, b] (int r)
            {
                if (r == 1) editor.openHelpFor (cells()[(size_t) b].type);
                if (r == 2)
                {
                    editor.pushUndo();
                    std::vector<int> ids;
                    for (auto& c : proc.getCables()) if (c.src.cell == b || c.dst.cell == b) ids.push_back (c.id);
                    for (int id : ids) proc.removeCable (id);
                }
            });
        }
        return;
    }

    if (j.valid())
    {
        bool out = spec (cells()[(size_t) j.cell].type).jacks[(size_t) j.jack].out;
        drag = {};
        drag.active = true;
        drag.pos = p;
        drag.cord = handCord; drag.resVal = handRes;
        drag.colour = (int) proc.getCables().size() + Random::getSystemRandom().nextInt (6);
        if (! out && ! e.mods.isShiftDown())
        {
            // pick up the most recent cord on this input
            const Cable* last = nullptr;
            for (auto& c : proc.getCables()) if (c.dst == j) last = &c;
            if (last != nullptr)
            {
                editor.pushUndo();
                drag.fixed = last->src; drag.fixedIsOut = true;
                drag.cord = last->cord; drag.resVal = last->resVal; drag.colour = last->colour;
                drag.pickedUp = true; drag.pickedFrom = j;
                proc.removeCable (last->id);
                return;
            }
        }
        drag.fixed = j; drag.fixedIsOut = out;
        return;
    }
    if (auto k = knobAt (p); k >= 0)
    {
        if (auto* prm = knobParam (k))
        {
            knobDrag = { k, prm->getValue(), p.y };
            prm->beginChangeGesture();
        }
    }
}

void Tapestry::mouseDrag (const MouseEvent& e)
{
    if (drag.active)
    {
        drag.pos = e.position;
        auto j = jackAt (e.position);
        if (j != hoverJack) { hoverJack = j; hoverJackTime = Time::getMillisecondCounter(); }
        return;
    }
    if (knobDrag.cell >= 0)
        if (auto* prm = knobParam (knobDrag.cell))
            prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, knobDrag.startVal + (knobDrag.startY - e.position.y) / 120.0f));
}

void Tapestry::mouseUp (const MouseEvent& e)
{
    if (knobDrag.cell >= 0)
    {
        if (auto* prm = knobParam (knobDrag.cell)) prm->endChangeGesture();
        knobDrag = {};
        return;
    }
    if (! drag.active) return;
    drag.active = false;
    auto t = jackAt (e.position);
    if (! t.valid())
    {
        if (drag.pickedUp) flash ("cord removed");
        repaint();
        return;
    }
    bool tOut = spec (cells()[(size_t) t.cell].type).jacks[(size_t) t.jack].out;
    if (t == drag.fixed) { repaint(); return; }
    if (tOut && drag.fixedIsOut)
    {
        flash ("Hot outputs! Never cross two OUTPUTS - patch into a lowercase input.");
        return;
    }
    if (! tOut && ! drag.fixedIsOut)
    {
        flash ("Both ends are inputs. Start or end on an OUTPUT (UPPERCASE).");
        return;
    }
    JackRef src = drag.fixedIsOut ? drag.fixed : t, dst = drag.fixedIsOut ? t : drag.fixed;
    if (! drag.pickedUp) editor.pushUndo();
    int id = proc.addCable (src, dst, drag.cord, drag.resVal);
    for (auto c : proc.getCables())
        if (c.id == id && c.colour != drag.colour) { c.colour = drag.colour; proc.updateCable (c); }
    repaint();
}

void Tapestry::mouseDoubleClick (const MouseEvent& e)
{
    if (auto k = knobAt (e.position); k >= 0)
    {
        if (auto* prm = knobParam (k)) prm->setValueNotifyingHost (prm->getDefaultValue());
        return;
    }
    if (auto c = cableAt (e.position); c >= 0)
    {
        editor.pushUndo();
        proc.removeCable (c);
        hoverCable = -1;
    }
}

void Tapestry::cableMenu (int id)
{
    const Cable* cp = nullptr;
    for (auto& c : proc.getCables()) if (c.id == id) cp = &c;
    if (cp == nullptr) return;
    Cable c = *cp;
    auto& ss = spec (cells()[(size_t) c.src.cell].type);
    auto& ds = spec (cells()[(size_t) c.dst.cell].type);
    PopupMenu m;
    m.addSectionHeader (cellName (c.dst.cell) + "." + ds.jacks[(size_t) c.dst.jack].label + " ( " + cellName (c.src.cell) + "." + ss.jacks[(size_t) c.src.jack].label + " )");
    m.addItem (1, "wire", true, c.cord == Cord::Wire);
    m.addItem (2, "resistor 1k  (~90%)", true, c.cord == Cord::Resistor && c.resVal == 0);
    m.addItem (3, "resistor 10k  (half)", true, c.cord == Cord::Resistor && c.resVal == 1);
    m.addItem (4, "resistor 100k  (a tenth)", true, c.cord == Cord::Resistor && c.resVal == 2);
    m.addItem (5, "capacitor  (passes changes only)", true, c.cord == Cord::Capacitor);
    m.addItem (6, "diode  (positive only)", true, c.cord == Cord::Diode);
    m.addSeparator();
    m.addItem (7, "change colour");
    m.addItem (9, "remove cord");
    m.showMenuAsync (PopupMenu::Options(), [this, c] (int r) mutable
    {
        if (r == 0) return;
        editor.pushUndo();
        if (r == 9) { proc.removeCable (c.id); return; }
        if (r == 1) c.cord = Cord::Wire;
        if (r >= 2 && r <= 4) { c.cord = Cord::Resistor; c.resVal = r - 2; }
        if (r == 5) c.cord = Cord::Capacitor;
        if (r == 6) c.cord = Cord::Diode;
        if (r == 7) c.colour = (c.colour + 1) % 6;
        proc.updateCable (c);
        repaint();
    });
}

void Tapestry::jackMenu (JackRef j)
{
    auto& sp = spec (cells()[(size_t) j.cell].type);
    PopupMenu m;
    m.addSectionHeader (cellName (j.cell) + "  " + sp.jacks[(size_t) j.jack].label);
    std::vector<int> ids;
    for (auto& c : proc.getCables())
        if (c.src == j || c.dst == j)
        {
            auto other = c.src == j ? c.dst : c.src;
            ids.push_back (c.id);
            m.addItem ((int) ids.size(), "remove cord to " + cellName (other.cell) + " " + spec (cells()[(size_t) other.cell].type).jacks[(size_t) other.jack].label);
        }
    if (! ids.empty()) { m.addSeparator(); m.addItem (1000, "remove all cords here"); }
    m.addItem (2000, String ("Help: ") + sp.name);
    m.showMenuAsync (PopupMenu::Options(), [this, ids, j] (int r)
    {
        if (r == 2000) { editor.openHelpFor (cells()[(size_t) j.cell].type); return; }
        if (r <= 0) return;
        editor.pushUndo();
        if (r == 1000) { for (int id : ids) proc.removeCable (id); }
        else if (r <= (int) ids.size()) proc.removeCable (ids[(size_t) r - 1]);
    });
}

// =============================================================================================
// top bar
namespace { const char* cordLabels[] = { "wire", "resistor", "capacitor", "diode" }; }

TopBar::TopBar (NabraProcessor& p, NabraEditor& e) : proc (p), editor (e)
{
    for (int i = 0; i < 4; ++i) cordRects.push_back ({ 452.0f + i * 104.0f, 8.0f, 96.0f, 50.0f });
    float x = 900;
    for (auto* n : { "RECIPES", "UNDO", "UNPATCH ALL", "LABELS", "COLOUR", "? HELP" })
    {
        float w = art::dymoWidth (n) + 26;
        buttons.push_back ({ n, { x, 19.0f, w, 26.0f } });
        x += w + 14;
    }
    masterPos = { L::designW - 64.0f, 32.0f };
    startTimerHz (15);
}

TopBar::~TopBar() = default;

int TopBar::itemAt (Point<float> p) const
{
    for (int i = 0; i < 4; ++i) if (cordRects[(size_t) i].contains (p)) return i;
    for (size_t i = 0; i < buttons.size(); ++i) if (buttons[i].second.contains (p)) return 10 + (int) i;
    if (p.getDistanceFrom (masterPos) < 16) return 100;
    return -1;
}

void TopBar::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (Colour (0xfff6f3ea), 0, 0, Colour (0xffe6e1d3), 0, r.getHeight(), false));
    g.fillRect (r);
    g.setColour (Colour (0xffbfb7a2));
    g.drawHorizontalLine ((int) r.getBottom() - 1, 0, r.getRight());

    g.setColour (Colour (0xff1b1a18)); g.setFont (art::serif (36, true));
    g.drawText ("Nabra", Rectangle<float> (24, 4, 130, 40), Justification::centredLeft);
    g.setColour (Colour (0xff6d6a62)); g.setFont (art::serif (13, false, true));
    g.drawText ("an analog brain - the knobless tapestry", Rectangle<float> (26, 40, 300, 16), Justification::centredLeft);
    g.drawText ("after Peter Blasser, 2000 / \"Stores at the Mall\", 2015", Rectangle<float> (160, 14, 280, 16), Justification::centredLeft);

    g.setColour (Colour (0xff6d6a62)); g.setFont (art::font (9, true));
    g.drawText ("CORD IN HAND", Rectangle<float> (452, 0, 200, 10), Justification::centredLeft);
    for (int i = 0; i < 4; ++i)
    {
        auto cr = cordRects[(size_t) i];
        bool sel = (int) editor.tapestry.handCord == i;
        if (sel)
        {
            g.setColour (Colour (0xff1d1d1f)); g.fillRoundedRectangle (cr, 4);
            g.setColour (Colour (0xfff6f3ea)); g.fillRoundedRectangle (cr.reduced (2), 3);
        }
        else if (hover == i) { g.setColour (Colour (0x14000000)); g.fillRoundedRectangle (cr, 4); }
        art::drawCordIcon (g, cr.withTrimmedBottom (14).reduced (4, 2), (Cord) i, editor.tapestry.handRes);
        g.setColour (Colour (0xff1b1a18)); g.setFont (art::serif (12.5f, sel));
        String t = cordLabels[i];
        if (i == 1) t << " " << resistorName (editor.tapestry.handRes);
        g.drawText (t, cr.removeFromBottom (15), Justification::centred);
    }

    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto& [n, br] = buttons[i];
        bool on = (n == String ("LABELS") && proc.showLabels) || (n == String ("COLOUR") && proc.colourBoards);
        g.setColour (Colour (0x44000000)); g.fillRoundedRectangle (br.translated (1, 2), 3);
        g.setColour (hover == 10 + (int) i ? Colour (0xff38383b) : Colour (0xff18181a));
        g.fillRoundedRectangle (br, 3);
        g.setColour (Colour (0xffeeeeea));
        g.setFont (Font (FontOptions ("Arial", 10.5f, Font::bold).withKerningFactor (0.14f)));
        g.drawText (n, br, Justification::centred);
        if (n == String ("LABELS") || n == String ("COLOUR"))
        {
            g.setColour (on ? Colour (0xff6cff3a) : Colour (0xff2c4a22));
            g.fillEllipse (br.getRight() - 12, br.getCentreY() - 2.5f, 5, 5);
        }
    }

    // master volume + the out LED
    float v = proc.apvts.getParameter ("master")->getValue();
    art::drawKnob (g, masterPos, 12, v, hover == 100);
    g.setColour (Colour (0xff6d6a62)); g.setFont (art::font (9, true));
    g.drawText ("VOLUME", Rectangle<float> (masterPos.x - 80, masterPos.y - 6, 56, 12), Justification::centredRight);
    art::drawLed (g, { masterPos.x + 34, masterPos.y }, 0, juce::jlimit (0.0f, 1.0f, proc.getEngine().outLevel.load() * 1.6f));
}

void TopBar::mouseMove (const MouseEvent& e)
{
    int h = itemAt (e.position);
    if (h != hover) { hover = h; repaint(); }
    setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
}
void TopBar::mouseExit (const MouseEvent&) { hover = -1; repaint(); }

void TopBar::mouseDown (const MouseEvent& e)
{
    int it = itemAt (e.position);
    if (it >= 0 && it < 4)
    {
        auto& t = editor.tapestry;
        if (it == 1 && t.handCord == Cord::Resistor) editor.setHand (Cord::Resistor, (t.handRes + 1) % 3);
        else editor.setHand ((Cord) it, t.handRes);
    }
    else if (it >= 10 && it < 100)
    {
        String n = buttons[(size_t) it - 10].first;
        if (n == "RECIPES") editor.showRecipeMenu (buttons[(size_t) it - 10].second);
        else if (n == "UNDO") editor.undo();
        else if (n == "UNPATCH ALL") editor.unpatchAll();
        else if (n == "LABELS") editor.toggleLabels();
        else if (n == "COLOUR") editor.toggleBoards();
        else if (n == "? HELP") editor.toggleHelp();
    }
    else if (it == 100)
    {
        draggingMaster = true;
        dragStart = e.position.y;
        auto* p = proc.apvts.getParameter ("master");
        dragVal = p->getValue();
        p->beginChangeGesture();
    }
    repaint();
}

void TopBar::mouseDrag (const MouseEvent& e)
{
    if (draggingMaster)
        proc.apvts.getParameter ("master")->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, dragVal + (dragStart - e.position.y) / 120.0f));
}

void TopBar::mouseUp (const MouseEvent&)
{
    if (draggingMaster) { proc.apvts.getParameter ("master")->endChangeGesture(); draggingMaster = false; }
}

void TopBar::mouseDoubleClick (const MouseEvent& e)
{
    if (itemAt (e.position) == 100)
    {
        auto* p = proc.apvts.getParameter ("master");
        p->setValueNotifyingHost (p->getDefaultValue());
    }
}

// =============================================================================================
NabraEditor::NabraEditor (NabraProcessor& p)
    : AudioProcessorEditor (p), tapestry (p, *this), topBar (p, *this), proc (p)
{
    root.setBounds (0, 0, (int) L::designW, (int) L::designH);
    tapestry.setBounds (root.getLocalBounds());
    topBar.setBounds (0, 0, (int) L::designW, (int) L::topH);
    help.setBounds (root.getLocalBounds());
    help.setVisible (false);
    help.onClose = [this] { toggleHelp(); };
    help.onLoadRecipe = [this] (int i)
    {
        pushUndo();
        proc.loadRecipe (i);
        toggleHelp();
        tapestry.flash (String ("loaded: ") + recipes()[(size_t) i].name);
    };
    root.addAndMakeVisible (tapestry);
    root.addAndMakeVisible (topBar);
    root.addChildComponent (help);
    addAndMakeVisible (root);
    proc.addChangeListener (this);

    setResizable (true, true);
    const double ratio = L::designW / L::designH;
    getConstrainer()->setFixedAspectRatio (ratio);
    setResizeLimits ((int) (L::designW * 0.42f), (int) (L::designH * 0.42f), (int) (L::designW * 1.4f), (int) (L::designH * 1.4f));
    float s = juce::jlimit (0.42f, 1.4f, proc.uiScale);
    setSize ((int) std::round (L::designW * s), (int) std::round (L::designH * s));
    setWantsKeyboardFocus (true);
}

NabraEditor::~NabraEditor() { proc.removeChangeListener (this); }

void NabraEditor::resized()
{
    float s = (float) getWidth() / L::designW;
    root.setTransform (AffineTransform::scale (s));
    proc.uiScale = s;
}

void NabraEditor::changeListenerCallback (ChangeBroadcaster*) { tapestry.repaint(); }

bool NabraEditor::keyPressed (const KeyPress& k)
{
    if (k == KeyPress ('z', ModifierKeys::commandModifier, 0)) { undo(); return true; }
    if (k.getTextCharacter() == '?') { toggleHelp(); return true; }
    if (k == KeyPress::escapeKey && help.isVisible()) { toggleHelp(); return true; }
    return false;
}

void NabraEditor::pushUndo()
{
    undoStack.push_back (proc.getCables());
    if (undoStack.size() > 64) undoStack.erase (undoStack.begin());
}

void NabraEditor::undo()
{
    if (undoStack.empty()) { tapestry.flash ("nothing to undo"); return; }
    proc.setCables (undoStack.back());
    undoStack.pop_back();
    tapestry.repaint();
}

void NabraEditor::toggleHelp()
{
    help.setVisible (! help.isVisible());
    if (help.isVisible()) { help.toFront (true); help.grabKeyboardFocus(); }
    else grabKeyboardFocus();
}

void NabraEditor::openHelpFor (MT t)
{
    help.showModule (t);
    if (! help.isVisible()) toggleHelp();
}

void NabraEditor::showRecipeMenu (Rectangle<float>)
{
    PopupMenu m;
    m.addSectionHeader ("from the thesis");
    for (int i = 0; i < (int) recipes().size(); ++i)
    {
        if (i == 3) m.addSectionHeader ("with the MIDI strip");
        m.addItem (i + 1, recipes()[(size_t) i].name);
    }
    m.addSeparator();
    m.addItem (100, "Empty tapestry");
    m.addItem (101, "Read about the recipes...");
    m.showMenuAsync (PopupMenu::Options(), [this] (int r)
    {
        if (r <= 0) return;
        if (r == 101) { toggleHelp(); help.showPage (4); return; }
        pushUndo();
        if (r == 100) { proc.loadRecipe (-1); return; }
        proc.loadRecipe (r - 1);
        tapestry.flash (String ("loaded: ") + recipes()[(size_t) r - 1].name);
    });
}

void NabraEditor::unpatchAll()
{
    pushUndo();
    proc.clearCables();
    tapestry.flash ("unpatched - the tapestry is silent. (cmd-Z to undo)");
}

void NabraEditor::setHand (Cord c, int r)
{
    tapestry.handCord = c;
    tapestry.handRes = r;
    topBar.repaint();
    String m = String ("cord in hand: ") + cordName (c);
    if (c == Cord::Resistor) m << " " << resistorName (r) << "  (click again for another value)";
    tapestry.flash (m);
}

void NabraEditor::toggleLabels()
{
    proc.showLabels = ! proc.showLabels;
    tapestry.invalidateCache();
    topBar.repaint();
    if (! proc.showLabels) tapestry.flash ("\"no connections are labeled and I traverse it with my personal knowledge\"");
}

void NabraEditor::toggleBoards()
{
    proc.colourBoards = ! proc.colourBoards;
    tapestry.invalidateCache();
    topBar.repaint();
    tapestry.flash (proc.colourBoards ? "colour boards: scans of the real circuits" : "thesis boards: the photographs from \"Stores at the Mall\"");
}
