#include "Art.h"
#include "NabraArt.h"
#include <map>
#include <cmath>

using namespace juce;

namespace art
{
namespace
{
struct R
{
    uint32_t s;
    explicit R (uint32_t seed) : s (seed * 747796405u + 2891336453u) {}
    float u() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 16777216.0f; }
    float r (float a, float b) { return a + (b - a) * u(); }
};

Image loadRes (const String& name)
{
    int size = 0;
    if (auto* d = NabraArt::getNamedResource (name.toRawUTF8(), size))
        return ImageFileFormat::loadFrom (d, (size_t) size).convertedToFormat (Image::ARGB);  // grayscale JPEGs load single-channel
    return {};
}

Path blob (Point<float> c, float rad, R& rng, int pts = 9, float irregular = 0.35f)
{
    Path p;
    std::vector<Point<float>> v;
    float rot = rng.r (0, 6.283f);
    float sx = rng.r (0.75f, 1.3f), sy = rng.r (0.75f, 1.3f);
    for (int i = 0; i < pts; ++i)
    {
        float a = rot + 6.2832f * (float) i / (float) pts;
        float rr = rad * (1.0f + rng.r (-irregular, irregular));
        v.push_back (c + Point<float> (std::cos (a) * rr * sx, std::sin (a) * rr * sy));
    }
    // smooth closed curve through midpoints
    auto mid = [] (Point<float> a, Point<float> b) { return (a + b) * 0.5f; };
    p.startNewSubPath (mid (v.back(), v[0]));
    for (size_t i = 0; i < v.size(); ++i)
        p.quadraticTo (v[i], mid (v[i], v[(i + 1) % v.size()]));
    p.closeSubPath();
    return p;
}

Image weaveTile()
{
    static Image t;
    if (t.isValid()) return t;
    t = Image (Image::ARGB, 96, 96, true);
    R rng (77);
    for (int y = 0; y < 96; ++y)
        for (int x = 0; x < 96; ++x)
        {
            float w = ((x % 3 == 0) ? 0.06f : 0.0f) + ((y % 3 == 0) ? 0.05f : 0.0f) + rng.r (0.0f, 0.08f);
            t.setPixelAt (x, y, Colour::fromFloatRGBA (0.25f, 0.24f, 0.22f, w));
        }
    return t;
}
} // namespace

Font font (float h, bool bold, bool italic)
{
    return Font (FontOptions ("Arial", h, (bold ? Font::bold : 0) | (italic ? Font::italic : 0)));
}
Font serif (float h, bool bold, bool italic)
{
    return Font (FontOptions ("Times New Roman", h, (bold ? Font::bold : 0) | (italic ? Font::italic : 0)));
}
Font mono (float h, bool bold)
{
    return Font (FontOptions ("Courier New", h, bold ? Font::bold : 0));
}

Image rawPhoto (const String& res)
{
    static std::map<String, Image> cache;
    auto& i = cache[res];
    if (! i.isValid()) i = loadRes (res + "_jpg");
    return i;
}

Image photo (const String& res)
{
    static std::map<String, Image> cache;
    auto& out = cache[res];
    if (out.isValid()) return out;
    Image src = rawPhoto (res);
    if (! src.isValid()) return {};
    out = Image (Image::ARGB, src.getWidth(), src.getHeight(), true);
    const int w = src.getWidth(), h = src.getHeight();
    Image::BitmapData sd (src, Image::BitmapData::readOnly), od (out, Image::BitmapData::writeOnly);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            float fx = juce::jmin ((float) x, (float) (w - 1 - x)) / (w * 0.07f);
            float fy = juce::jmin ((float) y, (float) (h - 1 - y)) / (h * 0.06f);
            float a = juce::jlimit (0.0f, 1.0f, juce::jmin (fx, fy));
            a = a * a * (3.0f - 2.0f * a);
            auto c = sd.getPixelColour (x, y);
            float b = c.getBrightness();
            b = juce::jlimit (0.0f, 1.0f, (b - 0.5f) * 1.12f + 0.53f);  // a touch of contrast, like the thesis print
            od.setPixelColour (x, y, Colour::fromFloatRGBA (b, b, b * 0.985f, a));
        }
    return out;
}

Image boardScan (nabra::MT type, int num)
{
    // seven colour scans of real boards; board7 is the one marked "dust", the rest are shared out
    const int idx = type == nabra::MT::Dust ? 7 : (((int) type * 5 + num * 3) % 6) + 1;
    static std::map<int, Image> cache;
    auto& i = cache[idx];
    if (! i.isValid()) i = loadRes ("board" + String (idx) + "_jpg");
    return i;
}

Image diagram (const String& res)
{
    static std::map<String, Image> cache;
    auto& i = cache[res];
    if (! i.isValid()) i = loadRes (res + "_png");
    return i;
}

Image midiBoard (nabra::MT type)
{
    static std::map<int, Image> cache;
    auto& img = cache[(int) type];
    if (img.isValid()) return img;
    const int W = 400, H = 500;
    img = Image (Image::ARGB, W, H, true);
    {
    Graphics g (img);
    R rng ((uint32_t) type * 31u + 5u);
    // board
    Rectangle<float> b (28, 30, W - 56, H - 60);
    g.setColour (Colour (0xff6f6f6b));
    g.fillRoundedRectangle (b, 6);
    for (int i = 0; i < 1600; ++i)
    {
        g.setColour (Colour::greyLevel (rng.r (0.3f, 0.6f)).withAlpha (0.25f));
        g.fillRect (b.getX() + rng.r (0, b.getWidth()), b.getY() + rng.r (0, b.getHeight()), 2.0f, 2.0f);
    }
    // stripboard copper strips
    for (float y = b.getY() + 14; y < b.getBottom() - 10; y += 15.0f)
    {
        g.setColour (Colour (0xff8d8c86));
        g.fillRect (b.getX() + 10, y - 4.5f, b.getWidth() - 20, 9.0f);
        for (float x = b.getX() + 18; x < b.getRight() - 14; x += 15.0f)
        {
            g.setColour (Colour (0xff3a3a38));
            g.fillEllipse (x - 2, y - 2, 4, 4);
        }
    }
    // the edge pads like the hand-etched boards
    g.setColour (Colour (0xffa7a6a0));
    for (int i = 0; i < 4; ++i)
    {
        g.fillRect (b.getX(), b.getY() + 40 + i * 100.0f, 22.0f, 60.0f);
        g.fillRect (b.getRight() - 22, b.getY() + 40 + i * 100.0f, 22.0f, 60.0f);
    }
    // MIDI DIN socket
    Point<float> dc (b.getCentreX(), b.getY() + 90);
    g.setColour (Colour (0xff2a2a28)); g.fillEllipse (dc.x - 46, dc.y - 46, 92, 92);
    g.setColour (Colour (0xffb9b8b2)); g.drawEllipse (dc.x - 46, dc.y - 46, 92, 92, 5);
    g.setColour (Colour (0xff111111)); g.fillEllipse (dc.x - 30, dc.y - 30, 60, 60);
    for (int i = 0; i < 5; ++i)
    {
        float a = 3.1416f + 3.1416f * (float) i / 4.0f;
        g.setColour (Colour (0xffd8d6cc));
        g.fillEllipse (dc.x + std::cos (a) * 19 - 4, dc.y + std::sin (a) * 19 - 4 + 6, 8, 8);
    }
    // opto-coupler DIP + a jellybean
    auto dip = [&] (Rectangle<float> r, const String& t)
    {
        g.setColour (Colour (0xff1c1c1c)); g.fillRoundedRectangle (r, 3);
        g.setColour (Colour (0xffcfcfcf));
        int legs = (int) (r.getWidth() / 16);
        for (int i = 0; i < legs; ++i)
        {
            g.fillRect (r.getX() + 8 + i * 16.0f, r.getY() - 6, 5.0f, 6.0f);
            g.fillRect (r.getX() + 8 + i * 16.0f, r.getBottom(), 5.0f, 6.0f);
        }
        g.setColour (Colour (0xff9a9a9a)); g.setFont (art::mono (17, true));
        g.drawText (t, r, Justification::centred);
        g.fillEllipse (r.getX() + 5, r.getCentreY() - 3, 6, 6);
    };
    dip ({ b.getX() + 70, b.getY() + 190, 70, 44 }, "6N138");
    dip ({ b.getX() + 170, b.getY() + 270, 102, 44 }, "LM324");
    // resistors
    for (int i = 0; i < 6; ++i)
    {
        float x = b.getX() + 60 + rng.r (0, b.getWidth() - 140), y = b.getY() + 340 + rng.r (0, 70);
        g.setColour (Colour (0xffbab6a8)); g.fillRoundedRectangle (x, y, 34, 11, 4);
        g.setColour (Colour (0xff333333)); g.fillRect (x + 8, y, 3.0f, 11.0f); g.fillRect (x + 16, y, 3.0f, 11.0f);
    }
    // solder blobs
    for (int i = 0; i < 60; ++i)
    {
        R& q = rng;
        float x = b.getX() + q.r (20, b.getWidth() - 20), y = b.getY() + q.r (20, b.getHeight() - 20);
        g.setColour (Colour (0xffc9c8c2).withAlpha (0.8f));
        g.fillEllipse (x, y, 7, 6);
    }
    }
    Image::BitmapData d (img, Image::BitmapData::readWrite);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x)
        {
            auto c = d.getPixelColour (x, y);
            float br = c.getBrightness();
            d.setPixelColour (x, y, Colour::fromFloatRGBA (br, br, br * 0.985f, c.getFloatAlpha()));
        }
    return img;
}

// ======================= canvas =======================
void paintCanvas (Graphics& g, Rectangle<float> area, uint32_t seed, float density)
{
    R rng (seed);
    g.setColour (Colour (0xffe9e7e0));
    g.fillRect (area);
    // weave
    g.setTiledImageFill (weaveTile(), 0, 0, 1.0f);
    g.fillRect (area);

    const float A = area.getWidth() * area.getHeight() / (2000.0f * 800.0f) * density;
    auto rp = [&] { return Point<float> (area.getX() + rng.r (0, area.getWidth()), area.getY() + rng.r (0, area.getHeight())); };

    // pale grey washes
    for (int i = 0; i < (int) (260 * A); ++i)
    {
        float rad = std::pow (rng.u(), 2.0f) * 26.0f + 3.0f;
        g.setColour (Colour::greyLevel (rng.r (0.55f, 0.8f)).withAlpha (rng.r (0.35f, 0.75f)));
        g.fillPath (blob (rp(), rad, rng, 10, 0.45f));
    }
    // dark grey mid splats
    for (int i = 0; i < (int) (380 * A); ++i)
    {
        float rad = std::pow (rng.u(), 2.5f) * 14.0f + 1.5f;
        g.setColour (Colour::greyLevel (rng.r (0.3f, 0.5f)).withAlpha (rng.r (0.6f, 0.9f)));
        g.fillPath (blob (rp(), rad, rng));
    }
    // skeins: Pollock's poured lines
    for (int i = 0; i < (int) (80 * A); ++i)
    {
        auto p = rp();
        float ang = rng.r (0, 6.283f);
        float w = rng.r (0.5f, 2.4f);
        int segs = 6 + (int) rng.r (0, 14);
        g.setColour (Colour::greyLevel (rng.r (0.02f, 0.14f)).withAlpha (rng.r (0.7f, 0.95f)));
        for (int s = 0; s < segs; ++s)
        {
            ang += rng.r (-0.7f, 0.7f);
            float len = rng.r (8.0f, 34.0f);
            auto q = p + Point<float> (std::cos (ang) * len, std::sin (ang) * len);
            w = juce::jlimit (0.3f, 3.0f, w + rng.r (-0.5f, 0.5f));
            g.drawLine (Line<float> (p, q), w);
            if (rng.u() < 0.15f) g.fillEllipse (Rectangle<float> (w * 2.6f, w * 2.6f).withCentre (q));
            p = q;
        }
    }
    // black splatter with satellites
    for (int i = 0; i < (int) (1400 * A); ++i)
    {
        float rad = std::pow (rng.u(), 3.2f) * 11.0f + 0.6f;
        auto c = rp();
        g.setColour (Colour::greyLevel (rng.r (0.0f, 0.12f)).withAlpha (rng.r (0.82f, 1.0f)));
        g.fillPath (blob (c, rad, rng, 8, 0.3f));
        int sat = rad > 4 ? (int) rng.r (2, 9) : (int) rng.r (0, 3);
        for (int s = 0; s < sat; ++s)
        {
            float a = rng.r (0, 6.283f), d = rad * rng.r (1.3f, 3.2f), sr = rad * rng.r (0.08f, 0.3f);
            g.fillEllipse (Rectangle<float> (sr * 2, sr * 2).withCentre (c + Point<float> (std::cos (a) * d, std::sin (a) * d)));
        }
    }
    // fine speckle
    for (int i = 0; i < (int) (2600 * A); ++i)
    {
        float rad = rng.r (0.3f, 1.2f);
        g.setColour (Colour::greyLevel (rng.r (0.0f, 0.35f)).withAlpha (rng.r (0.5f, 0.95f)));
        g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (rp()));
    }
}

void paintSewnPatch (Graphics& g, Rectangle<float> a)
{
    // a raw, unpainted strip of duck canvas sewn onto the tapestry
    g.setColour (Colour (0x30000000));
    g.fillRoundedRectangle (a.translated (2, 4), 4);
    g.setColour (Colour (0xffd9d3c3));
    g.fillRoundedRectangle (a, 3);
    g.setTiledImageFill (weaveTile(), 0, 0, 1.4f);
    g.fillRoundedRectangle (a, 3);
    paintCanvas (g, a.reduced (6), 991, 0.18f);
    g.setColour (Colour (0xffd9d3c3).withAlpha (0.55f));
    g.fillRect (a.reduced (6));
    // stitches
    g.setColour (Colour (0xff3b3a36));
    float dash[] = { 5.0f, 4.0f };
    Path p; p.addRoundedRectangle (a.reduced (5), 3);
    Path d; PathStrokeType (1.2f).createDashedStroke (d, p, dash, 2);
    g.fillPath (d);
}

void paintBoardShadow (Graphics& g, Rectangle<float> b)
{
    for (int i = 6; i >= 1; --i)
    {
        g.setColour (Colour::fromFloatRGBA (0, 0, 0, 0.05f));
        g.fillRoundedRectangle (b.translated (2.5f, 4.0f).expanded ((float) i * 1.4f), 6.0f + i);
    }
}

// ======================= jacks =======================
static Point<float> outward (nabra::Side s)
{
    switch (s)
    {
        case nabra::Side::L: return { -1, 0 };
        case nabra::Side::R: return { 1, 0 };
        case nabra::Side::T: return { 0, -1 };
        case nabra::Side::B: return { 0, 1 };
    }
    return {};
}

Point<float> loopTip (Point<float> e, nabra::Side s) { return e + outward (s) * 12.0f; }

void drawLoop (Graphics& g, Point<float> e, nabra::Side s, bool lit)
{
    auto o = outward (s);
    Point<float> t (-o.y, o.x);   // along the edge
    Path p;
    auto P = [&] (float along, float out) { return e + t * along + o * out; };
    p.startNewSubPath (P (-6.5f, -2.0f));
    p.cubicTo (P (-9.0f, 9.0f), P (-5.0f, 14.5f), P (0.0f, 14.5f));
    p.cubicTo (P (5.0f, 14.5f), P (9.0f, 9.0f), P (6.5f, -2.0f));
    if (lit)
    {
        g.setColour (Colour (0x66ffd27a));
        g.strokePath (p, PathStrokeType (8.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (Colour (0xcc1a120c));
    g.strokePath (p, PathStrokeType (3.8f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setGradientFill (ColourGradient (Colour (0xffe0a46a), P (-6, 12).x, P (-6, 12).y, Colour (0xff8a4a22), P (6, 0).x, P (6, 0).y, false));
    g.strokePath (p, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
    // braid
    float dash[] = { 1.4f, 1.6f };
    Path d; PathStrokeType (1.0f).createDashedStroke (d, p, dash, 2);
    g.setColour (Colour (0xfff6d2a4).withAlpha (0.8f));
    g.fillPath (d);
    // solder pad where it meets the board
    g.setColour (Colour (0xffbdbcb6));
    g.fillEllipse (Rectangle<float> (5, 5).withCentre (P (-6.5f, -2.5f)));
    g.fillEllipse (Rectangle<float> (5, 5).withCentre (P (6.5f, -2.5f)));
}

float tapeWidth (const String& text, bool out)
{
    return GlyphArrangement::getStringWidth (font (9.5f, out), text) + 7.0f;
}

void drawTape (Graphics& g, Rectangle<float> r, const String& text, bool out, bool inv, float angle)
{
    Graphics::ScopedSaveState ss (g);
    g.addTransform (AffineTransform::rotation (angle, r.getCentreX(), r.getCentreY()));
    g.setColour (Colour (0x40000000));
    g.fillRect (r.translated (0.6f, 1.0f));
    Colour bg = inv ? Colour (0xff141414) : (out ? Colour (0xfffbfaf4) : Colour (0xffeee2c4));
    g.setColour (bg.withAlpha (0.95f));
    g.fillRect (r);
    // torn ends
    g.setColour (bg.darker (0.25f).withAlpha (0.6f));
    for (float y = r.getY(); y < r.getBottom(); y += 2.0f)
    {
        g.fillRect (r.getX() - 0.8f, y, 0.8f, 1.0f);
        g.fillRect (r.getRight(), y + 1.0f, 0.8f, 1.0f);
    }
    g.setColour (inv ? Colour (0xfff2f2f2) : Colour (0xff141414));
    g.setFont (font (9.5f, out));
    g.drawText (text, r, Justification::centred, false);
}

float dymoWidth (const String& text)
{
    return GlyphArrangement::getStringWidth (Font (FontOptions ("Arial", 9.0f, Font::bold).withKerningFactor (0.14f)), text) + 12.0f;
}

void drawDymo (Graphics& g, Point<float> c, const String& text)
{
    auto w = dymoWidth (text);
    Rectangle<float> r (w, 14.0f);
    r.setCentre (c);
    g.setColour (Colour (0x55000000));
    g.fillRoundedRectangle (r.translated (0.8f, 1.4f), 2.0f);
    g.setGradientFill (ColourGradient (Colour (0xff2a2a2c), r.getX(), r.getY(), Colour (0xff121214), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (Colour (0x22ffffff));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + 2, r.getRight() - 2);
    auto f = Font (FontOptions ("Arial", 9.0f, Font::bold).withKerningFactor (0.14f));
    g.setFont (f);
    g.setColour (Colour (0x55000000));
    g.drawText (text, r.translated (0.5f, 0.7f), Justification::centred, false);
    g.setColour (Colour (0xffeeeeea));
    g.drawText (text, r, Justification::centred, false);
}

// ======================= cords =======================
Colour cordColour (int idx)
{
    static const uint32 c[] = { 0xffb8312f, 0xff252525, 0xffd9b42c, 0xff3c7a3a, 0xff2d5b98, 0xffe6e3da };
    return Colour (c[(idx % 6 + 6) % 6]);
}

static void bez (Point<float> a, Point<float> b, Point<float>& c1, Point<float>& c2)
{
    float dist = a.getDistanceFrom (b);
    float sag = juce::jmin (70.0f, 10.0f + 0.07f * dist);
    c1 = a + (b - a) * 0.22f + Point<float> (0, sag);
    c2 = b + (a - b) * 0.22f + Point<float> (0, sag);
}

static Point<float> bezAt (Point<float> a, Point<float> c1, Point<float> c2, Point<float> b, float t)
{
    float u = 1 - t;
    return a * (u * u * u) + c1 * (3 * u * u * t) + c2 * (3 * u * t * t) + b * (t * t * t);
}

Path cablePath (Point<float> a, Point<float> b)
{
    Point<float> c1, c2;
    bez (a, b, c1, c2);
    Path p;
    p.startNewSubPath (a);
    p.cubicTo (c1, c2, b);
    return p;
}

float distanceToCable (Point<float> a, Point<float> b, Point<float> p)
{
    Point<float> c1, c2;
    bez (a, b, c1, c2);
    float best = 1e9f;
    Point<float> prev = a;
    for (int i = 1; i <= 32; ++i)
    {
        auto q = bezAt (a, c1, c2, b, (float) i / 32.0f);
        Line<float> l (prev, q);
        Point<float> np;
        best = juce::jmin (best, l.getDistanceFromPoint (p, np));
        prev = q;
    }
    return best;
}

static void drawClip (Graphics& g, Point<float> tip, float angle, Colour boot)
{
    Graphics::ScopedSaveState ss (g);
    g.addTransform (AffineTransform::rotation (angle).translated (tip));
    // local: x axis points from tip back along the cord
    Path jaw;
    jaw.startNewSubPath (0, -1.2f); jaw.lineTo (8, -3.2f); jaw.lineTo (8, 3.2f); jaw.lineTo (0, 1.2f); jaw.closeSubPath();
    g.setColour (Colour (0xff6c6c6a)); g.fillPath (jaw);
    g.setColour (Colour (0xffd8d8d2)); g.fillRect (1.5f, -2.0f, 6.0f, 1.0f);
    g.setColour (Colour (0xff3a3a3a));
    for (float x = 1.5f; x < 7.5f; x += 1.5f) g.fillRect (x, -0.6f, 0.6f, 1.2f);
    Rectangle<float> b (7.5f, -3.8f, 10.0f, 7.6f);
    g.setColour (boot.darker (0.5f)); g.fillRoundedRectangle (b.expanded (0.6f), 2.5f);
    g.setColour (boot); g.fillRoundedRectangle (b, 2.5f);
    g.setColour (Colours::white.withAlpha (0.25f)); g.fillRoundedRectangle (b.withHeight (2.0f).translated (1, 0.8f).withWidth (7), 1.0f);
}

static void drawPart (Graphics& g, Point<float> c, float angle, nabra::Cord cord, int resVal)
{
    Graphics::ScopedSaveState ss (g);
    g.addTransform (AffineTransform::rotation (angle).translated (c));
    g.setColour (Colour (0xffb9b9b4));
    g.drawLine (-13, 0, 13, 0, 1.2f);   // tinned leads
    if (cord == nabra::Cord::Resistor)
    {
        Path p; p.addRoundedRectangle (-9.0f, -3.4f, 18.0f, 6.8f, 3.0f);
        g.setColour (Colour (0xffd8c296)); g.fillPath (p);
        g.setColour (Colour (0xff7d6a45)); g.strokePath (p, PathStrokeType (0.6f));
        const Colour bands[3][3] = { { Colour (0xff6b3e1e), Colours::black, Colour (0xffc0302a) },
                                     { Colour (0xff6b3e1e), Colours::black, Colour (0xffe07a1c) },
                                     { Colour (0xff6b3e1e), Colours::black, Colour (0xffe2c51f) } };
        for (int i = 0; i < 3; ++i) { g.setColour (bands[juce::jlimit (0, 2, resVal)][i]); g.fillRect (-5.5f + i * 3.2f, -3.4f, 1.6f, 6.8f); }
        g.setColour (Colour (0xffc8a24a)); g.fillRect (5.0f, -3.4f, 1.6f, 6.8f);
    }
    else if (cord == nabra::Cord::Capacitor)
    {
        Path p; p.addRoundedRectangle (-7.5f, -4.4f, 15.0f, 8.8f, 3.5f);
        g.setColour (Colour (0xff2f5f9c)); g.fillPath (p);
        g.setColour (Colour (0xff12305a)); g.strokePath (p, PathStrokeType (0.6f));
        g.setColour (Colour (0xffd8e2ee)); g.fillRect (-7.0f, -1.0f, 14.0f, 1.0f);
        g.setColour (Colours::white.withAlpha (0.3f)); g.fillRoundedRectangle (-5.0f, -3.6f, 9.0f, 1.4f, 0.7f);
    }
    else if (cord == nabra::Cord::Diode)
    {
        Path p; p.addRoundedRectangle (-7.0f, -2.6f, 14.0f, 5.2f, 2.2f);
        g.setColour (Colour (0xff1c1a18)); g.fillPath (p);
        g.setColour (Colour (0xffc9c9c9)); g.fillRect (3.6f, -2.6f, 1.8f, 5.2f);   // cathode band toward the input
        g.setColour (Colours::white.withAlpha (0.25f)); g.fillRect (-5.0f, -2.0f, 7.0f, 0.8f);
    }
}

void drawCable (Graphics& g, Point<float> a, Point<float> b, int colour, nabra::Cord cord, int resVal, bool hover, float alpha,
                bool clipA, bool clipB)
{
    Point<float> c1, c2;
    bez (a, b, c1, c2);
    Path p; p.startNewSubPath (a); p.cubicTo (c1, c2, b);
    auto col = cordColour (colour).withMultipliedAlpha (alpha);
    // shadow on the canvas
    g.setColour (Colour (0x28000000).withMultipliedAlpha (alpha));
    g.strokePath (p, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation (2.5f, 5.0f));
    if (hover)
    {
        g.setColour (Colour (0x88fff2b0));
        g.strokePath (p, PathStrokeType (8.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (col.darker (0.7f));
    g.strokePath (p, PathStrokeType (4.2f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (col);
    g.strokePath (p, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::white.withAlpha (0.28f * alpha));
    g.strokePath (p, PathStrokeType (0.9f), AffineTransform::translation (-0.6f, -0.8f));

    auto angleAt = [&] (float t0, float t1) { auto q0 = bezAt (a, c1, c2, b, t0), q1 = bezAt (a, c1, c2, b, t1); return std::atan2 (q1.y - q0.y, q1.x - q0.x); };
    if (cord != nabra::Cord::Wire)
        drawPart (g, bezAt (a, c1, c2, b, 0.5f), angleAt (0.47f, 0.53f), cord, resVal);
    if (clipA) drawClip (g, a, angleAt (0.0f, 0.06f), col);
    if (clipB) drawClip (g, b, angleAt (1.0f, 0.94f), col);
}

void drawCordIcon (Graphics& g, Rectangle<float> r, nabra::Cord cord, int resVal)
{
    Point<float> a (r.getX() + 4, r.getCentreY() - 6), b (r.getRight() - 4, r.getCentreY() - 6);
    Path p; p.startNewSubPath (a); p.quadraticTo ({ r.getCentreX(), r.getCentreY() + 12 }, b);
    auto col = cordColour (cord == nabra::Cord::Wire ? 0 : cord == nabra::Cord::Resistor ? 2 : cord == nabra::Cord::Capacitor ? 4 : 1);
    g.setColour (col.darker (0.7f)); g.strokePath (p, PathStrokeType (3.6f));
    g.setColour (col); g.strokePath (p, PathStrokeType (2.4f));
    drawClip (g, a, -0.9f + 3.1416f, col);
    drawClip (g, b, 0.9f, col);
    if (cord != nabra::Cord::Wire) drawPart (g, { r.getCentreX(), r.getCentreY() + 3 }, 0.0f, cord, resVal);
}

// ======================= knob / led =======================
void drawKnob (Graphics& g, Point<float> c, float r, float v, bool hover)
{
    // hex nut
    Path hex;
    for (int i = 0; i < 6; ++i)
    {
        float a = 3.1416f / 6 + i * 3.1416f / 3;
        auto q = c + Point<float> (std::cos (a), std::sin (a)) * (r + 3.5f);
        if (i == 0) hex.startNewSubPath (q); else hex.lineTo (q);
    }
    hex.closeSubPath();
    g.setColour (Colour (0x55000000)); g.fillPath (hex, AffineTransform::translation (1.2f, 2.0f));
    g.setGradientFill (ColourGradient (Colour (0xffdadad4), c.x - r, c.y - r, Colour (0xff6d6d69), c.x + r, c.y + r, false));
    g.fillPath (hex);
    // knurled cap
    auto cap = Rectangle<float> (r * 2, r * 2).withCentre (c);
    g.setColour (Colour (0xff2b2b2a)); g.fillEllipse (cap.expanded (0.8f));
    g.setGradientFill (ColourGradient (Colour (0xfff0f0ea), c.x - r * 0.6f, c.y - r * 0.8f, Colour (0xff7c7c78), c.x + r, c.y + r, true));
    g.fillEllipse (cap);
    g.setColour (Colour (0x40000000));
    for (int i = 0; i < 28; ++i)
    {
        float a = i * 6.2832f / 28;
        g.drawLine (Line<float> (c + Point<float> (std::cos (a), std::sin (a)) * (r - 1.6f), c + Point<float> (std::cos (a), std::sin (a)) * r), 0.7f);
    }
    float ang = (-135.0f + 270.0f * v) * 3.1416f / 180.0f;
    Point<float> d (std::sin (ang), -std::cos (ang));
    g.setColour (Colour (0xff1a1a1a));
    g.drawLine (Line<float> (c - d * (r * 0.15f), c + d * (r * 0.92f)), 2.0f);
    if (hover) { g.setColour (Colour (0x66fff2b0)); g.drawEllipse (cap.expanded (3.5f), 2.0f); }
}

Colour ledColour (int idx)
{
    static const uint32 c[] = { 0xffff3b26, 0xff6cff3a, 0xffffb21f };
    return Colour (c[(idx % 3 + 3) % 3]);
}

void drawLed (Graphics& g, Point<float> c, int idx, float level)
{
    auto col = ledColour (idx);
    auto r = Rectangle<float> (6.5f, 6.5f).withCentre (c);
    g.setColour (Colour (0xff222222)); g.fillEllipse (r.expanded (1.2f));
    g.setColour (col.darker (2.2f).interpolatedWith (col, juce::jlimit (0.0f, 1.0f, level)));
    g.fillEllipse (r);
    if (level > 0.05f)
    {
        g.setGradientFill (ColourGradient (col.withAlpha (0.55f * level), c.x, c.y, col.withAlpha (0.0f), c.x + 11, c.y, true));
        g.fillEllipse (r.expanded (8));
    }
    g.setColour (Colours::white.withAlpha (0.55f));
    g.fillEllipse (r.getX() + 1.4f, r.getY() + 1.1f, 2.0f, 1.6f);
}
} // namespace art
