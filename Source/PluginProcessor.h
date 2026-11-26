#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "FxEngine.h"
#include "FxPresets.h"

// Alvos do XY pad (o capacete)
struct XYTarget { const char* label; int param; };
static const XYTarget kXYTargets[] = {
    { "---", -1 }, { "MIX", vfx::P_outMix }, { "VOZ", vfx::P_voiceLvl },
    { "TRACT LVL", vfx::P_trLvl }, { "TRACT SHIFT", vfx::P_trShift }, { "TRACT CLARITY", vfx::P_trClarity },
    { "VOCODER LVL", vfx::P_vcLvl }, { "VOCODER SHIFT", vfx::P_vcShift }, { "VOCODER SCALE", vfx::P_vcScale },
    { "ROBOT LVL", vfx::P_rbLvl }, { "ROBOT BITS", vfx::P_rbBits }, { "ROBOT BATS", vfx::P_rbBats },
    { "TALKBOX LVL", vfx::P_tbLvl }, { "TALKBOX FORMANT", vfx::P_tbFormant }, { "TALKBOX DRIVE", vfx::P_tbDrive },
    { "HARMONY LVL", vfx::P_hmLvl }, { "HARMONY FORMANT", vfx::P_hmFormant }, { "HARMONY HUMANIZE", vfx::P_hmHuman },
    { "DISTORT DRIVE", vfx::P_dsDrive }, { "RING FREQ", vfx::P_rmFreq }, { "FILTER FREQ", vfx::P_flFreq }, { "FILTER RES", vfx::P_flRes },
    { "SHRED MIX", vfx::P_shMix }, { "CHORUS MIX", vfx::P_chMix }, { "DELAY MIX", vfx::P_dlMix }, { "DELAY FEEDBACK", vfx::P_dlFeedback },
};
static constexpr int kNumXYTargets = (int) (sizeof (kXYTargets) / sizeof (kXYTargets[0]));

struct UserPreset { juce::String name, category; juce::File file; };

class TheVocFxProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    TheVocFxProcessor();
    ~TheVocFxProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "THE VOC FX"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return vfx::numFactoryPresets(); }
    int getCurrentProgram() override { return juce::jmax (0, currentProgram); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---------- presets ----------
    void loadValues (const float* v, const juce::String& name);
    void currentValues (float* v) const;
    void loadFactory (int index);
    bool loadUserPreset (const juce::File& f);
    bool saveUserPreset (const juce::String& name, const juce::String& category);
    juce::Array<UserPreset> scanUserPresets() const;
    static juce::File userPresetFolder();
    void stepPreset (int delta);   // anterior / proximo (fabrica + usuario)
    juce::String presetName() const { return currentName; }

    // ---------- INSPIRE ----------
    struct Idea { juce::String name, tags; float values[vfx::P_COUNT]; };
    void generateIdeas (int count);
    void applyIdea (int i);
    const std::vector<Idea>& getIdeas() const { return ideas; }

    // ---------- XY pad ----------
    int xyTarget[2] { 1, 25 };
    float xyPos[2] { 1.f, 0.35f };
    void setXY (float x, float y, bool begin, bool end);

    juce::AudioProcessorValueTreeState apvts;
    vfx::Engine engine;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;

    std::atomic<float>* raw[vfx::P_COUNT] {};
    juce::String currentName { "INIT" };
    int currentProgram = 0;
    juce::File currentUserFile;
    std::atomic<int> pendingProgram { -1 };
    std::vector<Idea> ideas;
    juce::Random rnd;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheVocFxProcessor)
};
