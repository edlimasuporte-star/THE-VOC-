// THE VOC FX - analise de pitch, PSOLA em tempo real (baixa latencia) e synth portador
#pragma once
#include "FxDsp.h"

namespace vfx
{
// ------------------------------------------------------------------
// Rastreador de pitch YIN (sinal decimado ~12 kHz) + marcas de periodo
// ------------------------------------------------------------------
class Tracker
{
public:
    static constexpr int RB = 1 << 16, RM = RB - 1;
    std::vector<float> ring;      // voz mono em taxa cheia
    int64_t n = 0;                // indice absoluto da proxima amostra
    bool voiced = false;
    float T = 0.f;                // periodo em amostras (taxa cheia)
    float midi = 0.f;             // pitch detectado (MIDI float)
    float rms = 0.f;
    int reg = 1;

    void prepare (double fs)
    {
        sr = fs;
        decim = std::max (1, (int) std::lround (fs / 12000.0));
        srD = fs / decim;
        hop = 48 * decim;
        ring.assign (RB, 0.f); dec.assign (DB, 0.f); xb.assign (800, 0.f); dn.assign (420, 0.f);
        rmsA = std::exp (-1.f / (float) (fs * 0.02));
        reset();
    }
    void reset()
    {
        n = 0; decWritten = 0; decAcc = 0.f; decCnt = 0; voiced = false; T = 0.f; midi = 0.f; nT = 0; prevVoiced = false;
        markCount = 0; haveMark = false; lastMark = 0; rms = 0.f; unvoicedRun = 0;
        std::fill (ring.begin(), ring.end(), 0.f); std::fill (dec.begin(), dec.end(), 0.f);
    }

    inline float at (int64_t i) const { return ring[(size_t) (i & RM)]; }
    inline float atInterp (double p) const
    {
        const int64_t i = (int64_t) std::floor (p); const float fr = (float) (p - (double) i);
        const float a = at (i), b = at (i + 1); return a + (b - a) * fr;
    }

    // retorna true quando uma nova analise foi feita
    bool push (float x)
    {
        ring[(size_t) (n & RM)] = x;
        rms = std::sqrt (rmsA * rms * rms + (1.f - rmsA) * x * x);
        decAcc += x;
        if (++decCnt >= decim) { dec[(size_t) (decWritten & DM)] = decAcc / (float) decim; ++decWritten; decAcc = 0.f; decCnt = 0; }
        bool analysed = false;
        if (((n + 1) % hop) == 0) { analyse(); analysed = true; }
        if (voiced) updateMarks();
        ++n;
        return analysed;
    }

    // marca de periodo mais proxima de 'cen' e que termine antes de 'limit'
    bool snapMark (double cen, int64_t limit, int64_t& out) const
    {
        int64_t best = -1; double bd = 1e18;
        for (int64_t k = markCount - 1; k >= 0 && markCount - k <= MN; --k)
        {
            const int64_t mk = marks[k % MN];
            if (mk > limit) continue;
            const double d = std::fabs ((double) mk - cen);
            if (d < bd) { bd = d; best = mk; }
            if ((double) mk < cen - 2.0 * T) break;
        }
        if (best >= 0 && bd < 1.5 * T) { out = best; return true; }
        return false;
    }

private:
    static constexpr int DB = 4096, DM = DB - 1, MN = 512, MAXTAU = 400;
    double sr = 48000.0, srD = 12000.0;
    int decim = 4, hop = 192;
    std::vector<float> dec, xb, dn;
    int64_t decWritten = 0; float decAcc = 0.f; int decCnt = 0;
    float Tb[3] { 0.f, 0.f, 0.f }; int nT = 0; bool prevVoiced = false; int unvoicedRun = 0;
    int64_t marks[MN] {}; int64_t markCount = 0, lastMark = 0; bool haveMark = false;
    float rmsA = 0.999f;

    void pushMark (int64_t m) { marks[markCount % MN] = m; ++markCount; lastMark = m; }
    int64_t argmax (int64_t a, int64_t b) const
    {
        int64_t bi = a; float bv = -1e30f;
        for (int64_t k = a; k <= b; ++k) { const float v = at (k); if (v > bv) { bv = v; bi = k; } }
        return bi;
    }
    void updateMarks()
    {
        if (T < 8.f) return;
        if (haveMark && lastMark < n - (int64_t) (4.f * T)) haveMark = false;
        if (! haveMark)
        {
            if (n - (int64_t) T < 0) return;
            pushMark (argmax (n - (int64_t) T, n)); haveMark = true; return;
        }
        if (lastMark + (int64_t) (1.3f * T) <= n)
            pushMark (argmax (lastMark + (int64_t) (0.7f * T), lastMark + (int64_t) (1.3f * T)));
    }

    void analyse()
    {
        static const float fminT[3] = { 50.f, 75.f, 140.f };
        static const float fmaxT[3] = { 420.f, 800.f, 1400.f };
        const int rg = std::clamp (reg, 0, 2);
        const int minTau = std::max (2, (int) (srD / fmaxT[rg]));
        const int maxTau = std::min ((int) (srD / fminT[rg]) + 2, MAXTAU);
        const int Wn = 240, total = Wn + maxTau;
        bool v = false; float Tin = 0.f;
        if (decWritten >= total)
        {
            for (int k = 0; k < total; ++k) xb[(size_t) k] = dec[(size_t) ((decWritten - total + k) & DM)];
            float e = 0.f;
            for (int k = total - Wn; k < total; ++k) e += xb[(size_t) k] * xb[(size_t) k];
            if (std::sqrt (e / (float) Wn) > 0.0012f)
            {
                const float thr = 0.18f;
                dn[0] = 1.f; float run = 0.f;
                for (int tau = 1; tau < maxTau; ++tau)
                {
                    float d = 0.f;
                    for (int j = 0; j < Wn; ++j) { const float df = xb[(size_t) j] - xb[(size_t) (j + tau)]; d += df * df; }
                    run += d;
                    dn[(size_t) tau] = run > 0.f ? d * (float) tau / run : 1.f;
                }
                int te = -1;
                for (int tau = minTau; tau < maxTau - 1; ++tau)
                    if (dn[(size_t) tau] < thr)
                    {
                        while (tau + 1 < maxTau - 1 && dn[(size_t) (tau + 1)] < dn[(size_t) tau]) ++tau;
                        te = tau; break;
                    }
                if (te > 1 && te < maxTau - 1)
                {
                    const float y0 = dn[(size_t) (te - 1)], y1 = dn[(size_t) te], y2 = dn[(size_t) (te + 1)];
                    const float den = y0 - 2.f * y1 + y2;
                    const float tr = (float) te + (std::fabs (den) > 1e-9f ? 0.5f * (y0 - y2) / den : 0.f);
                    Tin = tr * (float) decim; v = true;
                }
            }
        }
        if (v)
        {
            if (! prevVoiced) nT = 0;
            Tb[nT % 3] = Tin; ++nT;
            if (nT >= 3) Tin = std::max (std::min (Tb[0], Tb[1]), std::min (std::max (Tb[0], Tb[1]), Tb[2]));
            T = Tin; midi = 69.f + 12.f * std::log2 ((float) sr / Tin / 440.f);
            voiced = true; unvoicedRun = 0;
        }
        else
        {
            // tolera 1 quadro sem pitch (evita cortes curtos)
            if (++unvoicedRun > 1) { voiced = false; haveMark = false; }
        }
        prevVoiced = v;
    }
};

// ------------------------------------------------------------------
// PSOLA em tempo real: graos de 1 periodo, so usa passado (latencia ~2 periodos)
// ------------------------------------------------------------------
class PsolaVoice
{
public:
    void prepare (double fs)
    {
        sr = fs; ola.assign (OB, 0.f);
        Tu = (float) (fs * 0.006); maxL = (float) (fs * 0.022);
        reset();
    }
    void reset() { std::fill (ola.begin(), ola.end(), 0.f); nextOut = 0; lagSm = 0.f; started = false; }

    // gera graos necessarios ate o instante n (antes de ler a saida de n)
    void run (const Tracker& tr, int64_t n, float ratio, float ff, float extraLag, float gain)
    {
        if (! started) { nextOut = n; started = true; lagSm = 2.f * Tu; }
        if (nextOut < n - (int64_t) maxL) nextOut = n;
        const bool vo = tr.voiced && tr.T > 8.f;
        const float Tin = vo ? tr.T : Tu;
        const float r = vo ? clampf (ratio, 0.25f, 4.f) : 1.f;
        ff = clampf (ff, 0.5f, 2.f);
        const float L = clampf (Tin / ff, 16.f, maxL);
        int guard = 0;
        while ((double) nextOut - L <= (double) n && guard++ < 16)
        {
            const float Tout = std::max (Tin / r, 16.f);
            const float ext = L * ff;
            const float lagT = 2.f * Tin + 0.0015f * (float) sr + extraLag;
            lagSm += 0.25f * (lagT - lagSm);
            const double s = (double) nextOut - (double) lagSm;
            const int64_t limit = n - (int64_t) std::ceil (ext) - 2;
            int64_t c;
            if (! (vo && tr.snapMark (s, limit, c))) c = std::min ((int64_t) std::llround (s), limit);
            if (gain > 0.f)
            {
                const float g = gain * Tout / L, inv = 1.f / (2.f * L);
                const int Li = (int) L;
                const HannTable& H = hann();
                for (int j = -Li; j <= Li; ++j)
                {
                    const int64_t oi = nextOut + j;
                    if (oi < n) continue;
                    const float w = H.at (((float) j + L) * inv);
                    const float x = tr.atInterp ((double) c + (double) j * ff);
                    ola[(size_t) (oi & OM)] += w * x * g;
                }
            }
            nextOut += (int64_t) std::lround (Tout);
        }
    }
    inline float out (int64_t n) { float& s = ola[(size_t) (n & OM)]; const float y = s; s = 0.f; return y; }

private:
    static constexpr int OB = 1 << 15, OM = OB - 1;
    std::vector<float> ola;
    double sr = 48000.0;
    int64_t nextOut = 0; float lagSm = 0.f, Tu = 288.f, maxL = 1000.f; bool started = false;
};

// ------------------------------------------------------------------
// Synth portador: ate 8 notas, 10 receitas de timbre
// ------------------------------------------------------------------
enum Wave { W_SAW = 0, W_SQUARE, W_SYNC, W_TRI, W_SAWS, W_NONE };
struct Recipe { int w1; float pw1; int w2; float semi2, cents2, g2, noise, detune, sync; };
static const Recipe kRecipes[10] = {
    { W_SAW,    0.5f,  W_SAW,     0.f,  8.f, 0.60f, 0.00f,  0.f, 2.f },   // Classic Saw
    { W_SQUARE, 0.5f,  W_SQUARE,-12.f,  0.f, 0.45f, 0.00f,  0.f, 2.f },   // Square
    { W_SAWS,   0.5f,  W_NONE,    0.f,  0.f, 0.00f, 0.00f, 18.f, 2.f },   // Super Saws
    { W_SYNC,   0.5f,  W_SAW,   -12.f,  0.f, 0.30f, 0.00f,  0.f, 2.4f },  // Sync Lead
    { W_TRI,    0.5f,  W_TRI,    12.f,  0.f, 0.25f, 0.03f,  0.f, 2.f },   // Soft Tri
    { W_SAW,    0.5f,  W_SQUARE,-12.f,  0.f, 0.70f, 0.00f,  0.f, 2.f },   // Saw + Sub
    { W_SQUARE, 0.22f, W_SAW,    12.f,  0.f, 0.20f, 0.00f,  0.f, 2.f },   // Hollow Pulse
    { W_SAW,    0.5f,  W_NONE,    0.f,  0.f, 0.00f, 0.45f,  0.f, 2.f },   // Air Noise
    { W_SQUARE, 0.5f,  W_TRI,    12.f,  0.f, 0.55f, 0.00f,  0.f, 2.f },   // Organ
    { W_SAWS,   0.5f,  W_SAW,   -12.f,  0.f, 0.50f, 0.00f, 10.f, 2.f },   // 80s Brass
};

struct NoteSlot { bool gate = false; float note = 60.f, target = 60.f, env = 0.f, level = 1.f; int64_t age = 0; int midiNote = -1; };

inline float polyBlep (float t, float dt)
{
    if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
    if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
    return 0.f;
}

class OscBank
{
public:
    static constexpr int NS = 8;
    void reset() { for (auto& p : ph) for (auto& x : p) x = 0.f; }
    void seed (uint32_t s) { rng = Rng (s); for (int i = 0; i < NS; ++i) for (int k = 0; k < 10; ++k) ph[i][k] = rng.uni(); }

    // inc = freq/sr de cada nota; amp = envelope*nivel
    float render (const Recipe& R, const float* inc, const float* amp, int count)
    {
        float sum = 0.f;
        float df[5];
        static const float off[5] = { -1.f, -0.5f, 0.f, 0.5f, 1.f };
        for (int k = 0; k < 5; ++k) df[k] = std::exp2 (off[k] * R.detune * 0.01f / 12.f);
        const float f2 = std::exp2 ((R.semi2 + R.cents2 * 0.01f) / 12.f);
        for (int i = 0; i < count; ++i)
        {
            if (amp[i] <= 1e-5f) continue;
            float* p = ph[i];
            float s = osc (R.w1, R.pw1, df, R.sync, inc[i], p);
            if (R.w2 != W_NONE) s += R.g2 * osc (R.w2, 0.5f, df, R.sync, inc[i] * f2, p + 5);
            if (R.noise > 0.f) s += R.noise * 1.7f * rng.next();
            sum += s * amp[i];
        }
        return sum;
    }

private:
    float ph[NS][10] {};
    Rng rng { 777u };

    static inline float adv (float& p, float inc) { p += inc; if (p >= 1.f) p -= std::floor (p); return p; }

    float osc (int w, float pw, const float* df, float syncR, float inc, float* p)
    {
        inc = clampf (inc, 0.f, 0.45f);
        switch (w)
        {
            case W_SAW: { const float t = adv (p[0], inc); return 2.f * t - 1.f - polyBlep (t, inc); }
            case W_SQUARE:
            {
                const float t = adv (p[0], inc);
                float v = t < pw ? 1.f : -1.f;
                v += polyBlep (t, inc);
                float t2 = t + 1.f - pw; t2 -= std::floor (t2);
                v -= polyBlep (t2, inc);
                return v * 0.8f;
            }
            case W_TRI: { const float t = adv (p[0], inc); return 2.f * std::fabs (2.f * t - 1.f) - 1.f; }
            case W_SYNC:
            {
                const float before = p[0];
                const float t = adv (p[0], inc);
                if (t < before) p[1] = t * syncR; // reset do escravo
                const float s = adv (p[1], inc * syncR);
                return 2.f * s - 1.f;
            }
            case W_SAWS:
            {
                float s = 0.f;
                for (int k = 0; k < 5; ++k)
                {
                    const float ik = clampf (inc * df[k], 0.f, 0.45f);
                    const float t = adv (p[k], ik);
                    s += 2.f * t - 1.f - polyBlep (t, ik);
                }
                return s * 0.45f;
            }
            default: return 0.f;
        }
    }
};
} // namespace vfx
