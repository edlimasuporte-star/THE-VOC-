// THE VOC FX - cadeia de efeitos (estereo): Distort, Ring Mod, Filter, Transform, Shred, Chorus, Delay
#pragma once
#include "FxDsp.h"

namespace vfx
{
struct Stereo { float l, r; };

class DistortFx
{
public:
    void prepare (double fs) { sr = fs; for (int c = 0; c < 2; ++c) { dc[c].prepare (fs); dc[c].reset(); } reset(); }
    void reset() { for (int c = 0; c < 2; ++c) { lp1[c] = lp2[c] = 0.f; dc[c].reset(); } }
    void set (float drive, float tone, int type)
    {
        pre = std::pow (40.f, drive); typ = type;
        const float fc = 600.f * std::exp2 (tone * 5.f);
        a = 1.f - std::exp (-kTwoPi * fc / (float) sr);
        makeup = 1.f / (0.45f + 0.55f * std::sqrt (pre));
    }
    Stereo process (Stereo x)
    {
        float in[2] = { x.l, x.r };
        for (int c = 0; c < 2; ++c)
        {
            const float z = in[c] * pre;
            float y;
            switch (typ)
            {
                case 0:  y = std::tanh (z + 0.25f) - 0.2449f; break;                       // WARM (assimetrico)
                case 1:  y = 0.6366f * std::atan (z * 1.3f); break;                         // ANALOG
                case 2:  y = 0.6f * std::sin (z) + 0.4f * clampf (z, -1.f, 1.f); break;     // EDGY (dobra)
                default: { const float f = (z >= 0.f ? 1.f : -1.f) * (1.f - std::exp (-std::fabs (z) * 1.5f)); y = f + 0.35f * std::fabs (f); } // FUZZ
            }
            y = dc[c].process (y);
            lp1[c] += a * (y - lp1[c]); lp2[c] += a * (lp1[c] - lp2[c]);
            in[c] = flushDenormal (lp2[c]) * makeup * 1.6f;
        }
        return { in[0], in[1] };
    }

private:
    double sr = 48000.0; float pre = 1.f, a = 1.f, makeup = 1.f; int typ = 0;
    float lp1[2] {}, lp2[2] {}; DcBlock dc[2];
};

class RingModFx
{
public:
    void prepare (double fs) { sr = fs; reset(); }
    void reset() { ph = 0.0; lph = 0.0; }
    void set (float freqHz, float lfoHz, float depth) { f = freqHz; lf = lfoHz; dp = depth; }
    Stereo process (Stereo x)
    {
        lph += lf / sr; if (lph >= 1.0) lph -= 1.0;
        const float tri = 4.f * std::fabs ((float) lph - 0.5f) - 1.f;
        const float fr = f * std::exp2 (dp * 2.f * tri);
        ph += fr / sr; if (ph >= 1.0) ph -= std::floor (ph);
        const float m = std::sin (kTwoPi * (float) ph);
        return { x.l * m, x.r * m };
    }

private:
    double sr = 48000.0, ph = 0.0, lph = 0.0; float f = 220.f, lf = 0.f, dp = 0.f;
};

class FilterFx
{
public:
    void prepare (double fs) { sr = fs; reset(); }
    void reset() { for (auto& s : svf) s.reset(); for (auto& s : svf2) s.reset(); posSm = -1.f; cnt = 0; }
    void set (float pos01, float res, int type) { tgtPos = pos01; rs = res; typ = type; }
    Stereo process (Stereo x)
    {
        if (posSm < 0.f) posSm = tgtPos;
        posSm += 0.002f * (tgtPos - posSm);
        if (cnt-- <= 0) { cnt = 16; update(); }
        float in[2] = { x.l, x.r };
        for (int c = 0; c < 2; ++c)
        {
            float y;
            switch (typ)
            {
                case 0: svf[c].process (std::tanh (in[c] * 1.2f) / 1.2f); y = svf[c].lp; break;             // NY LP
                case 1: svf[c].process (in[c]); y = svf[c].hp; break;                                        // NY HP
                case 2: svf[c].process (in[c]); y = std::tanh (svf[c].bp * (2.f + rs * 6.f)) * 0.3f; break;  // SCREAM
                default:
                    if (comboMode == 0) y = in[c];
                    else { svf[c].process (in[c]); svf2[c].process (comboMode < 0 ? svf[c].lp : svf[c].hp); y = comboMode < 0 ? svf2[c].lp : svf2[c].hp; }
            }
            in[c] = y;
        }
        return { in[0], in[1] };
    }

private:
    double sr = 48000.0; Svf svf[2], svf2[2];
    float tgtPos = 0.6f, posSm = -1.f, rs = 0.3f; int typ = 0, cnt = 0, comboMode = 0;
    void update()
    {
        const float q = typ == 2 ? 2.f + rs * 18.f : 0.6f + rs * rs * 12.f;
        if (typ == 3)
        {
            const float p = posSm;
            if (p < 0.48f) { comboMode = -1; const float f = 150.f * std::pow (20000.f / 150.f, p / 0.48f); for (int c = 0; c < 2; ++c) { svf[c].set (sr, f, 0.7f + rs); svf2[c].set (sr, f, 0.6f); } }
            else if (p > 0.52f) { comboMode = 1; const float f = 20.f * std::pow (5000.f / 20.f, (p - 0.52f) / 0.48f); for (int c = 0; c < 2; ++c) { svf[c].set (sr, f, 0.7f + rs); svf2[c].set (sr, f, 0.6f); } }
            else comboMode = 0;
            return;
        }
        const float f = 20.f * std::pow (1000.f, posSm);
        for (int c = 0; c < 2; ++c) svf[c].set (sr, f, q);
    }
};

class TransformFx
{
public:
    void prepare (double fs) { sr = fs; for (auto& d : comb) d.prepare ((int) (fs * 0.01)); cur = -1; reset(); }
    void reset() { for (int c = 0; c < 2; ++c) { for (auto& b : bq[c]) b.reset(); comb[c].reset(); cy[c] = 0.f; } }
    void set (int type, float width) { if (type != cur) design (type); wd = width; }
    Stereo process (Stereo x)
    {
        float in[2] = { x.l, x.r };
        for (int c = 0; c < 2; ++c)
        {
            float y = in[c];
            for (int i = 0; i < nbq; ++i) y = bq[c][i].process (y);
            if (combD > 0.f) { cy[c] = y + combFb * comb[c].read (combD * (c ? 1.07f : 1.f)); comb[c].push (flushDenormal (cy[c])); y = 0.5f * (y + cy[c]); }
            in[c] = y * gainComp;
        }
        const float M = 0.5f * (in[0] + in[1]), S = 0.5f * (in[0] - in[1]) * clampf (1.f + wd, 0.f, 2.f);
        return { M + S, M - S };
    }

private:
    double sr = 48000.0; Biquad bq[2][6]; DelayLine comb[2]; float cy[2] {};
    int nbq = 0, cur = -1; float combD = 0.f, combFb = 0.f, wd = 0.f, gainComp = 1.f;
    void design (int t)
    {
        cur = t; nbq = 0; combD = 0.f; combFb = 0.f; gainComp = 1.f;
        auto add = [this] (int kind, float f, float q, float db)
        {
            for (int c = 0; c < 2; ++c)
            {
                Biquad& b = bq[c][nbq];
                if (kind == 0) b.setHP (sr, f, q); else if (kind == 1) b.setLP (sr, f, q); else if (kind == 2) b.setPeak (sr, f, q, db);
                else if (kind == 3) b.setShelf (sr, f, db, false); else b.setShelf (sr, f, db, true);
                b.reset();
            }
            ++nbq;
        };
        switch (t)
        {
            case 0: add (0, 200.f, 0.7f, 0); add (2, 900.f, 1.2f, 8.f); add (1, 6000.f, 0.7f, 0); combD = (float) (sr * 0.0022); combFb = 0.55f; gainComp = 0.7f; break; // HOLLOW
            case 1: add (0, 700.f, 0.7f, 0); add (4, 5000.f, 0.7f, 8.f); add (2, 9000.f, 1.f, 4.f); gainComp = 0.8f; break;                                      // AIR
            case 2: add (2, 450.f, 3.f, 7.f); add (2, 2500.f, 1.f, -6.f); add (1, 6000.f, 0.7f, 0); combD = (float) (sr * 0.0007); combFb = 0.4f; gainComp = 0.8f; break; // CREEP
            case 3: add (3, 120.f, 0.7f, 4.f); add (2, 1000.f, 0.7f, -6.f); add (4, 6000.f, 0.7f, 4.f); gainComp = 0.9f; break;                                 // BROADCAST
            case 4: add (0, 500.f, 0.8f, 0); add (0, 500.f, 0.8f, 0); add (1, 3500.f, 0.8f, 0); add (1, 3500.f, 0.8f, 0); add (2, 1800.f, 1.f, 5.f); gainComp = 1.5f; break; // RADIO
            case 5: add (0, 300.f, 0.7f, 0); add (2, 4000.f, 1.f, 8.f); add (1, 7000.f, 0.7f, 0); gainComp = 0.75f; break;                                       // SMALL AMP
            case 6: add (0, 90.f, 0.7f, 0); add (2, 1500.f, 0.6f, 6.f); add (1, 4000.f, 0.7f, 0); add (1, 4000.f, 0.7f, 0); gainComp = 0.85f; break;             // 2X10 MID
            default: add (0, 80.f, 0.7f, 0); add (2, 800.f, 0.8f, -5.f); add (1, 5500.f, 0.7f, 0); gainComp = 1.1f; break;                                     // 2X10 SMILE
        }
    }
};

// Shred: captura trechos sincronizados ao tempo e repete (sequencia euclidiana A/B/C)
class ShredFx
{
public:
    void prepare (double fs) { sr = fs; for (auto& b : buf) b.assign ((size_t) (fs * 2.5) + 8, 0.f); reset(); }
    void reset() { segPos = 0; segIdx = -1; step = 0; }
    void set (int lengthIdx, int seq, int rA, int rB, int rC) { lenIdx = lengthIdx; sq = std::clamp (seq, 1, 16); rate[0] = rA; rate[1] = rB; rate[2] = rC; }
    // beatPos: posicao em batidas (ppq) desta amostra; spb: amostras por batida
    Stereo process (Stereo x, double beatPos, double spb, int64_t& stepOut)
    {
        static const double lenBeats[4] = { 2.0, 1.0, 0.5, 0.25 };
        static const double rateBeats[5] = { 0.5, 0.25, 0.125, 0.0625, 0.03125 };
        const double L = lenBeats[std::clamp (lenIdx, 0, 3)];
        const int64_t idx = (int64_t) std::floor (beatPos / L);
        if (idx != segIdx)
        {
            segIdx = idx; segPos = 0;
            const int k = sq - 1, s16 = (int) (((idx % 16) + 16) % 16);
            const bool hit = k > 0 && ((s16 * k) % 16) < k;      // ritmo euclidiano E(k,16)
            int hitsBefore = 0;
            for (int i = 0; i < s16; ++i) if (((i * k) % 16) < k) ++hitsBefore;
            step = hit ? 1 + (hitsBefore % 2) : 0;
        }
        stepOut = step;
        const int R = std::max (32, (int) std::min ((double) buf[0].size() - 4, rateBeats[std::clamp (rate[step], 0, 4)] * spb));
        Stereo y;
        if (segPos < R) { buf[0][(size_t) segPos] = x.l; buf[1][(size_t) segPos] = x.r; y = x; }
        else
        {
            const int p = segPos % R;
            const int fade = std::min (64, R / 4);
            float w = 1.f;
            if (p < fade) w = (float) p / (float) fade;
            else if (p > R - fade) w = (float) (R - p) / (float) fade;
            y = { buf[0][(size_t) p] * w, buf[1][(size_t) p] * w };
        }
        ++segPos;
        return y;
    }

private:
    double sr = 48000.0; std::vector<float> buf[2];
    int64_t segIdx = -1; int segPos = 0, step = 0, lenIdx = 1, sq = 1, rate[3] { 1, 2, 3 };
};

class ChorusFx
{
public:
    void prepare (double fs) { sr = fs; for (auto& d : dl) d.prepare ((int) (fs * 0.06)); reset(); }
    void reset() { for (auto& d : dl) d.reset(); ph = 0.0; }
    void set (float depth, float rateHz, float width) { dp = depth; rt = rateHz; wd = width; }
    Stereo process (Stereo x)
    {
        dl[0].push (x.l); dl[1].push (x.r);
        ph += rt / sr; if (ph >= 1.0) ph -= 1.0;
        const float base = (float) (sr * 0.014), dep = (float) (sr * 0.007) * dp;
        float o[2];
        for (int c = 0; c < 2; ++c)
        {
            const float off = c == 0 ? 0.f : 0.25f * wd + 0.0f;
            const float l1 = std::sin (kTwoPi * (float) (ph + off)), l2 = std::sin (kTwoPi * (float) (ph + off + 0.5f));
            o[c] = 0.5f * (dl[c].read (base + dep * (0.5f + 0.5f * l1)) + dl[c].read (base * 1.4f + dep * (0.5f + 0.5f * l2)));
        }
        const float M = 0.5f * (o[0] + o[1]), S = 0.5f * (o[0] - o[1]) * (0.3f + 1.2f * wd);
        return { M + S, M - S };
    }

private:
    double sr = 48000.0, ph = 0.0; DelayLine dl[2]; float dp = 0.4f, rt = 0.8f, wd = 0.7f;
};

class DelayFx
{
public:
    void prepare (double fs) { sr = fs; for (auto& d : dl) d.prepare ((int) (fs * 4.2)); reset(); }
    void reset() { for (auto& d : dl) d.reset(); tSm = -1.f; lp[0] = lp[1] = 0.f; }
    void set (float timeSamples, float width, float fb)
    {
        tTgt = clampf (timeSamples, 16.f, (float) sr * 4.f); wd = width; f = fb;
        a = 1.f - std::exp (-kTwoPi * 6500.f / (float) sr);
    }
    Stereo process (Stereo x)
    {
        if (tSm < 0.f) tSm = tTgt;
        tSm += 0.0004f * (tTgt - tSm);
        const float tl = tSm, tr = tSm * (1.f + 0.25f * wd);
        const float wl = dl[0].read (tl), wr = dl[1].read (std::min (tr, (float) sr * 4.1f));
        lp[0] += a * (wl - lp[0]); lp[1] += a * (wr - lp[1]);
        const float cross = 0.5f * wd;
        dl[0].push (flushDenormal (x.l + f * ((1.f - cross) * lp[0] + cross * lp[1])));
        dl[1].push (flushDenormal (x.r + f * ((1.f - cross) * lp[1] + cross * lp[0])));
        return { wl, wr };
    }

private:
    double sr = 48000.0; DelayLine dl[2]; float tSm = -1.f, tTgt = 10000.f, wd = 0.5f, f = 0.3f, a = 0.5f, lp[2] {};
};
} // namespace vfx
