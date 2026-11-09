#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace nabra;

juce::AudioProcessorValueTreeState::ParameterLayout NabraProcessor::makeLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "master", 1 }, "Master", 0.0f, 1.5f, 0.8f));
    for (int i = 0; i < (int) cells().size(); ++i)
    {
        int k = knobParamIndex (i);
        if (k < 0) continue;
        l.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "vol" + juce::String (k), 1 },
                                                            cellName (i) + " volume", 0.0f, 1.0f, 0.7f));
    }
    return l;
}

NabraProcessor::NabraProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", makeLayout())
{
    engine.master = apvts.getRawParameterValue ("master");
    for (int k = 0; k < knobParamCount(); ++k)
        engine.knobs[(size_t) k] = apvts.getRawParameterValue ("vol" + juce::String (k));
    loadRecipe (3); // "Kee sings": play your keyboard straight away
    currentProgram = 3;
}

bool NabraProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto o = l.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

void NabraProcessor::prepareToPlay (double sr, int block)
{
    os = std::make_unique<juce::dsp::Oversampling<float>> (2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    os->initProcessing ((size_t) block);
    setLatencySamples ((int) std::round (os->getLatencyInSamples()));
    work.setSize (2, block);
    engine.prepare (sr * 2.0);
    {
        const juce::SpinLock::ScopedLockType sl (patchLock);
        pendingPatch = cables;
        patchPending = true;
    }
}

void NabraProcessor::pushPatch()
{
    const juce::SpinLock::ScopedLockType sl (patchLock);
    pendingPatch = cables;
    patchPending = true;
}

void NabraProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiBuf)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    if (os == nullptr) return;

    {
        const juce::SpinLock::ScopedTryLockType tl (patchLock);
        if (tl.isLocked() && patchPending)
        {
            engine.setPatch (pendingPatch);   // no allocation: conns has reserved capacity
            patchPending = false;
        }
    }

    // ---- transport ----
    auto& m = engine.midi;
    bool standalone = wrapperType == wrapperType_Standalone;
    bool gotHost = false;
    if (! standalone)
        if (auto* ph = getPlayHead())
            if (auto pos = ph->getPosition())
            {
                gotHost = true;
                if (auto bpm = pos->getBpm()) m.bpm = *bpm;
                if (auto ts = pos->getTimeSignature()) m.barLenQ = 4.0 * ts->numerator / juce::jmax (1, ts->denominator);
                bool playing = pos->getIsPlaying();
                if (playing)
                {
                    if (auto q = pos->getPpqPosition()) m.ppq = *q;
                    if (! m.playing && m.ppq < 0) m.ppq = 0;
                }
                m.playing = playing;
            }
    if (! gotHost) { if (! m.playing) m.ppq = 0; m.playing = true; m.bpm = 120.0; m.barLenQ = 4.0; }

    juce::dsp::AudioBlock<float> block (work.getArrayOfWritePointers(), 2, (size_t) n);
    block.clear();
    auto up = os->processSamplesUp (block);
    up.clear();
    float* L = up.getChannelPointer (0);
    float* R = up.getChannelPointer (1);

    int pos = 0;
    auto renderTo = [&] (int end)
    {
        if (end > pos) engine.render (L + pos * 2, R + pos * 2, (end - pos) * 2);
        pos = juce::jmax (pos, end);
    };
    for (const auto meta : midiBuf)
    {
        renderTo (juce::jlimit (0, n, meta.samplePosition));
        const auto msg = meta.getMessage();
        if (msg.isNoteOn()) m.noteOn (msg.getNoteNumber(), msg.getFloatVelocity());
        else if (msg.isNoteOff()) m.noteOff (msg.getNoteNumber());
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) m.allOff();
        else if (msg.isPitchWheel()) m.bend = (float) (msg.getPitchWheelValue() - 8192) / 8192.0f;
        else if (msg.isChannelPressure()) m.press = msg.getChannelPressureValue() / 127.0f;
        else if (msg.isAftertouch()) m.press = msg.getAfterTouchValue() / 127.0f;
        else if (msg.isController())
        {
            float v = msg.getControllerValue() / 127.0f;
            switch (msg.getControllerNumber())
            {
                case 1: m.mod = v; break;
                case 11: m.expr = v; break;
                case 74: m.brite = v; break;
                default: break;
            }
        }
    }
    renderTo (n);

    os->processSamplesDown (block);
    buffer.copyFrom (0, 0, work, 0, 0, n);
    if (buffer.getNumChannels() > 1) buffer.copyFrom (1, 0, work, 1, 0, n);
    else buffer.addFrom (0, 0, work, 1, 0, n), buffer.applyGain (0.5f);
}

// ---------------- patch editing ----------------
int NabraProcessor::addCable (JackRef src, JackRef dst, Cord cord, int resVal)
{
    for (auto& c : cables)
        if (c.src == src && c.dst == dst) return c.id;
    Cable c;
    c.id = nextId++;
    c.src = src; c.dst = dst; c.cord = cord; c.resVal = resVal; c.colour = nextColour++ % 6;
    cables.push_back (c);
    pushPatch();
    return c.id;
}

void NabraProcessor::removeCable (int id)
{
    cables.erase (std::remove_if (cables.begin(), cables.end(), [id] (auto& c) { return c.id == id; }), cables.end());
    pushPatch();
}

void NabraProcessor::updateCable (const Cable& u)
{
    for (auto& c : cables) if (c.id == u.id) c = u;
    pushPatch();
}

void NabraProcessor::clearCables() { cables.clear(); pushPatch(); sendChangeMessage(); }

void NabraProcessor::setCables (std::vector<Cable> v)
{
    cables = std::move (v);
    nextId = 1; nextColour = 0;
    for (auto& c : cables) { nextId = juce::jmax (nextId, c.id + 1); nextColour = juce::jmax (nextColour, c.colour + 1); }
    pushPatch();
    sendChangeMessage();
}

void NabraProcessor::loadRecipe (int index)
{
    if (index < 0 || index >= (int) recipes().size()) { setCables ({}); return; }
    setCables (buildRecipe (recipes()[(size_t) index]));
}

int NabraProcessor::getNumPrograms() { return (int) recipes().size() + 1; }
void NabraProcessor::setCurrentProgram (int i)
{
    currentProgram = i;
    loadRecipe (i < (int) recipes().size() ? i : -1);
}
const juce::String NabraProcessor::getProgramName (int i)
{
    return i < (int) recipes().size() ? juce::String (recipes()[(size_t) i].name) : juce::String ("Empty tapestry");
}

// ---------------- state ----------------
static juce::String jackKey (JackRef j)
{
    return cellKey (j.cell) + "." + spec (cells()[(size_t) j.cell].type).jacks[(size_t) j.jack].id;
}
static JackRef parseJack (const juce::String& s)
{
    JackRef j;
    j.cell = findCell (s.upToFirstOccurrenceOf (".", false, false));
    if (j.cell >= 0) j.jack = spec (cells()[(size_t) j.cell].type).jackIndex (s.fromFirstOccurrenceOf (".", false, false));
    return j;
}

void NabraProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement root ("NABRA");
    root.setAttribute ("version", 1);
    root.setAttribute ("uiScale", uiScale);
    root.setAttribute ("labels", showLabels);
    root.setAttribute ("boards", colourBoards ? "colour" : "thesis");
    root.setAttribute ("program", currentProgram);
    if (auto x = apvts.copyState().createXml()) root.addChildElement (x.release());
    auto* p = root.createNewChildElement ("PATCH");
    for (auto& c : cables)
    {
        auto* e = p->createNewChildElement ("CORD");
        e->setAttribute ("src", jackKey (c.src));
        e->setAttribute ("dst", jackKey (c.dst));
        e->setAttribute ("type", (int) c.cord);
        e->setAttribute ("res", c.resVal);
        e->setAttribute ("colour", c.colour);
    }
    copyXmlToBinary (root, dest);
}

void NabraProcessor::setStateInformation (const void* data, int size)
{
    auto x = getXmlFromBinary (data, size);
    if (x == nullptr || ! x->hasTagName ("NABRA")) return;
    uiScale = (float) x->getDoubleAttribute ("uiScale", uiScale);
    showLabels = x->getBoolAttribute ("labels", true);
    colourBoards = x->getStringAttribute ("boards", "thesis") == "colour";
    currentProgram = x->getIntAttribute ("program", currentProgram);
    if (auto* ps = x->getChildByName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*ps));
    std::vector<Cable> v;
    if (auto* p = x->getChildByName ("PATCH"))
    {
        int id = 1;
        for (auto* e : p->getChildIterator())
        {
            Cable c;
            c.src = parseJack (e->getStringAttribute ("src"));
            c.dst = parseJack (e->getStringAttribute ("dst"));
            if (! c.src.valid() || ! c.dst.valid()) continue;
            c.id = id++;
            c.cord = (Cord) juce::jlimit (0, 3, e->getIntAttribute ("type"));
            c.resVal = e->getIntAttribute ("res", 1);
            c.colour = e->getIntAttribute ("colour", 0);
            v.push_back (c);
        }
    }
    setCables (std::move (v));
}

juce::AudioProcessorEditor* NabraProcessor::createEditor() { return new NabraEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NabraProcessor(); }
