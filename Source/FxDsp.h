// THE VOC FX - blocos basicos de DSP (C++ puro)
#pragma once
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>

namespace vfx
{
constexpr float kPi = 3.14159265358979f;
constexpr float kTwoPi = 6.28318530717959f;

inline float dbToGain (float db) { return db <= -59.9f ? 0.f : std::pow (10.f, db / 20.f); }
inline float clampf (float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
inline float flushDenormal (float v) { return std::fabs (v) < 1.0e-15f ? 0.f : v; }

struct Rng
{
    uint32_t s = 0x9E3779B9u;
    explicit Rng (uint32_t seed = 0x9E3779B9u) : s (seed ? seed : 1u) {}
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) * (2.f / 16777216.f) - 1.f; } // [-1,1)
    float uni() { return 0.5f * (next() + 1.f); }
};

// Suavizador de parametro (one-pole)
struct Smooth
{
    float v = 0.f, target = 0.f, a = 0.999f;
    void prepare (double sr, float ms) { a = std::exp (-1.f / (float) (sr * ms * 0.001)); }
    void set (float t) { target = t; }
    void snap (float t) { v = target = t; }
    float next() { v = target + a * (v - target); return v; }
};

struct Biquad
{
    float b0 = 1.f, b1 = 0.f, b2 = 0.f, a1 = 0.f, a2 = 0.f, z1 = 0.f, z2 = 0.f;
    void reset() { z1 = z2 = 0.f; }
    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        z1 = flushDenormal (z1); z2 = flushDenormal (z2);
        return y;
    }
    void norm (float a0) { b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0; }
    static float wOf (double sr, float f) { return kTwoPi * clampf (f, 5.f, (float) sr * 0.47f) / (float) sr; }
    void setLP (double sr, float f, float q)
    {
        const float w = wOf (sr, f), c = std::cos (w), al = std::sin (w) / (2.f * q);
        b0 = (1.f - c) * 0.5f; b1 = 1.f - c; b2 = b0; a1 = -2.f * c; a2 = 1.f - al; norm (1.f + al);
    }
    void setHP (double sr, float f, float q)
    {
        const float w = wOf (sr, f), c = std::cos (w), al = std::sin (w) / (2.f * q);
        b0 = (1.f + c) * 0.5f; b1 = -(1.f + c); b2 = b0; a1 = -2.f * c; a2 = 1.f - al; norm (1.f + al);
    }
    void setBP (double sr, float f, float q) // ganho 0 dB no centro
    {
        const float w = wOf (sr, f), c = std::cos (w), al = std::sin (w) / (2.f * q);
        b0 = al; b1 = 0.f; b2 = -al; a1 = -2.f * c; a2 = 1.f - al; norm (1.f + al);
    }
    void setPeak (double sr, float f, float q, float db)
    {
        const float A = std::pow (10.f, db / 40.f), w = wOf (sr, f), c = std::cos (w), al = std::sin (w) / (2.f * q);
        b0 = 1.f + al * A; b1 = -2.f * c; b2 = 1.f - al * A; a1 = -2.f * c; a2 = 1.f - al / A; norm (1.f + al / A);
    }
    void setShelf (double sr, float f, float db, bool high)
    {
        const float A = std::pow (10.f, db / 40.f), w = wOf (sr, f), c = std::cos (w), s = std::sin (w);
        const float al = s / 2.f * std::sqrt (2.f), sq = 2.f * std::sqrt (A) * al;
        if (high)
        {
            b0 = A * ((A + 1.f) + (A - 1.f) * c + sq); b1 = -2.f * A * ((A - 1.f) + (A + 1.f) * c); b2 = A * ((A + 1.f) + (A - 1.f) * c - sq);
            a1 = 2.f * ((A - 1.f) - (A + 1.f) * c); a2 = (A + 1.f) - (A - 1.f) * c - sq; norm ((A + 1.f) - (A - 1.f) * c + sq);
        }
        else
        {
            b0 = A * ((A + 1.f) - (A - 1.f) * c + sq); b1 = 2.f * A * ((A - 1.f) - (A + 1.f) * c); b2 = A * ((A + 1.f) - (A - 1.f) * c - sq);
            a1 = -2.f * ((A - 1.f) + (A + 1.f) * c); a2 = (A + 1.f) + (A - 1.f) * c - sq; norm ((A + 1.f) + (A - 1.f) * c + sq);
        }
    }
    void setIdentity() { b0 = 1.f; b1 = b2 = a1 = a2 = 0.f; }
};

// Filtro de estado variavel TPT (Zavalishin)
struct Svf
{
    float g = 0.f, k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f, ic1 = 0.f, ic2 = 0.f;
    float lp = 0.f, bp = 0.f, hp = 0.f;
    void reset() { ic1 = ic2 = 0.f; }
    void set (double sr, float f, float q)
    {
        g = std::tan (kPi * clampf (f, 10.f, (float) sr * 0.46f) / (float) sr);
        k = 1.f / std::max (q, 0.05f);
        a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
    }
    inline void process (float x)
    {
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = flushDenormal (2.f * v1 - ic1); ic2 = flushDenormal (2.f * v2 - ic2);
        lp = v2; bp = v1; hp = x - k * v1 - v2;
    }
};

struct DcBlock
{
    float x1 = 0.f, y1 = 0.f, r = 0.995f;
    void prepare (double sr) { r = 1.f - kTwoPi * 20.f / (float) sr; }
    void reset() { x1 = y1 = 0.f; }
    inline float process (float x) { const float y = x - x1 + r * y1; x1 = x; y1 = flushDenormal (y); return y; }
};

// Linha de atraso com leitura fracionaria
struct DelayLine
{
    std::vector<float> buf; int mask = 0, w = 0;
    void prepare (int minSize) { int s = 1; while (s < minSize) s <<= 1; buf.assign ((size_t) s, 0.f); mask = s - 1; w = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.f); w = 0; }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    inline float read (float d) const // d em amostras (>= 1)
    {
        const float rp = (float) w - d;
        const int i0 = (int) std::floor (rp);
        const float fr = rp - (float) i0;
        const float s0 = buf[(size_t) (i0 & mask)], s1 = buf[(size_t) ((i0 + 1) & mask)];
        return s0 + (s1 - s0) * fr;
    }
};

// Tabela de janela Hann
struct HannTable
{
    float t[2049];
    HannTable() { for (int i = 0; i <= 2048; ++i) t[i] = 0.5f * (1.f - std::cos (kTwoPi * (float) i / 2048.f)); }
    inline float at (float pos01) const { return t[(int) (clampf (pos01, 0.f, 1.f) * 2048.f)]; }
};
inline const HannTable& hann() { static const HannTable h; return h; }

// Escalas (graus em semitons). Cromatica usa a maior para intervalos diatonicos.
static const int kScaleLen[6] = { 7, 7, 12, 5, 5, 7 };
static const int kScaleNotes[6][12] = {
    { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
    { 0, 2, 4, 7, 9 }, { 0, 3, 5, 7, 10 }, { 0, 2, 3, 5, 7, 9, 10 } };
inline int fdiv (int a, int b) { int q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) --q; return q; }
inline int pmod (int a, int b) { return a - b * fdiv (a, b); }

struct ScaleMap
{
    int key = 0, scale = 0;
    int len() const { return kScaleLen[scale]; }
    int noteAt (int idx, int sc) const { const int l = kScaleLen[sc]; return key + 12 * fdiv (idx, l) + kScaleNotes[sc][pmod (idx, l)]; }
    // nota da escala mais proxima de m (float) -> retorna nota e indice
    int nearest (float m, int sc, int* idxOut) const
    {
        const int l = kScaleLen[sc];
        const int base = (int) std::floor ((m - (float) key) / 12.f);
        int bestIdx = 0; float bd = 1e9f;
        for (int o = base - 1; o <= base + 1; ++o)
            for (int k = 0; k < l; ++k)
            {
                const int idx = o * l + k;
                const float d = std::fabs ((float) noteAt (idx, sc) - m);
                if (d < bd) { bd = d; bestIdx = idx; }
            }
        if (idxOut) *idxOut = bestIdx;
        return noteAt (bestIdx, sc);
    }
};
} // namespace vfx
