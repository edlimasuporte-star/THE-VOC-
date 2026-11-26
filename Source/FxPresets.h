// THE VOC FX - presets de fabrica por estilo + gerador criativo INSPIRE (C++ puro)
#pragma once
#include "FxParams.h"
#include "FxDsp.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdio>

namespace vfx
{
struct FactoryPreset { const char* name; const char* category; const char* data; };

// data: "id=valor id=valor ..." (o que nao aparece fica no valor padrao)
static const FactoryPreset kFactory[] = {
    { "INIT", "BASICO", "" },

    // ---------------- POP ----------------
    { "Pop Stack Brilhante", "POP", "hmOn=1 autoInt1=11 autoInt2=13 hmLvl=-9 hmHuman=30 hmWidth=80 corrOn=1 corrSpeed=25 chOn=1 chMix=20 dlOn=1 dlDiv=3 dlMix=12 dlFeedback=25 outGain=2" },
    { "Pop Afinado", "POP", "corrOn=1 corrSpeed=0 corrStr=100 dlOn=1 dlDiv=5 dlMix=15 chOn=1 chMix=15 outGain=2" },
    { "Refrao Dobrado", "POP", "hmOn=1 autoInt1=9 autoInt2=2 autoLvl2=40 hmHuman=60 hmWidth=100 hmLvl=-8 chOn=1 chMix=25 corrOn=1 corrSpeed=30 outGain=3" },
    { "Pop Vocoder Pad", "POP", "vcOn=1 vcMode=0 vcSynth=2 vcLvl=-8 autoInt1=11 autoInt2=13 voiceLvl=-3 chOn=1 chMix=30 dlOn=1 dlMix=15 outGain=5" },
    { "Bridge Radio", "POP", "tfOn=1 tfType=4 flOn=1 flType=1 flFreq=45 dlOn=1 dlDiv=3 dlMix=20 outGain=4.5" },
    { "Pop Chop", "POP", "shOn=1 shSeq=5 shLength=2 shMix=60 corrOn=1 dlOn=1 dlMix=10 outGain=2.5" },

    // ---------------- ALTERNATIVO ----------------
    { "Megafone Grunge", "ALTERNATIVO", "dsOn=1 dsType=3 dsDrive=55 dsMix=70 tfOn=1 tfType=5 flOn=1 flType=1 flFreq=50 outGain=-4" },
    { "Voz Fantasma", "ALTERNATIVO", "hmOn=1 autoInt1=2 hmFormant=-40 hmLvl=-6 trOn=1 trBreath=60 trShift=-30 trLvl=-10 dlOn=1 dlDiv=6 dlFeedback=55 dlMix=30 chOn=1 outGain=4" },
    { "Ring Metalico", "ALTERNATIVO", "rmOn=1 rmFreq=180 rmMix=40 rmLfo=0.5 rmDepth=20 dsOn=1 dsType=2 dsDrive=30 dsMix=30" },
    { "Talkbox Sujo", "ALTERNATIVO", "tbOn=1 tbDrive=70 tbSpeaker=80 tbLvl=-4 tbSynth=5 voiceLvl=-12 dsOn=1 dsType=1 dsMix=40" },
    { "Coro Distante", "ALTERNATIVO", "hmOn=1 autoInt1=7 autoInt2=11 autoInt3=16 hmHuman=70 hmWidth=100 hmLvl=-8 tfOn=1 tfType=0 tfMix=40 dlOn=1 dlDiv=5 dlMix=30 dlFeedback=45" },
    { "Scream Filter", "ALTERNATIVO", "flOn=1 flType=2 flFreq=55 flRes=60 flMix=50 dsOn=1 dsType=0 dsDrive=40 outGain=-5" },

    // ---------------- INDIE ----------------
    { "Indie Dobrado Lo-Fi", "INDIE", "hmOn=1 autoInt1=9 hmHuman=45 hmLvl=-6 tfOn=1 tfType=3 chOn=1 chMix=30 flOn=1 flType=0 flFreq=78 outGain=5.5" },
    { "Quarto de Fita", "INDIE", "tfOn=1 tfType=7 dsOn=1 dsType=0 dsDrive=20 dsMix=30 dlOn=1 dlDiv=4 dlFeedback=40 dlMix=22 flOn=1 flFreq=72 outGain=-2.5" },
    { "Harmonia Folk", "INDIE", "hmOn=1 autoInt1=11 autoInt2=13 hmLvl=-10 hmHuman=50 hmWidth=70 dlOn=1 dlDiv=5 dlMix=12" },
    { "Vocoder Caseiro", "INDIE", "vcOn=1 vcMode=1 vcSynth=4 vcLvl=-6 voiceLvl=-6 tfOn=1 tfType=5 tfMix=50 outGain=8.5" },
    { "Sonho Shoegaze", "INDIE", "chOn=1 chDepth=80 chMix=50 dlOn=1 dlDiv=6 dlFeedback=65 dlMix=40 hmOn=1 autoInt1=16 hmLvl=-14 flOn=1 flFreq=68 outGain=2.5" },
    { "Robo Timido", "INDIE", "rbOn=1 rbMode=0 rbBits=10 rbLvl=-10 voiceLvl=-2 dlOn=1 dlMix=18" },

    // ---------------- ELETRONICO ----------------
    { "Capacete Robotico", "ELETRONICO", "tbOn=1 tbSynth=0 tbDrive=40 tbLvl=-2 voiceLvl=-60 corrOn=1 corrSpeed=0 chOn=1 chMix=20 outGain=2.5" },
    { "Vocoder Club", "ELETRONICO", "vcOn=1 vcMode=2 vcSynth=2 vcLvl=-3 voiceLvl=-14 autoInt1=13 autoInt2=16 dlOn=1 dlDiv=3 dlMix=20 dlFeedback=40 outGain=7.5" },
    { "Glitch Shred", "ELETRONICO", "shOn=1 shSeq=9 shLength=2 shRateA=1 shRateB=3 shRateC=4 shMix=100 dsOn=1 dsType=2 dsMix=25 outGain=-3.5" },
    { "Bitcrush Vox", "ELETRONICO", "rbOn=1 rbMode=1 rbBits=75 rbLvl=-4 voiceLvl=-12 flOn=1 flType=3 flFreq=60 outGain=6.5" },
    { "Trance Supersaw", "ELETRONICO", "vcOn=1 vcMode=0 vcSynth=2 vcLvl=-4 autoInt1=11 autoInt2=13 autoInt3=16 voiceLvl=-10 dlOn=1 dlDiv=4 dlMix=30 chOn=1 chMix=30 outGain=4" },
    { "Ring Techno", "ELETRONICO", "rmOn=1 rmSync=1 rmDiv=4 rmMix=60 flOn=1 flType=1 flFreq=40 dlOn=1 dlDiv=3 dlMix=25 outGain=4.5" },
    { "Monstro Grave", "ELETRONICO", "hmOn=1 autoInt1=2 autoInt2=1 hmFormant=-70 hmLvl=-3 voiceLvl=-60 dsOn=1 dsType=0 dsDrive=25 dsMix=30 outGain=-1.5" },

    // ---------------- WORSHIP ----------------
    { "Coro de Adoracao", "WORSHIP", "hmOn=1 autoInt1=11 autoInt2=13 autoInt3=2 autoLvl3=50 hmHuman=55 hmWidth=100 hmLvl=-7 chOn=1 chMix=25 dlOn=1 dlDiv=5 dlFeedback=45 dlMix=22 outGain=2" },
    { "Pad Celestial", "WORSHIP", "vcOn=1 vcMode=0 vcSynth=4 vcScale=25 vcLvl=-8 synAttack=180 synRelease=900 autoInt1=13 autoInt2=16 chOn=1 chDepth=60 chMix=40 dlOn=1 dlDiv=6 dlFeedback=55 dlMix=30 outGain=2" },
    { "Ministracao Suave", "WORSHIP", "corrOn=1 corrSpeed=45 corrStr=70 dlOn=1 dlDiv=5 dlMix=20 dlFeedback=40 chOn=1 chMix=15" },
    { "Oitavas de Gloria", "WORSHIP", "hmOn=1 autoInt1=16 autoInt2=2 autoLvl1=60 autoLvl2=55 hmHuman=35 hmLvl=-8 dlOn=1 dlMix=25 dlDiv=5" },
    { "Orgao Vocal", "WORSHIP", "vcOn=1 vcMode=1 vcSynth=8 vcLvl=-9 autoInt1=13 voiceLvl=-2 dlOn=1 dlDiv=6 dlMix=25 dlFeedback=50" },
    { "Atmosfera", "WORSHIP", "trOn=1 trSynth=4 trClarity=40 trBreath=40 trLvl=-12 chOn=1 chDepth=80 chMix=40 dlOn=1 dlDiv=7 dlFeedback=70 dlMix=35 flOn=1 flFreq=70 outGain=2" },

    // ---------------- ANOS 80 ----------------
    { "Vocoder 1983", "ANOS 80", "vcOn=1 vcMode=1 vcSynth=0 vcLvl=-4 voiceLvl=-30 chOn=1 chMix=35 dlOn=1 dlDiv=3 dlMix=20 outGain=5.5" },
    { "Talkbox Funk", "ANOS 80", "tbOn=1 tbSynth=0 tbMode=2 tbDrive=50 tbLvl=-3 voiceLvl=-60 chOn=1 chMix=20 outGain=7" },
    { "Synth Brass Choir", "ANOS 80", "vcOn=1 vcSynth=9 vcMode=0 vcLvl=-5 autoInt1=11 autoInt2=13 voiceLvl=-8 chOn=1 chMix=40 dlOn=1 dlDiv=5 dlMix=20 outGain=6.5" },
    { "Gated Arena", "ANOS 80", "gate=-50 chOn=1 chMix=40 dlOn=1 dlDiv=0 dlFeedback=60 dlMix=35 tfOn=1 tfType=3 outGain=8" },
    { "Fala e Soletra", "ANOS 80", "rbOn=1 rbMode=0 rbBits=45 rbLvl=-2 rbSynth=1 voiceLvl=-60" },
    { "Neon Chorus", "ANOS 80", "chOn=1 chDepth=70 chRate=0.6 chMix=55 dlOn=1 dlDiv=4 dlMix=25 hmOn=1 autoInt1=9 hmHuman=40 hmLvl=-10 outGain=4.5" },

    // ---------------- ANOS 90 ----------------
    { "Hard Tune 98", "ANOS 90", "corrOn=1 corrSpeed=0 corrStr=100 chOn=1 chMix=15 dlOn=1 dlDiv=3 dlMix=12 outGain=1.5" },
    { "Telefone Trip-Hop", "ANOS 90", "tfOn=1 tfType=4 dsOn=1 dsType=1 dsDrive=25 dsMix=30 dlOn=1 dlDiv=5 dlMix=25 dlFeedback=50 outGain=5" },
    { "Grunge Radio", "ANOS 90", "dsOn=1 dsType=3 dsDrive=45 dsMix=45 tfOn=1 tfType=6 flOn=1 flType=1 flFreq=42 outGain=-8.5" },
    { "Eurodance Vocoder", "ANOS 90", "vcOn=1 vcMode=2 vcSynth=1 vcLvl=-5 autoInt1=13 voiceLvl=-6 dlOn=1 dlDiv=3 dlMix=18 outGain=4" },
    { "R&B Harmonias", "ANOS 90", "hmOn=1 autoInt1=11 autoInt2=13 autoInt3=15 hmLvl=-9 hmHuman=40 corrOn=1 corrSpeed=30 chOn=1 chMix=20 outGain=2.5" },
    { "Big Beat Filter", "ANOS 90", "flOn=1 flType=3 flFreq=35 flRes=50 dsOn=1 dsType=2 dsDrive=35 dsMix=40 shOn=1 shSeq=4 shMix=50 outGain=-3.5" },

    // ---------------- HIP HOP / TRAP ----------------
    { "Trap Tune", "HIP HOP", "corrOn=1 corrSpeed=0 scale=1 hmOn=1 autoInt1=2 autoLvl1=35 hmLvl=-10 dlOn=1 dlDiv=3 dlMix=15" },
    { "Ad-lib Grave", "HIP HOP", "hmOn=1 autoInt1=2 hmFormant=-50 hmLvl=-2 voiceLvl=-6 dsOn=1 dsDrive=15 dsMix=20" },
    { "Robo Sombrio", "HIP HOP", "rbOn=1 rbMode=2 rbBats=70 rbLvl=-8 voiceLvl=-2" },
    { "Lo-Fi Phone", "HIP HOP", "tfOn=1 tfType=4 rbOn=1 rbBits=30 rbLvl=-16 flOn=1 flFreq=65 outGain=7.5" },

    // ---------------- EXPERIMENTAL ----------------
    { "Morcego Alienigena", "EXPERIMENTAL", "rbOn=1 rbBats=100 rbBytes=50 rbMode=2 rbLvl=-4 voiceLvl=-60 rmOn=1 rmFreq=90 rmMix=30 outGain=9" },
    { "Esquilo Espacial", "EXPERIMENTAL", "hmOn=1 autoInt1=16 hmFormant=80 hmChar=100 hmLvl=0 voiceLvl=-60 dlOn=1 dlMix=30" },
    { "Maquina Quebrada", "EXPERIMENTAL", "shOn=1 shSeq=13 shLength=3 shRateA=2 shRateB=4 shRateC=3 rbOn=1 rbBits=90 rbLvl=-8 voiceLvl=-6 outGain=3" },
    { "Tract Nasal", "EXPERIMENTAL", "trOn=1 trNasal=90 trShift=40 trClarity=80 trLvl=-2 voiceLvl=-60 outGain=-2" },
    { "Radio Alien", "EXPERIMENTAL", "tfOn=1 tfType=2 rmOn=1 rmFreq=600 rmLfo=4 rmDepth=40 rmMix=50 dlOn=1 dlDiv=1 dlFeedback=70 dlMix=30 outGain=4.5" },
};

inline int numFactoryPresets() { return (int) (sizeof (kFactory) / sizeof (kFactory[0])); }

static const char* kCategories[] = { "BASICO", "POP", "ALTERNATIVO", "INDIE", "ELETRONICO", "WORSHIP", "ANOS 80", "ANOS 90", "HIP HOP", "EXPERIMENTAL" };
inline int numCategories() { return (int) (sizeof (kCategories) / sizeof (kCategories[0])); }

inline void defaultValues (float* v) { for (int i = 0; i < P_COUNT; ++i) v[i] = kParams[i].def; }

// aplica "id=valor id=valor" sobre v
inline void applyData (const char* data, float* v)
{
    std::string s (data ? data : "");
    size_t pos = 0;
    while (pos < s.size())
    {
        while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\n' || s[pos] == '\r' || s[pos] == '\t')) ++pos;
        size_t end = pos; while (end < s.size() && s[end] != ' ' && s[end] != '\n' && s[end] != '\r' && s[end] != '\t') ++end;
        const std::string tok = s.substr (pos, end - pos);
        const size_t eq = tok.find ('=');
        if (eq != std::string::npos)
        {
            const int id = findParam (tok.substr (0, eq).c_str());
            if (id >= 0) v[id] = clampParam (id, (float) std::atof (tok.substr (eq + 1).c_str()));
        }
        pos = end;
    }
}

inline void factoryPresetValues (int idx, float* v)
{
    defaultValues (v);
    if (idx >= 0 && idx < numFactoryPresets()) applyData (kFactory[idx].data, v);
}

// serializa todos os valores (formato dos presets do usuario)
inline std::string serialize (const float* v)
{
    std::string out; char buf[64];
    for (int i = 0; i < P_COUNT; ++i) { std::snprintf (buf, sizeof (buf), "%s=%g\n", kParams[i].id, (double) v[i]); out += buf; }
    return out;
}

// ------------------------------------------------------------------
// INSPIRE: gera um preset criativo aleatorio (mas musical)
// keep: valores atuais (mantem modo, tom, escala, registro e gate da musica)
// ------------------------------------------------------------------
inline void inspire (uint32_t seed, float* v, std::string& name, const float* keep = nullptr)
{
    Rng r (seed * 2654435761u + 12345u);
    auto U = [&] (float a, float b) { return a + (b - a) * r.uni(); };
    auto I = [&] (int a, int b) { return a + (int) (r.uni() * (float) (b - a + 1) * 0.9999f); };
    auto chance = [&] (float pr) { return r.uni() < pr; };
    defaultValues (v);
    if (keep) for (int id : { (int) P_mode, (int) P_key, (int) P_scale, (int) P_reg, (int) P_gate, (int) P_polyMode }) v[id] = keep[id];

    static const int chords[][3] = { { 11, 13, 0 }, { 13, 16, 0 }, { 2, 0, 0 }, { 11, 0, 0 }, { 9, 2, 0 }, { 16, 2, 0 }, { 7, 11, 16 }, { 11, 13, 2 }, { 13, 0, 0 }, { 9, 0, 0 }, { 6, 11, 0 }, { 15, 11, 13 } };
    const int ch = I (0, 11);
    for (int k = 0; k < 3; ++k) { v[P_autoInt1 + k * 2] = (float) chords[ch][k]; v[P_autoLvl1 + k * 2] = U (45.f, 90.f); }

    // arquetipo
    const int arch = I (0, 9);
    int modsOn = 0;
    auto mod = [&] (int which)
    {
        ++modsOn;
        switch (which)
        {
            case 0: v[P_trOn] = 1; v[P_trClarity] = U (30, 100); v[P_trShift] = U (-70, 70); v[P_trNasal] = chance (0.3f) ? U (20, 90) : 0.f; v[P_trBreath] = U (0, 60); v[P_trLvl] = U (-9, -2); v[P_trSynth] = (float) I (0, 9); v[P_trPan] = U (-30, 30); break;
            case 1: v[P_vcOn] = 1; v[P_vcMode] = (float) I (0, 2); v[P_vcShift] = chance (0.5f) ? U (-50, 50) : 0.f; v[P_vcScale] = U (25, 90); v[P_vcLvl] = U (-8, -2); v[P_vcSynth] = (float) I (0, 9); break;
            case 2: v[P_rbOn] = 1; v[P_rbMode] = (float) I (0, 2); v[P_rbBits] = U (0, 80); v[P_rbBytes] = chance (0.4f) ? U (10, 80) : 0.f; v[P_rbBats] = chance (0.4f) ? U (10, 100) : 0.f; v[P_rbLvl] = U (-9, -2); v[P_rbSynth] = (float) I (0, 9); break;
            case 3: v[P_tbOn] = 1; v[P_tbDrive] = U (10, 80); v[P_tbSpeaker] = U (20, 100); v[P_tbFormant] = chance (0.5f) ? U (-60, 60) : 0.f; v[P_tbMode] = (float) I (0, 2); v[P_tbLvl] = U (-7, -1); v[P_tbSynth] = (float) I (0, 9); break;
            default: v[P_hmOn] = 1; v[P_hmFormant] = chance (0.6f) ? 0.f : U (-80, 80); v[P_hmChar] = chance (0.7f) ? 0.f : U (20, 100); v[P_hmHuman] = U (10, 80); v[P_hmLvl] = U (-10, -3); v[P_hmWidth] = U (40, 100); break;
        }
    };
    switch (arch)
    {
        case 0: mod (4); break;                       // coro
        case 1: mod (1); break;                       // vocoder
        case 2: mod (3); break;                       // talkbox
        case 3: mod (2); break;                       // robo
        case 4: mod (0); break;                       // trato
        case 5: mod (1); mod (4); break;              // vocoder + coro
        case 6: mod (2); mod (3); break;              // robo + talkbox
        case 7: mod (0); mod (4); break;              // trato + coro
        case 8: mod (I (0, 4)); break;
        default: break;                               // so efeitos
    }
    v[P_voiceLvl] = modsOn == 0 ? U (-3, 0) : (chance (0.3f) ? -60.f : U (-12, -1));
    if (v[P_voiceLvl] < -59.f)
    {
        // sem voz seca: modulos mais presentes e harmonia com pelo menos 2 vozes
        for (int id : { (int) P_trLvl, (int) P_vcLvl, (int) P_rbLvl, (int) P_tbLvl, (int) P_hmLvl }) v[id] = std::min (v[id] + 4.f, 2.f);
        if (v[P_autoInt2] < 0.5f) v[P_autoInt2] = 9.f;
        v[P_outGain] = 2.f;
    }
    if (chance (0.35f)) { v[P_corrOn] = 1; v[P_corrSpeed] = chance (0.5f) ? 0.f : U (10, 50); v[P_corrStr] = U (60, 100); }
    if (chance (0.15f)) v[P_synOct] = (float) (chance (0.5f) ? -1 : 1);

    // efeitos
    int fxCount = modsOn == 0 ? I (2, 4) : I (1, 3);
    int guard = 0;
    while (fxCount > 0 && guard++ < 30)
    {
        const int f = I (0, 6);
        switch (f)
        {
            case 0: if (v[P_dsOn] > 0.5f) continue; v[P_dsOn] = 1; v[P_dsDrive] = U (10, 70); v[P_dsTone] = U (30, 90); v[P_dsType] = (float) I (0, 3); v[P_dsMix] = U (20, 70); break;
            case 1: if (v[P_rmOn] > 0.5f) continue; v[P_rmOn] = 1; v[P_rmSync] = chance (0.4f) ? 1.f : 0.f; v[P_rmFreq] = U (40, 900); v[P_rmDiv] = (float) I (2, 5); v[P_rmLfo] = U (0, 6); v[P_rmDepth] = U (0, 50); v[P_rmMix] = U (15, 50); break;
            case 2: if (v[P_flOn] > 0.5f) continue; v[P_flOn] = 1; v[P_flType] = (float) I (0, 3); v[P_flFreq] = (int) v[P_flType] == 1 ? U (30, 55) : U (45, 80); v[P_flRes] = U (0, 60); v[P_flMix] = (int) v[P_flType] == 2 ? U (25, 50) : U (50, 100); break;
            case 3: if (v[P_tfOn] > 0.5f) continue; v[P_tfOn] = 1; v[P_tfType] = (float) I (0, 7); v[P_tfWidth] = U (-30, 60); v[P_tfMix] = ((int) v[P_tfType] <= 1) ? U (30, 60) : U (40, 100); break;
            case 4: if (v[P_shOn] > 0.5f || ! chance (0.6f)) continue; v[P_shOn] = 1; v[P_shLength] = (float) I (1, 3); v[P_shSeq] = (float) I (1, 16); v[P_shRateA] = (float) I (0, 2); v[P_shRateB] = (float) I (1, 3); v[P_shRateC] = (float) I (2, 4); v[P_shMix] = U (40, 100); break;
            case 5: if (v[P_chOn] > 0.5f) continue; v[P_chOn] = 1; v[P_chDepth] = U (20, 90); v[P_chRate] = U (0.2f, 2.5f); v[P_chWidth] = U (40, 100); v[P_chMix] = U (15, 55); break;
            default: if (v[P_dlOn] > 0.5f) continue; v[P_dlOn] = 1; v[P_dlSync] = 1; v[P_dlDiv] = (float) I (1, 7); v[P_dlWidth] = U (20, 100); v[P_dlFeedback] = U (15, 70); v[P_dlMix] = U (10, 35); break;
        }
        --fxCount;
    }
    for (int i = 0; i < P_COUNT; ++i) v[i] = clampParam (i, v[i]);

    static const char* adj[] = { "NEON", "CROMO", "VELUDO", "COSMICO", "FANTASMA", "ELETRICO", "SOLAR", "LUNAR", "ANALOGICO", "TURBO", "DOURADO", "SELVAGEM",
                                 "LIQUIDO", "VINTAGE", "MAGNETICO", "PIXEL", "ATOMICO", "CELESTE", "NOTURNO", "SUPER" };
    static const char* noun[] = { "PITCHER", "ORBITA", "CAPACETE", "COMETA", "TRANSMISSOR", "CORAL", "ROBO", "SINAL", "RADAR", "ECO", "FOGUETE", "SATELITE",
                                  "HOME RUN", "VISEIRA", "MICROFONE", "NEBULOSA", "ESTADIO", "LANCE", "FREQUENCIA", "GALAXIA" };
    name = std::string (noun[I (0, 19)]) + " " + adj[I (0, 19)];
}
} // namespace vfx
