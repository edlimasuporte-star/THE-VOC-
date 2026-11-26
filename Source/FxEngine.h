// THE VOC FX - motor principal (C++ puro, sem JUCE, testavel na linha de comando)
// Fluxo: entrada -> pitch -> [voz/correcao + TRACT + VOCODER + ROBOT + TALKBOX + HARMONY] -> gate -> efeitos -> saida
#pragma once
#include "FxParams.h"
#include "FxModules.h"
#include "FxEffects.h"
#include <atomic>
#include <limits>

namespace vfx
{
struct Meters
{
    std::atomic<float> in { 0.f }, out { 0.f }, note { -1.f };
    std::atomic<float> module[M_COUNT] { {0.f}, {0.f}, {0.f}, {0.f}, {0.f} };
    std::atomic<int> shredStep { 0 };
};

class Engine
{
public:
    Meters meters;

    Engine() { for (int i = 0; i < P_COUNT; ++i) p[i] = kParams[i].def; }

    void prepare (double fs, int /*maxBlock*/)
    {
        sr = fs;
        tracker.prepare (fs);
        corr.prepare (fs);
        for (auto& h : harm) h.prepare (fs);
        voc.prepare (fs); tract.prepare (fs); robot.prepare (fs); talk.prepare (fs);
        dist.prepare (fs); ring.prepare (fs); filt.prepare (fs); trans.prepare (fs); shred.prepare (fs); chorus.prepare (fs); delay.prepare (fs);
        for (int m = 0; m < 4; ++m) osc[m].seed (1234u + 77u * (uint32_t) m);
        for (auto* s : { &sVoice, &sTr, &sVc, &sRb, &sTb, &sHm, &sMix, &sGain, &sDs, &sRm, &sFl, &sTf, &sSh, &sCh, &sDl, &sGate })
            s->prepare (fs, 25.f);
        gateEnvA = std::exp (-1.f / (float) (fs * 0.001)); gateEnvR = std::exp (-1.f / (float) (fs * 0.08));
        meterA = std::exp (-1.f / (float) (fs * 0.05));
        reset();
    }

    void reset()
    {
        tracker.reset(); corr.reset(); for (auto& h : harm) h.reset();
        voc.reset(); tract.reset(); robot.reset(); talk.reset();
        dist.reset(); ring.reset(); filt.reset(); trans.reset(); shred.reset(); chorus.reset(); delay.reset();
        for (auto& s : slots) s = NoteSlot();
        nHeld = 0; unvoicedTime = 1.f; autoGate = false; gateEnv = 0.f; corrShift = 0.f; corrTgt = -1.f;
        for (int h = 0; h < 4; ++h) { hDrift[h] = 0.f; hDriftT[h] = 0.f; hLag[h] = rng.uni(); }
        localBeat = 0.0; ctl = 0;
        update (true);
    }

    void setParam (int id, float v) { if (id >= 0 && id < P_COUNT) p[id] = v; }
    float getParam (int id) const { return p[id]; }

    void noteOn (int note, int vel)
    {
        if (vel <= 0) { noteOff (note); return; }
        noteOff (note);
        if (nHeld < 16) held[nHeld++] = { note, vel };
        if (! isMono() && mode() == 1)
        {
            int best = -1; int64_t oldest = std::numeric_limits<int64_t>::max();
            for (int i = 0; i < 8; ++i) if (! slots[i].gate && slots[i].env <= 1e-4f) { best = i; break; }
            if (best < 0) for (int i = 0; i < 8; ++i) if (! slots[i].gate && slots[i].age < oldest) { oldest = slots[i].age; best = i; }
            if (best < 0) for (int i = 0; i < 8; ++i) if (slots[i].age < oldest) { oldest = slots[i].age; best = i; }
            NoteSlot& s = slots[best];
            const bool fresh = s.env <= 1e-4f;
            s.gate = true; s.midiNote = note; s.target = (float) note; if (fresh || p[P_glide] < 0.5f) s.note = (float) note;
            s.level = 0.4f + 0.6f * (float) vel / 127.f; s.age = sampleCount;
        }
    }
    void noteOff (int note)
    {
        for (int i = 0; i < nHeld; ++i)
            if (held[i].note == note) { for (int k = i; k < nHeld - 1; ++k) held[k] = held[k + 1]; --nHeld; break; }
        for (auto& s : slots) if (s.midiNote == note && mode() == 1 && ! isMono()) { s.gate = false; s.midiNote = -1; }
    }
    void allNotesOff() { nHeld = 0; for (auto& s : slots) { s.gate = false; s.midiNote = -1; } }

    // scL/scR podem ser nullptr (sem sidechain). Entrada e saida podem ser o mesmo buffer.
    void process (const float* inL, const float* inR, const float* scL, const float* scR,
                  float* outL, float* outR, int num, double bpm, double ppq, bool playing)
    {
        if (bpm < 20.0 || bpm > 400.0) bpm = 120.0;
        setTempo (bpm);
        update (false);
        const double spb = sr * 60.0 / bpm;
        if (playing) localBeat = ppq;
        const int md = mode();
        const bool en[M_COUNT] = { p[P_trOn] > 0.5f, p[P_vcOn] > 0.5f, p[P_rbOn] > 0.5f, p[P_tbOn] > 0.5f, p[P_hmOn] > 0.5f && md != 2 };
        const bool corrOn = p[P_corrOn] > 0.5f;
        float mAcc[M_COUNT] = { 0.f, 0.f, 0.f, 0.f, 0.f }; float inAcc = 0.f, outAcc = 0.f;

        for (int i = 0; i < num; ++i)
        {
            const float xl = inL[i], xr = inR[i];
            const float v = 0.5f * (xl + xr);
            const float sc = scL ? 0.5f * (scL[i] + (scR ? scR[i] : scL[i])) : 0.f;
            tracker.push (v);
            if (++ctl >= 32) { ctl = 0; control(); }
            const int64_t n = tracker.n - 1;

            // ---------- portadora (synth interno ou sidechain) ----------
            renderSlots();
            float car[4] = { 0.f, 0.f, 0.f, 0.f };
            for (int m = 0; m < 4; ++m)
                if (en[m]) car[m] = md == 2 ? sc : 0.5f * osc[m].render (kRecipes[recipeOf (m)], incs, amps, 8);

            // ---------- voz seca / corrigida ----------
            float wl = 0.f, wr = 0.f;
            const float vg = sVoice.next();
            if (corrOn)
            {
                corr.run (tracker, n, std::exp2 (corrShift / 12.f), 1.f, 0.f, 1.f);
                const float c = corr.out (n) * vg; wl += c; wr += c;
            }
            else { wl += xl * vg; wr += xr * vg; }

            // ---------- modulos ----------
            const float gTr = sTr.next(), gVc = sVc.next(), gRb = sRb.next(), gTb = sTb.next(), gHm = sHm.next();
            if (en[M_TRACT])
            {
                const float y = tract.process (tracker, v, car[0], p[P_trClarity] * 0.01f, p[P_trShift] * 0.01f, p[P_trNasal] * 0.01f, p[P_trBreath] * 0.01f) * gTr;
                addPan (y, p[P_trPan], wl, wr); mAcc[M_TRACT] += y * y;
            }
            if (en[M_VOCODER])
            {
                const float y = voc.process (v, car[1]) * gVc;
                addPan (y, p[P_vcPan], wl, wr); mAcc[M_VOCODER] += y * y;
            }
            if (en[M_ROBOT])
            {
                const float y = robot.process (tracker, car[2], (int) p[P_rbMode], p[P_rbBits] * 0.01f, p[P_rbBytes] * 0.01f, p[P_rbBats] * 0.01f) * gRb;
                addPan (y, p[P_rbPan], wl, wr); mAcc[M_ROBOT] += y * y;
            }
            if (en[M_TALKBOX])
            {
                const float y = talk.process (tracker, v, car[3], (int) p[P_tbMode], p[P_tbDrive] * 0.01f, p[P_tbSpeaker] * 0.01f, p[P_tbFormant] * 0.01f) * gTb;
                addPan (y, p[P_tbPan], wl, wr); mAcc[M_TALKBOX] += y * y;
            }
            if (en[M_HARMONY])
            {
                float hl = 0.f, hr = 0.f;
                for (int h = 0; h < 4; ++h)
                {
                    harm[h].run (tracker, n, hRatio[h], hFf[h], hExtraLag[h], hGain[h] > 0.f ? 1.f : 0.f);
                    const float y = harm[h].out (n) * hGainSm (h);
                    addPan (y, hPan[h], hl, hr);
                }
                wl += hl * gHm; wr += hr * gHm; mAcc[M_HARMONY] += 0.5f * (hl * hl + hr * hr) * gHm * gHm;
            }

            // ---------- gate (detecta na entrada, aplica no sinal processado) ----------
            const float ax = std::fabs (v);
            gateEnv = ax > gateEnv ? gateEnvA * gateEnv + (1.f - gateEnvA) * ax : gateEnvR * gateEnv + (1.f - gateEnvR) * ax;
            const float thr = dbToGain (p[P_gate]);
            if (p[P_gate] <= -79.5f) sGate.set (1.f);
            else if (gateEnv > thr * 1.4f) sGate.set (1.f);
            else if (gateEnv < thr) sGate.set (0.f);
            const float gg = sGate.next(); wl *= gg; wr *= gg;

            // ---------- efeitos ----------
            Stereo s { wl, wr };
            s = fx (s, dist, sDs, P_dsOn);
            if (p[P_rmOn] > 0.5f) { const float mx = sRm.next(); const Stereo y = ring.process (s); s = { s.l + (y.l - s.l) * mx, s.r + (y.r - s.r) * mx }; } else sRm.next();
            s = fx (s, filt, sFl, P_flOn);
            s = fx (s, trans, sTf, P_tfOn);
            {
                const double beat = localBeat + (double) i / spb;
                int64_t st = 0;
                if (p[P_shOn] > 0.5f) { const float mx = sSh.next(); const Stereo y = shred.process (s, beat, spb, st); s = { s.l + (y.l - s.l) * mx, s.r + (y.r - s.r) * mx }; meters.shredStep.store ((int) st); }
                else sSh.next();
            }
            s = fx (s, chorus, sCh, P_chOn);
            if (p[P_dlOn] > 0.5f) { const float mx = sDl.next(); const Stereo y = delay.process (s); s = { s.l + y.l * mx, s.r + y.r * mx }; }
            else sDl.next();

            // ---------- saida ----------
            const float mix = sMix.next();
            float ol = xl * (1.f - mix) + s.l * mix, orr = xr * (1.f - mix) + s.r * mix;
            const float wd = clampf (1.f + p[P_outWidth] * 0.01f, 0.f, 2.f);
            const float M = 0.5f * (ol + orr), S = 0.5f * (ol - orr) * wd;
            ol = M + S; orr = M - S;
            const float pn = p[P_outPan] * 0.01f;
            ol *= pn > 0.f ? 1.f - pn : 1.f; orr *= pn < 0.f ? 1.f + pn : 1.f;
            const float g = sGain.next(); ol *= g; orr *= g;
            if (p[P_safety] > 0.5f) { ol = softClip (ol); orr = softClip (orr); }
            if (! std::isfinite (ol) || ! std::isfinite (orr)) { ol = orr = 0.f; reset(); }
            outL[i] = ol; outR[i] = orr;
            inAcc += v * v; outAcc += 0.5f * (ol * ol + orr * orr);
            ++sampleCount;
        }
        localBeat += (double) num / spb;

        // ---------- medidores (para a viseira do capacete) ----------
        auto upd = [&] (std::atomic<float>& a, float acc)
        {
            const float rms = std::sqrt (acc / (float) std::max (num, 1));
            const float k = std::pow (meterA, (float) num);
            a.store (std::max (rms, a.load() * k));
        };
        upd (meters.in, inAcc); upd (meters.out, outAcc);
        for (int m = 0; m < M_COUNT; ++m) upd (meters.module[m], mAcc[m]);
        meters.note.store (tracker.voiced ? tracker.midi : -1.f);
    }

    int latencySamples() const { return 0; }

private:
    double sr = 48000.0;
    float p[P_COUNT];
    Tracker tracker; PsolaVoice corr, harm[4];
    Vocoder voc; TractModule tract; RobotModule robot; TalkboxModule talk;
    DistortFx dist; RingModFx ring; FilterFx filt; TransformFx trans; ShredFx shred; ChorusFx chorus; DelayFx delay;
    OscBank osc[4];
    NoteSlot slots[8];
    float incs[8] {}, amps[8] {};
    struct Held { int note, vel; } held[16] {}; int nHeld = 0;
    Smooth sVoice, sTr, sVc, sRb, sTb, sHm, sMix, sGain, sDs, sRm, sFl, sTf, sSh, sCh, sDl, sGate;
    float gateEnv = 0.f, gateEnvA = 0.9f, gateEnvR = 0.999f, meterA = 0.999f;
    float unvoicedTime = 1.f; bool autoGate = false;
    float corrShift = 0.f, corrTgt = -1.f;
    float hRatio[4] { 1, 1, 1, 1 }, hFf[4] { 1, 1, 1, 1 }, hGain[4] {}, hGainCur[4] {}, hPan[4] {}, hExtraLag[4] {};
    float hDrift[4] {}, hDriftT[4] {}, hLag[4] {};
    float vibPh = 0.f, attInc = 1.f, relCoef = 0.99f, glideCoef = 1.f, autoCoef = 1.f;
    double localBeat = 0.0; int ctl = 0; int64_t sampleCount = 0;
    Rng rng { 2025u };

    int mode() const { return (int) p[P_mode]; }
    bool isMono() const { return p[P_polyMode] < 0.5f; }
    int recipeOf (int m) const { static const int ids[4] = { P_trSynth, P_vcSynth, P_rbSynth, P_tbSynth }; return std::clamp ((int) p[ids[m]], 0, 9); }

    static float softClip (float x)
    {
        const float a = std::fabs (x);
        if (a <= 0.9f) return x;
        return (x > 0.f ? 1.f : -1.f) * (0.9f + 0.1f * std::tanh ((a - 0.9f) / 0.1f));
    }
    static void addPan (float y, float pan100, float& l, float& r)
    {
        const float ang = (clampf (pan100 * 0.01f, -1.f, 1.f) + 1.f) * 0.78539816f;
        l += y * std::cos (ang) * 1.41421356f; r += y * std::sin (ang) * 1.41421356f;
    }
    float hGainSm (int h) { hGainCur[h] += 0.002f * (hGain[h] - hGainCur[h]); return hGainCur[h]; }

    template <typename FX>
    Stereo fx (Stereo s, FX& e, Smooth& mixS, int onId)
    {
        const float mx = mixS.next();
        if (p[onId] <= 0.5f) return s;
        const Stereo y = e.process (s);
        return { s.l + (y.l - s.l) * mx, s.r + (y.r - s.r) * mx };
    }

    // ---------- parametros (por bloco) ----------
    void update (bool snap)
    {
        auto S = [snap] (Smooth& s, float v) { if (snap) s.snap (v); else s.set (v); };
        S (sVoice, dbToGain (p[P_voiceLvl]));
        S (sTr, dbToGain (p[P_trLvl])); S (sVc, dbToGain (p[P_vcLvl])); S (sRb, dbToGain (p[P_rbLvl]));
        S (sTb, dbToGain (p[P_tbLvl])); S (sHm, dbToGain (p[P_hmLvl]));
        S (sMix, p[P_outMix] * 0.01f); S (sGain, std::pow (10.f, p[P_outGain] / 20.f));
        S (sDs, p[P_dsMix] * 0.01f); S (sRm, p[P_rmMix] * 0.01f); S (sFl, p[P_flMix] * 0.01f); S (sTf, p[P_tfMix] * 0.01f);
        S (sSh, p[P_shMix] * 0.01f); S (sCh, p[P_chMix] * 0.01f); S (sDl, p[P_dlMix] * 0.01f);
        if (snap) sGate.snap (1.f);

        tracker.reg = (int) p[P_reg];
        voc.setParams ((int) p[P_vcMode], p[P_vcShift] * 0.01f, p[P_vcScale] * 0.01f);
        dist.set (p[P_dsDrive] * 0.01f, p[P_dsTone] * 0.01f, (int) p[P_dsType]);
        filt.set (p[P_flFreq] * 0.01f, p[P_flRes] * 0.01f, (int) p[P_flType]);
        trans.set ((int) p[P_tfType], p[P_tfWidth] * 0.01f);
        shred.set ((int) p[P_shLength], (int) std::lround (p[P_shSeq]), (int) p[P_shRateA], (int) p[P_shRateB], (int) p[P_shRateC]);
        chorus.set (p[P_chDepth] * 0.01f, p[P_chRate], p[P_chWidth] * 0.01f);
        const double bpm = lastBpm;
        static const double divBeats[7] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 0.0625 };
        const float rf = p[P_rmSync] > 0.5f ? (float) (bpm / 60.0 / divBeats[std::clamp ((int) p[P_rmDiv], 0, 6)]) : p[P_rmFreq];
        ring.set (rf, p[P_rmSync] > 0.5f ? 0.f : p[P_rmLfo], p[P_rmSync] > 0.5f ? 0.f : p[P_rmDepth] * 0.01f);
        static const double dlBeats[9] = { 0.125, 0.25, 0.375, 0.5, 0.75, 1.0, 1.5, 2.0, 4.0 };
        const float dt = p[P_dlSync] > 0.5f ? (float) (dlBeats[std::clamp ((int) p[P_dlDiv], 0, 8)] * 60.0 / bpm * sr) : p[P_dlTime] * 0.001f * (float) sr;
        delay.set (dt, p[P_dlWidth] * 0.01f, p[P_dlFeedback] * 0.01f);

        attInc = 1.f / std::max (1.f, p[P_synAttack] * 0.001f * (float) sr);
        relCoef = std::exp (-1.f / std::max (1.f, p[P_synRelease] * 0.001f * (float) sr / 4.f));
        glideCoef = p[P_glide] < 0.5f ? 1.f : 1.f - std::exp (-1.f / ((0.002f + p[P_glide] * 0.01f * p[P_glide] * 0.01f * 0.6f) * (float) sr));
        autoCoef = 1.f - std::exp (-1.f / (0.004f * (float) sr));
    }
public:
    void setTempo (double bpm) { if (bpm >= 20.0 && bpm <= 400.0) lastBpm = bpm; }
private:
    double lastBpm = 120.0;

    // ---------- controle (a cada 32 amostras): notas do synth, harmonias, correcao ----------
    void control()
    {
        const float dt = 32.f / (float) sr;
        const int md = mode();
        ScaleMap sm; sm.key = (int) p[P_key]; sm.scale = std::clamp ((int) p[P_scale], 0, 5);
        const int iscale = sm.scale == 2 ? 0 : sm.scale;   // cromatica -> intervalos da maior
        const bool voiced = tracker.voiced;
        const float m = tracker.midi;

        // correcao de afinacao
        if (voiced)
        {
            const int q = sm.nearest (m, sm.scale, nullptr);
            const float spd = p[P_corrSpeed] * 0.001f;
            if (corrTgt < 0.f || spd <= 0.0005f) corrTgt = (float) q;
            else corrTgt += (1.f - std::exp (-dt / spd)) * ((float) q - corrTgt);
            corrShift = clampf ((corrTgt - m) * p[P_corrStr] * 0.01f, -12.f, 12.f);
        }
        else { corrTgt = -1.f; corrShift = 0.f; }
        const float base = m + (p[P_corrOn] > 0.5f ? corrShift : 0.f);

        static const int kDeg[18] = { 0, -14, -7, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 14 };
        float autoNote[4] = { base, base, base, base }; bool autoOn[4] = { true, false, false, false };
        if (voiced)
        {
            int qi = 0; const int q = sm.nearest (m, iscale, &qi);
            for (int k = 1; k < 4; ++k)
            {
                const int sel = (int) p[P_autoInt1 + (k - 1) * 2];
                if (sel <= 0) continue;
                autoOn[k] = true;
                autoNote[k] = (float) sm.noteAt (qi + kDeg[sel], iscale) + (base - (float) q);
            }
        }

        // gate automatico do synth (segue a voz)
        const float holdT = 0.08f + (p[P_rbOn] > 0.5f ? p[P_rbBytes] * 0.0045f : 0.f);
        if (voiced) { unvoicedTime = 0.f; autoGate = true; } else { unvoicedTime += dt; if (unvoicedTime > holdT) autoGate = false; }

        const float oct = 12.f * std::round (p[P_synOct]);
        if (md == 0)
        {
            for (int k = 0; k < 8; ++k)
            {
                NoteSlot& s = slots[k];
                if (k < 4 && autoOn[k])
                {
                    s.level = k == 0 ? p[P_autoLvl0] * 0.01f : p[P_autoLvl1 + (k - 1) * 2] * 0.01f;
                    s.gate = autoGate && s.level > 0.001f;
                    if (voiced) { s.target = autoNote[k] + oct; if (s.env <= 1e-4f) s.note = s.target; }
                }
                else s.gate = false;
                s.midiNote = -1;
            }
        }
        else if (md == 1 && isMono())
        {
            NoteSlot& s = slots[0];
            if (nHeld > 0)
            {
                const auto& h = held[nHeld - 1];
                s.target = (float) h.note + oct; if (s.env <= 1e-4f) s.note = s.target;
                s.level = 0.4f + 0.6f * (float) h.vel / 127.f; s.gate = true;
            }
            else s.gate = false;
            for (int k = 1; k < 8; ++k) slots[k].gate = false;
        }
        else if (md == 1)
        {
            for (auto& s : slots) if (s.midiNote >= 0) s.target = (float) s.midiNote + oct;
        }
        else for (auto& s : slots) s.gate = false;

        // HARMONY: alvos das vozes
        const float hum = p[P_hmHuman] * 0.01f;
        for (int h = 0; h < 4; ++h)
        {
            hDriftT[h] -= dt;
            if (hDriftT[h] <= 0.f) { hDriftT[h] = 0.15f + 0.35f * rng.uni(); hDriftTarget[h] = rng.next(); }
            hDrift[h] += (hDriftTarget[h] - hDrift[h]) * (1.f - std::exp (-dt / 0.12f));
        }
        float tgtNote[4]; float lvl[4] = { 0.f, 0.f, 0.f, 0.f };
        if (md == 0)
        {
            for (int h = 0; h < 3; ++h) if (autoOn[h + 1]) { tgtNote[h] = autoNote[h + 1]; lvl[h] = p[P_autoLvl1 + h * 2] * 0.01f; }
        }
        else if (md == 1)
        {
            const int cnt = isMono() ? std::min (nHeld, 1) : std::min (nHeld, 4);
            for (int h = 0; h < cnt; ++h)
            {
                const auto& hd = held[isMono() ? nHeld - 1 : nHeld - 1 - h];
                tgtNote[h] = (float) hd.note; lvl[h] = 0.4f + 0.6f * (float) hd.vel / 127.f;
            }
        }
        const float wdt = p[P_hmWidth] * 0.01f;
        static const float panPos[4] = { -1.f, 1.f, -0.4f, 0.4f };
        for (int h = 0; h < 4; ++h)
        {
            hGain[h] = lvl[h];
            if (lvl[h] > 0.f && voiced)
            {
                const float semis = tgtNote[h] - m + hDrift[h] * hum * 0.35f;
                hRatio[h] = std::exp2 (clampf (semis, -24.f, 24.f) / 12.f);
            }
            else if (! voiced) hRatio[h] = 1.f;
            hFf[h] = std::exp2 (p[P_hmFormant] * 0.005f) * std::pow (hRatio[h], p[P_hmChar] * 0.01f);
            hPan[h] = panPos[h] * wdt * 100.f;
            hExtraLag[h] = hum * (float) sr * 0.018f * hLag[h];
        }
    }
    float hDriftTarget[4] {};

    void renderSlots()
    {
        const float vib = p[P_synVib] * 0.01f * 0.5f;
        vibPh += p[P_synVibRate] / (float) sr; if (vibPh >= 1.f) vibPh -= 1.f;
        const float vs = vib > 0.f ? vib * std::sin (kTwoPi * vibPh) : 0.f;
        const float coef = mode() == 0 ? autoCoef : glideCoef;
        for (int k = 0; k < 8; ++k)
        {
            NoteSlot& s = slots[k];
            s.note += (s.target - s.note) * coef;
            if (s.gate) s.env = std::min (1.f, s.env + attInc);
            else s.env = flushDenormal (s.env * relCoef);
            if (s.env < 1e-5f && ! s.gate) { amps[k] = 0.f; continue; }
            amps[k] = s.env * s.level;
            incs[k] = 440.f * std::exp2 ((s.note + vs - 69.f) / 12.f) / (float) sr;
        }
    }
};
} // namespace vfx
