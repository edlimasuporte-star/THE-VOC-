// THE VOC FX - modulos de sintese vocal: VOCODER, TRACT, ROBOT, TALKBOX (LPC)
#pragma once
#include "FxVoice.h"

namespace vfx
{
// ------------------------------------------------------------------
// VOCODER de bancos de filtros (3 modos)
// ------------------------------------------------------------------
class Vocoder
{
public:
    void prepare (double fs)
    {
        sr = fs; sib1.setHP (fs, 5500.f, 0.7f); sib2.setHP (fs, 5500.f, 0.7f);
        envA = std::exp (-1.f / (float) (fs * 0.0015));
        carA = std::exp (-1.f / (float) (fs * 0.012));
        curMode = -1; curShift = 999.f; reset();
    }
    void reset()
    {
        for (int b = 0; b < MAXB; ++b) { for (auto& f : m[b]) f.reset(); for (auto& f : c[b]) f.reset(); envM[b] = envC[b] = 0.f; }
        sib1.reset(); sib2.reset();
    }
    void setParams (int mode, float shift, float scale)
    {
        if (mode != curMode || std::fabs (shift - curShift) > 0.05f) design (mode, shift);
        const float relMs = 6.f + 220.f * (1.f - scale) * (1.f - scale);
        relA = std::exp (-1.f / (float) (sr * relMs * 0.001));
    }
    inline float process (float voice, float carrier)
    {
        float out = 0.f;
        for (int b = 0; b < nb; ++b)
        {
            float xm = m[b][0].process (voice), xc = c[b][0].process (carrier);
            if (stages == 2) { xm = m[b][1].process (xm); xc = c[b][1].process (xc); }
            const float am = std::fabs (xm), ac = std::fabs (xc);
            envM[b] = am > envM[b] ? envA * envM[b] + (1.f - envA) * am : relA * envM[b] + (1.f - relA) * am;
            envC[b] = carA * envC[b] + (1.f - carA) * ac;
            out += xc * envM[b] / (envC[b] + 2.0e-4f);
        }
        out *= makeup;
        out += 0.35f * sib2.process (sib1.process (voice));
        return flushDenormal (out);
    }

private:
    static constexpr int MAXB = 32;
    double sr = 48000.0;
    Biquad m[MAXB][2], c[MAXB][2], sib1, sib2;
    float envM[MAXB] {}, envC[MAXB] {};
    int nb = 20, stages = 1, curMode = -1; float curShift = 0.f;
    float envA = 0.9f, relA = 0.99f, carA = 0.99f, makeup = 1.f;

    void design (int mode, float shift)
    {
        curMode = mode; curShift = shift;
        static const int nbs[3] = { 20, 12, 32 };
        static const float qs[3] = { 4.f, 3.2f, 5.5f };
        nb = nbs[std::clamp (mode, 0, 2)]; stages = mode == 2 ? 2 : 1;
        const float q = qs[std::clamp (mode, 0, 2)];
        const float lo = 90.f, hi = std::min (11000.f, (float) sr * 0.42f);
        const float sh = std::exp2 (shift);           // shift em oitavas (-1..1)
        for (int b = 0; b < nb; ++b)
        {
            const float f = lo * std::pow (hi / lo, (float) b / (float) (nb - 1));
            for (int s = 0; s < stages; ++s) { m[b][s].setBP (sr, f, q); c[b][s].setBP (sr, clampf (f * sh, 40.f, (float) sr * 0.45f), q); }
        }
        makeup = 2.2f / std::sqrt ((float) nb) * (stages == 2 ? 1.6f : 1.f);
    }
};

// ------------------------------------------------------------------
// Nucleo LPC: analise por autocorrelacao (Levinson) + sintese em treliça
// Formant shift: a janela de analise e lida com passo 'ff' (reamostragem)
// ------------------------------------------------------------------
class LpcCore
{
public:
    static constexpr int MAXP = 48;
    float kCur[MAXP] {}, kTgt[MAXP] {}, kStep[MAXP] {};
    float eCur = 0.f, eTgt = 0.f, eStep = 0.f;
    int order = 16;

    void prepare (double fs) { sr = fs; win.assign ((size_t) (fs * 0.06) + 16, 0.f); reset(); }
    void reset()
    {
        for (int i = 0; i < MAXP; ++i) { kCur[i] = kTgt[i] = kStep[i] = 0.f; g[i] = 0.f; }
        g[MAXP] = 0.f; eCur = eTgt = eStep = 0.f; cnt = 0; pe = 0.f; deY = 0.f;
    }
    void configure (int ord, float winSec, float hopSec, float preEmph)
    {
        order = std::clamp (ord, 4, MAXP);
        W = std::clamp ((int) (sr * winSec), 64, (int) win.size() / 2 - 8);
        hop = std::max (16, (int) (sr * hopSec));
        pre = preEmph;
        peA = std::exp (-1.f / (float) (sr * 0.015));
    }

    // chamada por amostra: analisa quando completa um hop
    void tick (const Tracker& tr, float ff, bool hold, float holdDecay)
    {
        if (++cnt >= hop)
        {
            cnt = 0;
            if (hold) { eTgt *= holdDecay; for (int i = 0; i < order; ++i) kTgt[i] = kCur[i] + kStep[i] * 0.f; }
            else analyse (tr, ff);
            const float inv = 1.f / (float) hop;
            for (int i = 0; i < order; ++i) kStep[i] = (kTgt[i] - kCur[i]) * inv;
            eStep = (eTgt - eCur) * inv;
        }
        for (int i = 0; i < order; ++i) kCur[i] += kStep[i];
        eCur += eStep;
    }
    float lastE() const { return eTgt; }

    // excitacao -> envelope espectral da voz (scaleK < 1 achata o envelope)
    inline float synth (float exc, float scaleK, float deEmph)
    {
        pe = peA * pe + (1.f - peA) * exc * exc;
        const float x = exc / std::sqrt (pe + 1.0e-6f) * std::sqrt (std::max (eCur, 0.f));
        float f = x;
        for (int mI = order; mI >= 1; --mI)
        {
            const float k = kCur[mI - 1] * scaleK;
            f = f - k * g[mI - 1];
            gN[mI] = k * f + g[mI - 1];
        }
        gN[0] = f;
        for (int i = 0; i <= order; ++i) g[i] = flushDenormal (gN[i]);
        if (! std::isfinite (f)) { reset(); return 0.f; }
        deY = f + deEmph * deY;
        deY = flushDenormal (deY);
        return deY;
    }

private:
    double sr = 48000.0;
    std::vector<float> win;
    int W = 1024, hop = 96, cnt = 0;
    float pre = 0.9f, peA = 0.999f, pe = 0.f, deY = 0.f;
    float g[MAXP + 1] {}, gN[MAXP + 1] {};
    double r[MAXP + 1] {}, a[MAXP + 1] {}, at[MAXP + 1] {};

    void analyse (const Tracker& tr, float ff)
    {
        ff = clampf (ff, 0.5f, 2.f);
        const int64_t end = tr.n - 1;
        const double start = (double) end - (double) W * ff;
        const HannTable& H = hann();
        double wsum = 0.0;
        float prev = (float) tr.atInterp (start - ff);
        for (int i = 0; i < W; ++i)
        {
            const float s = tr.atInterp (start + (double) i * ff);
            const float w = H.at (((float) i + 0.5f) / (float) W);
            win[(size_t) i] = (s - pre * prev) * w;
            prev = s;
            wsum += (double) w * w;
        }
        for (int l = 0; l <= order; ++l)
        {
            double acc = 0.0;
            for (int i = 0; i + l < W; ++i) acc += (double) win[(size_t) i] * win[(size_t) (i + l)];
            r[l] = acc / std::max (wsum, 1e-9);
        }
        if (r[0] < 1.0e-10) { eTgt = 0.f; for (int i = 0; i < order; ++i) kTgt[i] = 0.f; return; }
        r[0] *= 1.0001; r[0] += 1.0e-12;
        for (int l = 1; l <= order; ++l) { const double lw = std::exp (-0.5 * std::pow (0.0035 * l * 2.0 * kPi * 0.25, 2.0)); r[l] *= lw; }
        // Levinson-Durbin, A(z) = 1 + sum a_i z^-i
        double E = r[0]; a[0] = 1.0;
        for (int i = 1; i <= order; ++i) a[i] = 0.0;
        for (int i = 1; i <= order; ++i)
        {
            double acc = r[i];
            for (int j = 1; j < i; ++j) acc += a[j] * r[i - j];
            double k = -acc / std::max (E, 1e-12);
            k = std::clamp (k, -0.995, 0.995);
            for (int j = 0; j <= i; ++j) at[j] = a[j];
            for (int j = 1; j < i; ++j) a[j] = at[j] + k * at[i - j];
            a[i] = k;
            kTgt[i - 1] = (float) k;
            E *= (1.0 - k * k);
        }
        eTgt = (float) std::max (E, 0.0);
    }
};

// ------------------------------------------------------------------
// TRACT: trato vocal sobre o synth (Clarity, Shift, Nasal, Breath)
// ------------------------------------------------------------------
class TractModule
{
public:
    void prepare (double fs)
    {
        sr = fs; lpc.prepare (fs);
        lpc.configure ((int) std::lround (fs / 2000.0), 0.025f, 0.002f, 0.9f);
        vA = std::exp (-1.f / (float) (fs * 0.01)); reset();
    }
    void reset() { lpc.reset(); nas1.reset(); nas2.reset(); vEnv = 0.f; lastNasal = -1.f; }
    float process (const Tracker& tr, float voice, float carrier, float clarity, float shift, float nasal, float breath)
    {
        if (std::fabs (nasal - lastNasal) > 0.002f)
        {
            lastNasal = nasal;
            nas1.setPeak (sr, 270.f, 1.2f, nasal * 9.f);
            nas2.setPeak (sr, 1100.f, 1.6f, -nasal * 15.f);
        }
        lpc.tick (tr, std::exp2 (shift * 0.6f), false, 1.f);
        vEnv = vA * vEnv + (1.f - vA) * std::fabs (voice);
        const float exc = carrierNorm (carrier) * (1.f - breath) + rng.next() * 1.7f * breath;
        float y = lpc.synth (exc, 0.35f + 0.65f * clarity, 0.9f) * (0.45f + 0.55f * clarity);
        y += (1.f - clarity) * 0.9f * cnLast * vEnv;
        return 2.2f * nas2.process (nas1.process (y));
    }

private:
    double sr = 48000.0; LpcCore lpc; Biquad nas1, nas2; Rng rng { 4242u };
    float vEnv = 0.f, vA = 0.99f, lastNasal = -1.f, cp = 0.f;
    float cnLast = 0.f;
    float carrierNorm (float c) { cp = 0.999f * cp + 0.001f * c * c; cnLast = c / std::sqrt (cp + 1e-6f); return cnLast; }
};

// ------------------------------------------------------------------
// ROBOT: LPC classico (Bits, Bytes, Bats, modos Spell/Read/Math)
// ------------------------------------------------------------------
class RobotModule
{
public:
    void prepare (double fs) { sr = fs; lpc.prepare (fs); curMode = -1; reset(); }
    void reset() { lpc.reset(); holdLeft = 0; heldE = 0.f; zoh = 0.f; zohAcc = 0.f; gravel = 0.f; gravelT = 0.f; }
    float process (const Tracker& tr, float carrier, int mode, float bits, float bytes, float bats)
    {
        if (mode != curMode)
        {
            curMode = mode;
            if (mode == 0)      lpc.configure ((int) std::lround (sr / 3000.0), 0.020f, 0.004f, 0.94f);
            else if (mode == 1) lpc.configure ((int) std::lround (sr / 2000.0), 0.012f, 0.0025f, 0.97f);
            else                lpc.configure ((int) std::lround (sr / 5000.0), 0.040f, 0.005f, 0.90f);
        }
        // BYTES: segura o quadro de vogal quando a voz cai (prolonga vogais)
        const float pw = tr.rms * tr.rms;
        bool hold = false;
        if (bytes > 0.01f)
        {
            if (pw > heldE * 0.3f) { heldE = pw; holdLeft = (int) (sr * bytes * 0.45f); }
            else if (holdLeft > 0 && heldE > 1.0e-7f) { hold = true; --holdLeft; heldE *= 0.99995f; }
            else heldE = pw;
        }
        lpc.tick (tr, 1.f, hold, hold ? 0.985f : 1.f);

        // BATS: ruido + cascalho (modulacao aleatoria de amplitude)
        if (--gravelT <= 0.f) { gravelTarget = rng.uni(); gravelT = (float) (sr * (0.004 + 0.02 * rng.uni())); }
        gravel += 0.01f * (gravelTarget - gravel);
        float exc = carrier * (1.f + bats * 1.6f * (gravel - 0.5f)) + bats * 0.9f * rng.next();
        float y = lpc.synth (exc, 1.f, 0.6f) * 0.65f;

        // BITS: amostragem com retencao de ordem zero + reducao de bits
        const float holdLen = 1.f + bits * bits * 38.f;
        zohAcc += 1.f;
        if (zohAcc >= holdLen) { zohAcc -= holdLen; zoh = y; }
        if (bits <= 0.001f) zoh = y;
        float q = zoh;
        if (bits > 0.4f) { const float lv = std::exp2 (16.f - (bits - 0.4f) * 20.f); q = std::round (q * lv) / lv; }
        return q;
    }

private:
    double sr = 48000.0; LpcCore lpc; Rng rng { 99u };
    int curMode = -1, holdLeft = 0; float heldE = 0.f, zoh = 0.f, zohAcc = 0.f, gravel = 0.f, gravelT = 0.f, gravelTarget = 0.5f;
};

// ------------------------------------------------------------------
// TALKBOX: Drive -> alto-falante -> filtro LPC da boca (formant ajustavel)
// ------------------------------------------------------------------
class TalkboxModule
{
public:
    void prepare (double fs)
    {
        sr = fs; lpc.prepare (fs); curMode = -1;
        spk1.setHP (fs, 160.f, 0.7f); spk2.setPeak (fs, 1250.f, 0.8f, 6.f); spk3.setLP (fs, 4500.f, 0.7f);
        tube.prepare ((int) (fs * 0.01)); sib1.setHP (fs, 5000.f, 0.7f); sib2.setHP (fs, 5000.f, 0.7f);
        reset();
    }
    void reset() { lpc.reset(); spk1.reset(); spk2.reset(); spk3.reset(); post.reset(); tube.reset(); tubeY = 0.f; sib1.reset(); sib2.reset(); }
    float process (const Tracker& tr, float voice, float carrier, int mode, float drive, float speaker, float formant)
    {
        if (mode != curMode)
        {
            curMode = mode;
            const float pe[3] = { 0.80f, 0.93f, 0.97f };
            lpc.configure ((int) std::lround (sr / 1500.0), 0.030f, 0.002f, pe[std::clamp (mode, 0, 2)]);
            dem = pe[std::clamp (mode, 0, 2)] * 0.92f;
            if (mode == 0) post.setLP (sr, 2800.f, 0.6f);
            else if (mode == 2) post.setShelf (sr, 3000.f, 4.5f, true);
            else post.setIdentity();
        }
        lpc.tick (tr, std::exp2 (formant * 0.5f), false, 1.f);
        const float pg = 1.f + drive * 14.f;
        float x = std::tanh (carrier * pg) / std::tanh (pg) * (1.f + drive);
        float s = spk3.process (spk2.process (spk1.process (x)));
        tubeY = s + 0.42f * tube.read ((float) (sr * 0.0013));
        tube.push (flushDenormal (tubeY));
        x = x + (tubeY * 0.8f - x) * speaker;
        float y = lpc.synth (x, 1.f, dem) * 0.27f;
        y = post.process (y);
        return y + 0.22f * sib2.process (sib1.process (voice));
    }

private:
    double sr = 48000.0; LpcCore lpc; Biquad spk1, spk2, spk3, post, sib1, sib2; DelayLine tube;
    float tubeY = 0.f, dem = 0.85f; int curMode = -1;
};
} // namespace vfx
