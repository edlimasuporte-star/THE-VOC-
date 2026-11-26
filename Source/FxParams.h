// THE VOC FX - tabela central de parametros (C++ puro, sem JUCE).
// Usada pelo motor DSP, pelos presets, pelo gerador INSPIRE e pela camada JUCE.
#pragma once
#include <cstring>
#include <string>

namespace vfx
{
enum ParamType { PFLOAT = 0, PCHOICE = 1, PBOOL = 2 };

// X(id, nome, tipo, min, max, default, escolhas "A|B|C", centro-de-skew (0 = linear), unidade)
#define VOCFX_PARAMS(X) \
    /* ---------- global / pitch / voicing ---------- */ \
    X (mode,       "Mode",            PCHOICE, 0, 2, 0, "AUTO|MIDI|SIDECHAIN", 0, "") \
    X (reg,        "Register",        PCHOICE, 0, 2, 1, "LOW|MID|HIGH", 0, "") \
    X (key,        "Key",             PCHOICE, 0, 11, 0, "C|C#|D|Eb|E|F|F#|G|Ab|A|Bb|B", 0, "") \
    X (scale,      "Scale",           PCHOICE, 0, 5, 0, "Major|Minor|Chromatic|Maj Pent|Min Pent|Dorian", 0, "") \
    X (corrOn,     "Pitch Correct",   PBOOL,   0, 1, 0, "", 0, "") \
    X (corrSpeed,  "Correct Speed",   PFLOAT,  0, 100, 20, "", 0, "ms") \
    X (corrStr,    "Correct Strength",PFLOAT,  0, 100, 100, "", 0, "%") \
    X (voiceLvl,   "Voice Level",     PFLOAT,  -60, 6, 0, "", 0, "dB") \
    X (polyMode,   "MIDI Voicing",    PCHOICE, 0, 1, 1, "MONO|POLY", 0, "") \
    X (glide,      "Glide",           PFLOAT,  0, 100, 0, "", 0, "%") \
    X (autoLvl0,   "Auto Root Level", PFLOAT,  0, 100, 100, "", 0, "%") \
    X (autoInt1,   "Auto Voice 1",    PCHOICE, 0, 17, 0, "Off|-2 Oct|-Oct|-7th|-6th|-5th|-4th|-3rd|-2nd|Unison|+2nd|+3rd|+4th|+5th|+6th|+7th|+Oct|+2 Oct", 0, "") \
    X (autoLvl1,   "Auto Voice 1 Lvl",PFLOAT,  0, 100, 70, "", 0, "%") \
    X (autoInt2,   "Auto Voice 2",    PCHOICE, 0, 17, 0, "Off|-2 Oct|-Oct|-7th|-6th|-5th|-4th|-3rd|-2nd|Unison|+2nd|+3rd|+4th|+5th|+6th|+7th|+Oct|+2 Oct", 0, "") \
    X (autoLvl2,   "Auto Voice 2 Lvl",PFLOAT,  0, 100, 70, "", 0, "%") \
    X (autoInt3,   "Auto Voice 3",    PCHOICE, 0, 17, 0, "Off|-2 Oct|-Oct|-7th|-6th|-5th|-4th|-3rd|-2nd|Unison|+2nd|+3rd|+4th|+5th|+6th|+7th|+Oct|+2 Oct", 0, "") \
    X (autoLvl3,   "Auto Voice 3 Lvl",PFLOAT,  0, 100, 70, "", 0, "%") \
    /* ---------- synth (portadora interna) ---------- */ \
    X (synOct,     "Synth Octave",    PFLOAT,  -2, 2, 0, "", 0, "oct") \
    X (synAttack,  "Synth Attack",    PFLOAT,  0, 500, 4, "", 60, "ms") \
    X (synRelease, "Synth Release",   PFLOAT,  5, 2000, 120, "", 200, "ms") \
    X (synVib,     "Synth Vibrato",   PFLOAT,  0, 100, 0, "", 0, "%") \
    X (synVibRate, "Synth Vib Rate",  PFLOAT,  0.5, 10, 5.5, "", 0, "Hz") \
    /* ---------- TRACT (trato vocal / Biovox) ---------- */ \
    X (trOn,       "Tract On",        PBOOL,   0, 1, 0, "", 0, "") \
    X (trClarity,  "Tract Clarity",   PFLOAT,  0, 100, 70, "", 0, "%") \
    X (trShift,    "Tract Shift",     PFLOAT,  -100, 100, 0, "", 0, "") \
    X (trNasal,    "Tract Nasal",     PFLOAT,  0, 100, 0, "", 0, "%") \
    X (trBreath,   "Tract Breath",    PFLOAT,  0, 100, 10, "", 0, "%") \
    X (trLvl,      "Tract Level",     PFLOAT,  -60, 6, -6, "", 0, "dB") \
    X (trPan,      "Tract Pan",       PFLOAT,  -100, 100, 0, "", 0, "") \
    X (trSynth,    "Tract Synth",     PCHOICE, 0, 9, 4, "Classic Saw|Square|Super Saws|Sync Lead|Soft Tri|Saw + Sub|Hollow Pulse|Air Noise|Organ|80s Brass", 0, "") \
    /* ---------- VOCODER ---------- */ \
    X (vcOn,       "Vocoder On",      PBOOL,   0, 1, 0, "", 0, "") \
    X (vcMode,     "Vocoder Mode",    PCHOICE, 0, 2, 0, "SMOOTH|VINTAGE|HARD", 0, "") \
    X (vcShift,    "Vocoder Shift",   PFLOAT,  -100, 100, 0, "", 0, "") \
    X (vcScale,    "Vocoder Scale",   PFLOAT,  0, 100, 60, "", 0, "%") \
    X (vcLvl,      "Vocoder Level",   PFLOAT,  -60, 6, -6, "", 0, "dB") \
    X (vcPan,      "Vocoder Pan",     PFLOAT,  -100, 100, 0, "", 0, "") \
    X (vcSynth,    "Vocoder Synth",   PCHOICE, 0, 9, 2, "Classic Saw|Square|Super Saws|Sync Lead|Soft Tri|Saw + Sub|Hollow Pulse|Air Noise|Organ|80s Brass", 0, "") \
    /* ---------- ROBOT (LPC / Compuvox) ---------- */ \
    X (rbOn,       "Robot On",        PBOOL,   0, 1, 0, "", 0, "") \
    X (rbBits,     "Robot Bits",      PFLOAT,  0, 100, 20, "", 0, "%") \
    X (rbBytes,    "Robot Bytes",     PFLOAT,  0, 100, 0, "", 0, "%") \
    X (rbBats,     "Robot Bats",      PFLOAT,  0, 100, 0, "", 0, "%") \
    X (rbMode,     "Robot Mode",      PCHOICE, 0, 2, 0, "SPELL|READ|MATH", 0, "") \
    X (rbLvl,      "Robot Level",     PFLOAT,  -60, 6, -6, "", 0, "dB") \
    X (rbPan,      "Robot Pan",       PFLOAT,  -100, 100, 0, "", 0, "") \
    X (rbSynth,    "Robot Synth",     PCHOICE, 0, 9, 1, "Classic Saw|Square|Super Saws|Sync Lead|Soft Tri|Saw + Sub|Hollow Pulse|Air Noise|Organ|80s Brass", 0, "") \
    /* ---------- TALKBOX ---------- */ \
    X (tbOn,       "Talkbox On",      PBOOL,   0, 1, 0, "", 0, "") \
    X (tbDrive,    "Talkbox Drive",   PFLOAT,  0, 100, 30, "", 0, "%") \
    X (tbSpeaker,  "Talkbox Speaker", PFLOAT,  0, 100, 50, "", 0, "%") \
    X (tbFormant,  "Talkbox Formant", PFLOAT,  -100, 100, 0, "", 0, "") \
    X (tbMode,     "Talkbox Mode",    PCHOICE, 0, 2, 1, "DARK|CLASSIC|BRIGHT", 0, "") \
    X (tbLvl,      "Talkbox Level",   PFLOAT,  -60, 6, -6, "", 0, "dB") \
    X (tbPan,      "Talkbox Pan",     PFLOAT,  -100, 100, 0, "", 0, "") \
    X (tbSynth,    "Talkbox Synth",   PCHOICE, 0, 9, 0, "Classic Saw|Square|Super Saws|Sync Lead|Soft Tri|Saw + Sub|Hollow Pulse|Air Noise|Organ|80s Brass", 0, "") \
    /* ---------- HARMONY (Polyvox) ---------- */ \
    X (hmOn,       "Harmony On",      PBOOL,   0, 1, 0, "", 0, "") \
    X (hmFormant,  "Harmony Formant", PFLOAT,  -100, 100, 0, "", 0, "") \
    X (hmChar,     "Harmony Character",PFLOAT, 0, 100, 0, "", 0, "%") \
    X (hmHuman,    "Harmony Humanize",PFLOAT,  0, 100, 25, "", 0, "%") \
    X (hmLvl,      "Harmony Level",   PFLOAT,  -60, 6, -6, "", 0, "dB") \
    X (hmWidth,    "Harmony Width",   PFLOAT,  0, 100, 60, "", 0, "%") \
    /* ---------- EFEITOS ---------- */ \
    X (dsOn,       "Distort On",      PBOOL,   0, 1, 0, "", 0, "") \
    X (dsDrive,    "Distort Drive",   PFLOAT,  0, 100, 30, "", 0, "%") \
    X (dsTone,     "Distort Tone",    PFLOAT,  0, 100, 60, "", 0, "%") \
    X (dsType,     "Distort Type",    PCHOICE, 0, 3, 0, "WARM|ANALOG|EDGY|FUZZ", 0, "") \
    X (dsMix,      "Distort Mix",     PFLOAT,  0, 100, 50, "", 0, "%") \
    X (rmOn,       "RingMod On",      PBOOL,   0, 1, 0, "", 0, "") \
    X (rmSync,     "RingMod Sync",    PBOOL,   0, 1, 0, "", 0, "") \
    X (rmFreq,     "RingMod Freq",    PFLOAT,  0.5, 3000, 220, "", 200, "Hz") \
    X (rmDiv,      "RingMod Division",PCHOICE, 0, 6, 3, "1/1|1/2|1/4|1/8|1/16|1/32|1/64", 0, "") \
    X (rmLfo,      "RingMod LFO",     PFLOAT,  0, 20, 2, "", 0, "Hz") \
    X (rmDepth,    "RingMod Depth",   PFLOAT,  0, 100, 0, "", 0, "%") \
    X (rmMix,      "RingMod Mix",     PFLOAT,  0, 100, 50, "", 0, "%") \
    X (flOn,       "Filter On",       PBOOL,   0, 1, 0, "", 0, "") \
    X (flFreq,     "Filter Freq",     PFLOAT,  0, 100, 60, "", 0, "%") \
    X (flRes,      "Filter Res",      PFLOAT,  0, 100, 30, "", 0, "%") \
    X (flType,     "Filter Type",     PCHOICE, 0, 3, 0, "NY LP|NY HP|SCREAM|COMBO", 0, "") \
    X (flMix,      "Filter Mix",      PFLOAT,  0, 100, 100, "", 0, "%") \
    X (tfOn,       "Transform On",    PBOOL,   0, 1, 0, "", 0, "") \
    X (tfType,     "Transform Type",  PCHOICE, 0, 7, 4, "HOLLOW|AIR|CREEP|BROADCAST|RADIO|SMALL AMP|2X10 MID|2X10 SMILE", 0, "") \
    X (tfWidth,    "Transform Width", PFLOAT,  -100, 100, 0, "", 0, "") \
    X (tfMix,      "Transform Mix",   PFLOAT,  0, 100, 100, "", 0, "%") \
    X (shOn,       "Shred On",        PBOOL,   0, 1, 0, "", 0, "") \
    X (shLength,   "Shred Length",    PCHOICE, 0, 3, 1, "1/2|1/4|1/8|1/16", 0, "") \
    X (shSeq,      "Shred Seq",       PFLOAT,  1, 16, 1, "", 0, "") \
    X (shRateA,    "Shred Rate A",    PCHOICE, 0, 4, 1, "1/8|1/16|1/32|1/64|1/128", 0, "") \
    X (shRateB,    "Shred Rate B",    PCHOICE, 0, 4, 2, "1/8|1/16|1/32|1/64|1/128", 0, "") \
    X (shRateC,    "Shred Rate C",    PCHOICE, 0, 4, 3, "1/8|1/16|1/32|1/64|1/128", 0, "") \
    X (shMix,      "Shred Mix",       PFLOAT,  0, 100, 100, "", 0, "%") \
    X (chOn,       "Chorus On",       PBOOL,   0, 1, 0, "", 0, "") \
    X (chDepth,    "Chorus Depth",    PFLOAT,  0, 100, 40, "", 0, "%") \
    X (chRate,     "Chorus Rate",     PFLOAT,  0.05, 8, 0.8, "", 1, "Hz") \
    X (chWidth,    "Chorus Width",    PFLOAT,  0, 100, 70, "", 0, "%") \
    X (chMix,      "Chorus Mix",      PFLOAT,  0, 100, 40, "", 0, "%") \
    X (dlOn,       "Delay On",        PBOOL,   0, 1, 0, "", 0, "") \
    X (dlSync,     "Delay Sync",      PBOOL,   0, 1, 1, "", 0, "") \
    X (dlTime,     "Delay Time",      PFLOAT,  10, 2000, 350, "", 300, "ms") \
    X (dlDiv,      "Delay Division",  PCHOICE, 0, 8, 4, "1/32|1/16|1/16D|1/8|1/8D|1/4|1/4D|1/2|1/1", 0, "") \
    X (dlWidth,    "Delay Width",     PFLOAT,  0, 100, 50, "", 0, "%") \
    X (dlFeedback, "Delay Feedback",  PFLOAT,  0, 95, 35, "", 0, "%") \
    X (dlMix,      "Delay Mix",       PFLOAT,  0, 100, 25, "", 0, "%") \
    /* ---------- saida ---------- */ \
    X (outMix,     "Output Mix",      PFLOAT,  0, 100, 100, "", 0, "%") \
    X (gate,       "Gate",            PFLOAT,  -80, 0, -60, "", 0, "dB") \
    X (outPan,     "Output Pan",      PFLOAT,  -100, 100, 0, "", 0, "") \
    X (outWidth,   "Output Width",    PFLOAT,  -100, 100, 0, "", 0, "") \
    X (outGain,    "Output Gain",     PFLOAT,  -24, 12, 0, "", 0, "dB") \
    X (safety,     "Safety Limiter",  PBOOL,   0, 1, 1, "", 0, "")

enum ParamId
{
#define VOCFX_ENUM(id, name, type, mn, mx, df, ch, sk, unit) P_##id,
    VOCFX_PARAMS (VOCFX_ENUM)
#undef VOCFX_ENUM
    P_COUNT
};

struct ParamDef
{
    const char* id; const char* name; int type; float min, max, def; const char* choices; float skewCentre; const char* unit;
};

static const ParamDef kParams[P_COUNT] = {
#define VOCFX_DEF(id, name, type, mn, mx, df, ch, sk, unit) { #id, name, type, (float) (mn), (float) (mx), (float) (df), ch, (float) (sk), unit },
    VOCFX_PARAMS (VOCFX_DEF)
#undef VOCFX_DEF
};

inline int findParam (const char* id)
{
    for (int i = 0; i < P_COUNT; ++i)
        if (std::strcmp (kParams[i].id, id) == 0) return i;
    return -1;
}

inline int numChoices (int p) { return (int) (kParams[p].max - kParams[p].min) + 1; }

inline std::string choiceName (int p, int idx)
{
    const char* s = kParams[p].choices; int k = 0; std::string cur;
    for (; *s; ++s)
    {
        if (*s == '|') { if (k == idx) return cur; ++k; cur.clear(); }
        else cur += *s;
    }
    return k == idx ? cur : std::string();
}

inline float clampParam (int p, float v)
{
    const ParamDef& d = kParams[p];
    if (v < d.min) v = d.min;
    if (v > d.max) v = d.max;
    if (d.type != PFLOAT) v = (float) (int) (v + 0.5f);
    return v;
}

// Os 5 modulos e as cores da viseira
enum Module { M_TRACT = 0, M_VOCODER, M_ROBOT, M_TALKBOX, M_HARMONY, M_COUNT };
} // namespace vfx
