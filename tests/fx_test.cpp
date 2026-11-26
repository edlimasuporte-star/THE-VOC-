// Teste offline do motor THE VOC FX (sem JUCE):
//   c++ -O2 -std=c++17 -I Source -o fx_test tests/fx_test.cpp && ./fx_test
#include "FxEngine.h"
#include "FxPresets.h"
#include "FxLevel.h"
#include <cstdio>
#include <chrono>
#include <memory>

using namespace vfx;
static const double SR = 48000.0;
static int fails = 0;
#define CHECK(c, ...) do { if (!(c)) { ++fails; std::printf ("  FALHOU: "); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

// voz sintetica: trem de pulsos glotais -> formantes, com vibrato e vogais alternadas
static std::vector<float> makeVoice (double seconds, float f0, bool gaps = true)
{
    const int N = (int) (SR * seconds);
    std::vector<float> x ((size_t) N);
    Biquad f1, f2, f3; double ph = 0.0; Rng r (5);
    const float vow[3][3] = { { 730, 1090, 2440 }, { 270, 2290, 3010 }, { 570, 840, 2410 } };
    int cur = -1;
    for (int i = 0; i < N; ++i)
    {
        const double t = i / SR;
        const int v = ((int) (t / 0.4)) % 3;
        if (v != cur) { cur = v; f1.setBP (SR, vow[v][0], 6); f2.setBP (SR, vow[v][1], 8); f3.setBP (SR, vow[v][2], 10); }
        const double f = f0 * std::pow (2.0, 0.15 / 12.0 * std::sin (6.28 * 5.0 * t));
        ph += f / SR; float g = 0.f;
        if (ph >= 1.0) { ph -= 1.0; }
        g = (float) (ph < 0.6 ? std::sin (3.14159 * ph / 0.6) : 0.0); g = g * g - 0.3f;
        float s = f1.process (g) * 1.0f + f2.process (g) * 0.6f + f3.process (g) * 0.3f;
        if (gaps && std::fmod (t, 1.2) > 1.05) s = 0.03f * r.next();      // consoante / pausa
        x[(size_t) i] = s * 0.5f;
    }
    return x;
}

static float rmsDb (const std::vector<float>& a, int from, int to)
{
    double e = 0; for (int i = from; i < to; ++i) e += (double) a[(size_t) i] * a[(size_t) i];
    return (float) (10.0 * std::log10 (e / std::max (1, to - from) + 1e-20));
}

// pitch medio (YIN simples) de um trecho
static float measureF0 (const std::vector<float>& a, int from, int to)
{
    std::vector<float> f0s;
    for (int s = from; s + 2400 < to; s += 1200)
    {
        const int W = 1200, maxT = 1000; std::vector<double> d (maxT, 0.0), dn (maxT, 1.0); double run = 0;
        for (int tau = 1; tau < maxT; ++tau) { double acc = 0; for (int j = 0; j < W; ++j) { const double df = a[(size_t) (s + j)] - a[(size_t) (s + j + tau)]; acc += df * df; } run += acc; dn[(size_t) tau] = run > 0 ? acc * tau / run : 1; }
        for (int tau = 40; tau < maxT - 1; ++tau) if (dn[(size_t) tau] < 0.15) { while (tau + 1 < maxT && dn[(size_t) tau + 1] < dn[(size_t) tau]) ++tau; f0s.push_back ((float) (SR / tau)); break; }
    }
    if (f0s.empty()) return 0.f;
    std::sort (f0s.begin(), f0s.end()); return f0s[f0s.size() / 2];
}

struct Run { std::vector<float> l, r; double secs; };
static Run run (Engine& e, const std::vector<float>& in, const std::vector<float>* sc = nullptr)
{
    Run o; o.l.assign (in.size(), 0.f); o.r.assign (in.size(), 0.f);
    const int B = 256; auto t0 = std::chrono::steady_clock::now();
    for (size_t i = 0; i < in.size(); i += B)
    {
        const int n = (int) std::min ((size_t) B, in.size() - i);
        e.process (&in[i], &in[i], sc ? &(*sc)[i] : nullptr, sc ? &(*sc)[i] : nullptr, &o.l[i], &o.r[i], n, 120.0, (double) i / SR * 2.0, true);
    }
    o.secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
    return o;
}

static bool finite (const Run& r) { for (size_t i = 0; i < r.l.size(); ++i) if (! std::isfinite (r.l[i]) || ! std::isfinite (r.r[i]) || std::fabs (r.l[i]) > 4.f) return false; return true; }

static void setOnly (Engine& e, std::initializer_list<std::pair<int, float>> kv)
{
    for (int i = 0; i < P_COUNT; ++i) e.setParam (i, kParams[i].def);
    for (auto& p : kv) e.setParam (p.first, p.second);
}

int main()
{
    auto voice = makeVoice (6.0, 220.f);
    const int N = (int) voice.size();
    const float inDb = rmsDb (voice, 0, N);
    std::printf ("entrada: %.1f dB RMS\n", inDb);
    auto eng = std::make_unique<Engine>(); Engine& e = *eng;

    // 1) cada modulo sozinho
    struct M { const char* name; int on; int lvl; } mods[] = {
        { "TRACT", P_trOn, P_trLvl }, { "VOCODER", P_vcOn, P_vcLvl }, { "ROBOT", P_rbOn, P_rbLvl }, { "TALKBOX", P_tbOn, P_tbLvl }, { "HARMONY", P_hmOn, P_hmLvl } };
    for (auto& m : mods)
    {
        setOnly (e, { { P_voiceLvl, -60.f }, { m.on, 1.f }, { m.lvl, 0.f }, { P_autoInt1, 11.f } });
        e.prepare (SR, 256);
        auto r = run (e, voice);
        const float db = rmsDb (r.l, N / 6, N);
        std::printf ("%-8s saida %.1f dB (dif %.1f)  CPU %.2f%%\n", m.name, db, db - inDb, 100.0 * r.secs / 6.0);
        CHECK (finite (r), "%s gerou NaN/estouro", m.name);
        CHECK (db > inDb - 18.f && db < inDb + 12.f, "%s nivel fora da faixa", m.name);
    }

    // 2) HARMONY +Oct (sem voz seca): pitch de saida deve ser ~440 Hz
    {
        auto steady = makeVoice (3.0, 220.f, false);
        setOnly (e, { { P_voiceLvl, -60.f }, { P_hmOn, 1.f }, { P_hmLvl, 0.f }, { P_autoInt1, 16.f }, { P_hmHuman, 0.f }, { P_hmWidth, 0.f } });
        e.prepare (SR, 256);
        auto r = run (e, steady);
        const float f = measureF0 (r.l, 24000, (int) steady.size());
        std::printf ("HARMONY +Oct: %.1f Hz (esperado 440)\n", f);
        CHECK (std::fabs (f - 440.f) < 12.f, "harmonia de oitava errada");
        setOnly (e, { { P_voiceLvl, -60.f }, { P_hmOn, 1.f }, { P_hmLvl, 0.f }, { P_autoInt1, 13.f }, { P_hmHuman, 0.f }, { P_key, 9.f }, { P_scale, 0.f } });
        e.prepare (SR, 256);
        auto r2 = run (e, steady);
        const float f2 = measureF0 (r2.l, 24000, (int) steady.size());
        std::printf ("HARMONY +5th em A maior: %.1f Hz (esperado 329.6)\n", f2);
        CHECK (std::fabs (f2 - 329.6f) < 10.f, "harmonia de quinta errada");
    }

    // 3) Correcao: 212 Hz em C maior -> A3 (220 Hz)
    {
        auto off = makeVoice (3.0, 212.f, false);
        setOnly (e, { { P_corrOn, 1.f }, { P_corrSpeed, 0.f }, { P_corrStr, 100.f }, { P_key, 0.f }, { P_scale, 0.f } });
        e.prepare (SR, 256);
        auto r = run (e, off);
        const float f = measureF0 (r.l, 24000, (int) off.size());
        std::printf ("CORRECAO 212 Hz -> %.1f Hz (esperado 220)\n", f);
        CHECK (std::fabs (f - 220.f) < 4.f, "correcao de afinacao errada");
    }

    // 4) MIDI e SIDECHAIN
    {
        setOnly (e, { { P_mode, 1.f }, { P_vcOn, 1.f }, { P_voiceLvl, -60.f } });
        e.prepare (SR, 256);
        e.noteOn (48, 100); e.noteOn (52, 100); e.noteOn (55, 100);
        auto r = run (e, voice);
        std::printf ("MIDI vocoder (acorde C): %.1f dB\n", rmsDb (r.l, N / 6, N));
        CHECK (finite (r) && rmsDb (r.l, N / 6, N) > inDb - 20.f, "modo MIDI sem saida");
        std::vector<float> sc ((size_t) N); double ph = 0; for (int i = 0; i < N; ++i) { ph += 110.0 / SR; ph -= std::floor (ph); sc[(size_t) i] = (float) (ph * 2 - 1) * 0.3f; }
        setOnly (e, { { P_mode, 2.f }, { P_vcOn, 1.f }, { P_tbOn, 1.f }, { P_voiceLvl, -60.f } });
        e.prepare (SR, 256);
        auto r2 = run (e, voice, &sc);
        std::printf ("SIDECHAIN vocoder+talkbox: %.1f dB\n", rmsDb (r2.l, N / 6, N));
        CHECK (finite (r2) && rmsDb (r2.l, N / 6, N) > inDb - 20.f, "sidechain sem saida");
    }

    // 5) Tudo ligado (pior caso de CPU) + todos os presets de fabrica
    {
        setOnly (e, { { P_trOn, 1 }, { P_vcOn, 1 }, { P_vcMode, 2 }, { P_rbOn, 1 }, { P_tbOn, 1 }, { P_hmOn, 1 }, { P_autoInt1, 11 }, { P_autoInt2, 13 }, { P_autoInt3, 2 },
                      { P_corrOn, 1 }, { P_dsOn, 1 }, { P_rmOn, 1 }, { P_flOn, 1 }, { P_tfOn, 1 }, { P_shOn, 1 }, { P_shSeq, 7 }, { P_chOn, 1 }, { P_dlOn, 1 } });
        e.prepare (SR, 256);
        auto r = run (e, voice);
        std::printf ("TUDO LIGADO: %.1f dB, CPU %.1f%% de um nucleo\n", rmsDb (r.l, N / 6, N), 100.0 * r.secs / 6.0);
        CHECK (finite (r), "tudo ligado gerou NaN");
        CHECK (r.secs / 6.0 < 0.5, "CPU alta demais");
    }
    {
        int np = 0; float worst = -200.f, quiet = 200.f; const char* qn = ""; const char* wn = "";
        for (int k = 0; k < numFactoryPresets(); ++k)
        {
            float vals[P_COUNT]; factoryPresetValues (k, vals);
            for (int i = 0; i < P_COUNT; ++i) e.setParam (i, vals[i]);
            e.prepare (SR, 256);
            auto r = run (e, voice);
            const float db = rmsDb (r.l, N / 6, N);
            if (! finite (r)) { ++fails; std::printf ("  FALHOU preset %s NaN\n", kFactory[k].name); }
            if (db > worst) { worst = db; wn = kFactory[k].name; }
            if (db < quiet) { quiet = db; qn = kFactory[k].name; }
            ++np;
        }
        std::printf ("%d presets OK. mais alto: %s %.1f dB | mais baixo: %s %.1f dB\n", np, wn, worst, qn, quiet);
        CHECK (worst < inDb + 12.f && quiet > inDb - 22.f, "preset com nivel desequilibrado");
        Rng rr (11); int bad = 0;
        for (int k = 0; k < 40; ++k)
        {
            float vals[P_COUNT]; std::string nm; inspire (rr.s ^ (uint32_t) (k * 7919), vals, nm);
            vals[P_outGain] = estimateMakeupDb (vals);
            for (int i = 0; i < P_COUNT; ++i) e.setParam (i, vals[i]);
            e.prepare (SR, 256);
            auto r = run (e, voice);
            if (! finite (r)) ++bad;
            const float db = rmsDb (r.l, N / 6, N);
            if (k < 6) std::printf ("INSPIRE %-22s %.1f dB\n", nm.c_str(), db);
            if (db < inDb - 6.f || db > inDb + 6.f) { ++bad; std::printf ("  quieto: %s %.1f dB\n", nm.c_str(), db); }
        }
        CHECK (bad == 0, "%d presets INSPIRE ruins", bad);
    }
    std::printf (fails ? "\n%d FALHA(S)\n" : "\nTODOS OS TESTES PASSARAM\n", fails);
    return fails ? 1 : 0;
}
