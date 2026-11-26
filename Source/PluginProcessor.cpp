#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FxLevel.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout TheVocFxProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> ps;
    for (int i = 0; i < vfx::P_COUNT; ++i)
    {
        const auto& d = vfx::kParams[i];
        const ParameterID pid { d.id, 1 };
        if (d.type == vfx::PBOOL)
            ps.push_back (std::make_unique<AudioParameterBool> (pid, d.name, d.def > 0.5f));
        else if (d.type == vfx::PCHOICE)
        {
            StringArray ch;
            for (int k = 0; k < vfx::numChoices (i); ++k) ch.add (vfx::choiceName (i, k));
            ps.push_back (std::make_unique<AudioParameterChoice> (pid, d.name, ch, (int) d.def));
        }
        else
        {
            const bool integerish = std::strcmp (d.id, "shSeq") == 0 || std::strcmp (d.id, "synOct") == 0;
            NormalisableRange<float> r (d.min, d.max, integerish ? 1.f : 0.f);
            if (d.skewCentre > 0.f) r.setSkewForCentre (d.skewCentre);
            const String unit (d.unit);
            auto fmt = [unit, integerish] (float v, int)
            {
                if (integerish) return String ((int) std::lround (v));
                if (unit == "dB" && v <= -59.9f) return String ("OFF");
                if (unit == "Hz" && v >= 1000.f) return String (v / 1000.f, 2) + " kHz";
                const String num = std::fabs (v) >= 100.f || unit == "%" ? String ((int) std::lround (v)) : String (v, 1);
                return unit.isEmpty() ? num : num + " " + unit;
            };
            ps.push_back (std::make_unique<AudioParameterFloat> (pid, d.name, r, d.def,
                AudioParameterFloatAttributes().withStringFromValueFunction (fmt).withLabel (unit)));
        }
    }
    return { ps.begin(), ps.end() };
}

TheVocFxProcessor::TheVocFxProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                                       .withInput ("Sidechain", AudioChannelSet::stereo(), false)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < vfx::P_COUNT; ++i) raw[i] = apvts.getRawParameterValue (vfx::kParams[i].id);
    startTimerHz (20);
}

TheVocFxProcessor::~TheVocFxProcessor() { stopTimer(); }

bool TheVocFxProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != AudioChannelSet::stereo()) return false;
    const auto in = l.getMainInputChannelSet();
    if (in != AudioChannelSet::stereo() && in != AudioChannelSet::mono()) return false;
    if (l.inputBuses.size() > 1)
    {
        const auto sc = l.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != AudioChannelSet::stereo() && sc != AudioChannelSet::mono()) return false;
    }
    return true;
}

void TheVocFxProcessor::prepareToPlay (double sr, int block)
{
    for (int i = 0; i < vfx::P_COUNT; ++i) engine.setParam (i, raw[i]->load());
    engine.prepare (sr, block);
    setLatencySamples (engine.latencySamples());
}

void TheVocFxProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;
    for (int i = 0; i < vfx::P_COUNT; ++i) engine.setParam (i, raw[i]->load());

    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn()) engine.noteOn (m.getNoteNumber(), m.getVelocity());
        else if (m.isNoteOff()) engine.noteOff (m.getNoteNumber());
        else if (m.isAllNotesOff() || m.isAllSoundOff()) engine.allNotesOff();
        else if (m.isProgramChange()) pendingProgram.store (m.getProgramChangeNumber());
    }

    double bpm = 120.0, ppq = 0.0; bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }

    const int n = buffer.getNumSamples();
    auto mainIn = getBusBuffer (buffer, true, 0);
    auto mainOut = getBusBuffer (buffer, false, 0);
    const float* inL = mainIn.getNumChannels() > 0 ? mainIn.getReadPointer (0) : nullptr;
    const float* inR = mainIn.getNumChannels() > 1 ? mainIn.getReadPointer (1) : inL;
    const float* scL = nullptr; const float* scR = nullptr;
    if (getBusCount (true) > 1)
    {
        auto* scBus = getBus (true, 1);
        if (scBus != nullptr && scBus->isEnabled())
        {
            auto sc = getBusBuffer (buffer, true, 1);
            if (sc.getNumChannels() > 0) { scL = sc.getReadPointer (0); scR = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : scL; }
        }
    }
    if (inL == nullptr || mainOut.getNumChannels() < 2) { buffer.clear(); return; }

    // copia de seguranca se a entrada mono ocupar o mesmo canal da saida
    engine.process (inL, inR, scL, scR, mainOut.getWritePointer (0), mainOut.getWritePointer (1), n, bpm, ppq, playing);
}

// ------------------------------------------------------------------ presets

void TheVocFxProcessor::loadValues (const float* v, const String& name)
{
    for (int i = 0; i < vfx::P_COUNT; ++i)
        if (auto* p = apvts.getParameter (vfx::kParams[i].id))
            p->setValueNotifyingHost (p->convertTo0to1 (vfx::clampParam (i, v[i])));
    currentName = name;
}

void TheVocFxProcessor::currentValues (float* v) const
{
    for (int i = 0; i < vfx::P_COUNT; ++i) v[i] = raw[i]->load();
}

void TheVocFxProcessor::loadFactory (int index)
{
    if (index < 0 || index >= vfx::numFactoryPresets()) return;
    float v[vfx::P_COUNT];
    vfx::factoryPresetValues (index, v);
    // o preset nao muda o modo, o tom e a escala da musica
    for (int id : { (int) vfx::P_mode, (int) vfx::P_key, (int) vfx::P_scale })
        if (std::strstr (vfx::kFactory[index].data, (String (vfx::kParams[id].id) + "=").toRawUTF8()) == nullptr) v[id] = raw[id]->load();
    loadValues (v, vfx::kFactory[index].name);
    currentProgram = index; currentUserFile = File();
}

void TheVocFxProcessor::setCurrentProgram (int index) { loadFactory (index); }

const String TheVocFxProcessor::getProgramName (int index)
{
    if (index < 0 || index >= vfx::numFactoryPresets()) return {};
    return String (vfx::kFactory[index].category) + " - " + vfx::kFactory[index].name;
}

File TheVocFxProcessor::userPresetFolder()
{
    auto f = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("THE VOC FX").getChildFile ("Presets");
    f.createDirectory();
    return f;
}

Array<UserPreset> TheVocFxProcessor::scanUserPresets() const
{
    Array<UserPreset> out;
    for (const auto& f : userPresetFolder().findChildFiles (File::findFiles, false, "*.vocfx"))
    {
        UserPreset u; u.file = f; u.name = f.getFileNameWithoutExtension(); u.category = "MEUS PRESETS";
        StringArray lines; lines.addLines (f.loadFileAsString());
        for (auto& l : lines)
        {
            if (l.startsWith ("name=")) u.name = l.fromFirstOccurrenceOf ("=", false, false).trim();
            if (l.startsWith ("category=")) u.category = l.fromFirstOccurrenceOf ("=", false, false).trim();
        }
        out.add (u);
    }
    std::sort (out.begin(), out.end(), [] (const UserPreset& a, const UserPreset& b) { return a.name.compareNatural (b.name) < 0; });
    return out;
}

bool TheVocFxProcessor::loadUserPreset (const File& f)
{
    if (! f.existsAsFile()) return false;
    float v[vfx::P_COUNT];
    currentValues (v);           // valores que nao estiverem no arquivo ficam como estao
    String name = f.getFileNameWithoutExtension();
    StringArray lines; lines.addLines (f.loadFileAsString());
    String data;
    for (auto& l : lines)
    {
        if (l.startsWith ("name=")) name = l.fromFirstOccurrenceOf ("=", false, false).trim();
        else if (! l.startsWith ("category=") && ! l.startsWith ("#")) data << l.trim() << " ";
    }
    vfx::applyData (data.toRawUTF8(), v);
    loadValues (v, name);
    currentUserFile = f; currentProgram = -1;
    return true;
}

bool TheVocFxProcessor::saveUserPreset (const String& nameIn, const String& category)
{
    const String name = nameIn.trim().isEmpty() ? String ("MEU PRESET") : nameIn.trim();
    float v[vfx::P_COUNT];
    currentValues (v);
    String text;
    text << "# THE VOC FX preset\n" << "name=" << name << "\n" << "category=" << category << "\n" << String (vfx::serialize (v));
    auto f = userPresetFolder().getChildFile (File::createLegalFileName (name) + ".vocfx");
    if (! f.replaceWithText (text)) return false;
    currentName = name; currentUserFile = f; currentProgram = -1;
    return true;
}

void TheVocFxProcessor::stepPreset (int delta)
{
    const auto users = scanUserPresets();
    const int nf = vfx::numFactoryPresets(), total = nf + users.size();
    int cur = currentProgram >= 0 ? currentProgram : -1;
    if (cur < 0)
        for (int i = 0; i < users.size(); ++i) if (users[i].file == currentUserFile) cur = nf + i;
    if (cur < 0) cur = 0;
    const int next = ((cur + delta) % total + total) % total;
    if (next < nf) loadFactory (next); else loadUserPreset (users[next - nf].file);
}

// ------------------------------------------------------------------ INSPIRE

void TheVocFxProcessor::generateIdeas (int count)
{
    ideas.clear();
    float keep[vfx::P_COUNT]; currentValues (keep);
    for (int k = 0; k < count; ++k)
    {
        Idea idea; std::string nm;
        vfx::inspire ((uint32_t) rnd.nextInt(), idea.values, nm, keep);
        idea.name = nm;
        StringArray t;
        const struct { int id; const char* tag; } tags[] = {
            { vfx::P_trOn, "TRACT" }, { vfx::P_vcOn, "VOCODER" }, { vfx::P_rbOn, "ROBOT" }, { vfx::P_tbOn, "TALKBOX" }, { vfx::P_hmOn, "HARMONY" },
            { vfx::P_corrOn, "AFINA" }, { vfx::P_dsOn, "DIST" }, { vfx::P_rmOn, "RING" }, { vfx::P_flOn, "FILTER" }, { vfx::P_tfOn, "TRANSF" },
            { vfx::P_shOn, "SHRED" }, { vfx::P_chOn, "CHORUS" }, { vfx::P_dlOn, "DELAY" } };
        for (auto& tg : tags) if (idea.values[tg.id] > 0.5f) t.add (tg.tag);
        idea.tags = t.joinIntoString (" + ");
        ideas.push_back (idea);
    }
}

void TheVocFxProcessor::applyIdea (int i)
{
    if (i < 0 || i >= (int) ideas.size()) return;
    auto& idea = ideas[(size_t) i];
    idea.values[vfx::P_outGain] = vfx::estimateMakeupDb (idea.values, getSampleRate() > 0 ? getSampleRate() : 48000.0);
    loadValues (idea.values, "* " + idea.name);
    currentProgram = -1; currentUserFile = File();
}

// ------------------------------------------------------------------ XY

void TheVocFxProcessor::setXY (float x, float y, bool begin, bool end)
{
    xyPos[0] = jlimit (0.f, 1.f, x); xyPos[1] = jlimit (0.f, 1.f, y);
    for (int a = 0; a < 2; ++a)
    {
        const int t = xyTarget[a];
        if (t <= 0 || t >= kNumXYTargets) continue;
        if (auto* p = apvts.getParameter (vfx::kParams[kXYTargets[t].param].id))
        {
            if (begin) p->beginChangeGesture();
            p->setValueNotifyingHost (xyPos[a]);
            if (end) p->endChangeGesture();
        }
    }
}

void TheVocFxProcessor::timerCallback()
{
    const int pc = pendingProgram.exchange (-1);
    if (pc >= 0)
    {
        const auto users = scanUserPresets();
        const int nf = vfx::numFactoryPresets();
        if (pc < nf) loadFactory (pc);
        else if (pc - nf < users.size()) loadUserPreset (users[pc - nf].file);
    }
}

// ------------------------------------------------------------------ estado

void TheVocFxProcessor::getStateInformation (MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", currentName, nullptr);
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("xyX", xyTarget[0], nullptr);
    state.setProperty ("xyY", xyTarget[1], nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void TheVocFxProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = ValueTree::fromXml (*xml);
            apvts.replaceState (vt);
            currentName = vt.getProperty ("presetName", "INIT").toString();
            currentProgram = (int) vt.getProperty ("program", -1);
            xyTarget[0] = jlimit (0, kNumXYTargets - 1, (int) vt.getProperty ("xyX", 1));
            xyTarget[1] = jlimit (0, kNumXYTargets - 1, (int) vt.getProperty ("xyY", 25));
        }
}

AudioProcessorEditor* TheVocFxProcessor::createEditor() { return new TheVocFxEditor (*this); }

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TheVocFxProcessor(); }
