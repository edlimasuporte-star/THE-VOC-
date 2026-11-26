// THE VOC FX - estimativa de ganho para presets INSPIRE (nivel parecido ao vivo, sem sustos)
#pragma once
#include "FxEngine.h"
#include <memory>

namespace vfx
{
// voz sintetica curta (pulsos glotais + formantes) usada so para medir nivel
inline std::vector<float> referenceVoice (double sr, double seconds)
{
    const int N = (int) (sr * seconds);
    std::vector<float> x ((size_t) N);
    Biquad f1, f2, f3; double ph = 0.0; int cur = -1;
    const float vow[3][3] = { { 730, 1090, 2440 }, { 270, 2290, 3010 }, { 570, 840, 2410 } };
    for (int i = 0; i < N; ++i)
    {
        const double t = i / sr;
        const int v = ((int) (t / 0.35)) % 3;
        if (v != cur) { cur = v; f1.setBP (sr, vow[v][0], 6); f2.setBP (sr, vow[v][1], 8); f3.setBP (sr, vow[v][2], 10); }
        ph += 200.0 * std::pow (2.0, 0.15 / 12.0 * std::sin (6.28 * 5.0 * t)) / sr;
        if (ph >= 1.0) ph -= 1.0;
        float g = (float) (ph < 0.6 ? std::sin (3.14159 * ph / 0.6) : 0.0); g = g * g - 0.3f;
        x[(size_t) i] = 0.5f * (f1.process (g) + f2.process (g) * 0.6f + f3.process (g) * 0.3f);
    }
    return x;
}

// devolve o ganho (dB) que deixa o preset com nivel parecido com a voz seca
inline float estimateMakeupDb (const float* vals, double sr = 48000.0)
{
    auto e = std::make_unique<Engine>();
    for (int i = 0; i < P_COUNT; ++i) e->setParam (i, vals[i]);
    e->setParam (P_outGain, 0.f); e->setParam (P_safety, 0.f); e->setParam (P_mode, 0.f);
    e->prepare (sr, 512);
    const auto in = referenceVoice (sr, 1.6);
    std::vector<float> l (in.size()), r (in.size());
    for (size_t i = 0; i < in.size(); i += 512)
    {
        const int n = (int) std::min ((size_t) 512, in.size() - i);
        e->process (&in[i], &in[i], nullptr, nullptr, &l[i], &r[i], n, 120.0, 0.0, false);
    }
    double ei = 0, eo = 0; const size_t from = in.size() / 4;
    for (size_t i = from; i < in.size(); ++i) { ei += (double) in[i] * in[i]; eo += 0.5 * ((double) l[i] * l[i] + (double) r[i] * r[i]); }
    if (eo < 1e-12) return 0.f;
    const float d = (float) (10.0 * std::log10 (ei / eo));
    return clampf (std::round (d * 2.f) / 2.f, -9.f, 9.f);
}
} // namespace vfx
