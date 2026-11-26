#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

namespace
{
const char* kModName[5] = { "TRACT", "VOCODER", "ROBOT", "TALKBOX", "HARMONY" };
const char* kModTag[5]  = { "vocal tract synth", "classic band vocoder", "LPC computer voice", "mouth-tube talkbox", "vocal harmonies" };
const int kModOn[5]   = { vfx::P_trOn, vfx::P_vcOn, vfx::P_rbOn, vfx::P_tbOn, vfx::P_hmOn };
const int kModLvl[5]  = { vfx::P_trLvl, vfx::P_vcLvl, vfx::P_rbLvl, vfx::P_tbLvl, vfx::P_hmLvl };
const int kModChar[5] = { vfx::P_trShift, vfx::P_vcShift, vfx::P_rbBits, vfx::P_tbFormant, vfx::P_hmHuman };
const char* kModCharName[5] = { "SHIFT", "SHIFT", "BITS", "FORMANT", "HUMAN" };

const char* kFxName[7] = { "DISTORT", "RING MOD", "FILTER", "TRANSFORM", "SHRED", "CHORUS", "DELAY" };
const int kFxOn[7]   = { vfx::P_dsOn, vfx::P_rmOn, vfx::P_flOn, vfx::P_tfOn, vfx::P_shOn, vfx::P_chOn, vfx::P_dlOn };
const int kFxMain[7] = { vfx::P_dsDrive, vfx::P_rmFreq, vfx::P_flFreq, vfx::P_tfType, vfx::P_shSeq, vfx::P_chDepth, vfx::P_dlFeedback };
const char* kFxMainName[7] = { "DRIVE", "FREQ", "FREQ", "TYPE", "SEQ", "DEPTH", "FEEDBK" };
const int kFxMix[7]  = { vfx::P_dsMix, vfx::P_rmMix, vfx::P_flMix, vfx::P_tfMix, vfx::P_shMix, vfx::P_chMix, vfx::P_dlMix };

String shortName (int id)
{
    String n = String (vfx::kParams[id].name).toUpperCase();
    static const char* prefixes[] = { "TRACT ", "VOCODER ", "ROBOT ", "TALKBOX ", "HARMONY ", "DISTORT ", "RINGMOD ", "FILTER ", "TRANSFORM ", "SHRED ", "CHORUS ", "DELAY ", "OUTPUT ", "SYNTH ", "CORRECT " };
    for (auto* p : prefixes) if (n.startsWith (p)) return n.substring ((int) std::strlen (p));
    return n;
}

String noteName (float midi)
{
    static const char* nn[12] = { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    const int m = (int) std::lround (midi);
    return String (nn[((m % 12) + 12) % 12]) + String (m / 12 - 1);
}
}

// ======================================================================= ParamControl

ParamControl::ParamControl (AudioProcessorValueTreeState& s, int paramId, const String& label, Colour accent, bool led)
    : id (paramId), text (label.isEmpty() ? shortName (paramId) : label)
{
    const auto& d = vfx::kParams[paramId];
    if (d.type == vfx::PBOOL)
    {
        toggle = std::make_unique<ToggleButton> (text);
        if (led) toggle->getProperties().set ("led", (int64) accent.getARGB());
        addAndMakeVisible (*toggle);
        ba = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (s, d.id, *toggle);
        text = {};
    }
    else if (d.type == vfx::PCHOICE)
    {
        combo = std::make_unique<ComboBox>();
        for (int k = 0; k < vfx::numChoices (paramId); ++k) combo->addItem (vfx::choiceName (paramId, k), k + 1);
        addAndMakeVisible (*combo);
        ca = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (s, d.id, *combo);
    }
    else
    {
        slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
        slider->setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        slider->getProperties().set ("accent", (int64) accent.getARGB());
        slider->setTooltip (d.name);
        addAndMakeVisible (*slider);
        sa = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (s, d.id, *slider);
        slider->setDoubleClickReturnValue (true, (double) d.def);
    }
}

void ParamControl::resized()
{
    auto r = getLocalBounds();
    if (toggle) { toggle->setBounds (r.withSizeKeepingCentre (r.getWidth(), jmin (r.getHeight(), 28))); return; }
    if (text.isNotEmpty()) r.removeFromTop (13);
    if (combo) { combo->setBounds (r.withSizeKeepingCentre (r.getWidth(), jmin (r.getHeight(), 24))); return; }
    if (slider)
    {
        const int tb = r.getHeight() > 50 ? 14 : 12;
        slider->setTextBoxStyle (Slider::TextBoxBelow, false, r.getWidth(), tb);
        slider->setBounds (r);
    }
}

void ParamControl::paint (Graphics& g)
{
    if (text.isEmpty()) return;
    g.setColour (ui::cream().withAlpha (0.7f));
    g.setFont (ui::font (10.5f, true));
    g.drawText (text, getLocalBounds().removeFromTop (13), Justification::centred);
}

// ======================================================================= HelmetPad

HelmetPad::HelmetPad (TheVocFxProcessor& p) : proc (p)
{
    helmet = ImageCache::getFromMemory (BinaryData::helmet_png, BinaryData::helmet_pngSize);
    visor  = ImageCache::getFromMemory (BinaryData::visor_png, BinaryData::visor_pngSize);
    setMouseCursor (MouseCursor::CrosshairCursor);
}

Rectangle<float> HelmetPad::imageArea() const
{
    auto b = getLocalBounds().toFloat().reduced (8.f);
    if (! helmet.isValid()) return b;
    const float ar = (float) helmet.getWidth() / (float) helmet.getHeight();
    float w = b.getWidth(), h = w / ar;
    if (h > b.getHeight()) { h = b.getHeight(); w = h * ar; }
    return b.withSizeKeepingCentre (w, h);
}

void HelmetPad::tick()
{
    const auto& m = proc.engine.meters;
    const float out = m.out.load();
    const float db = Decibels::gainToDecibels (out, -100.f);
    const float target = jlimit (0.f, 1.f, jmap (db, -48.f, -10.f, 0.f, 1.f));
    glow += (target - glow) * (target > glow ? 0.55f : 0.12f);
    // cor: mistura das cores dos modulos ativos, ponderada pelo nivel de cada um
    float r = 1.f * 0.02f, gg = 0.72f * 0.02f, b = 0.45f * 0.02f, wsum = 0.02f;
    const float vin = m.in.load();
    r += 1.f * vin * 0.6f; gg += 0.72f * vin * 0.6f; b += 0.45f * vin * 0.6f; wsum += vin * 0.6f;
    for (int k = 0; k < 5; ++k)
    {
        const float w = m.module[k].load();
        const auto c = ui::moduleColour (k);
        r += c.getFloatRed() * w; gg += c.getFloatGreen() * w; b += c.getFloatBlue() * w; wsum += w;
    }
    const float tr = r / wsum, tg = gg / wsum, tb = b / wsum;
    hue[0] += (tr - hue[0]) * 0.15f; hue[1] += (tg - hue[1]) * 0.15f; hue[2] += (tb - hue[2]) * 0.15f;
    pulse += 0.05f; if (pulse > MathConstants<float>::twoPi) pulse -= MathConstants<float>::twoPi;
    repaint();
}

void HelmetPad::paint (Graphics& g)
{
    const auto area = imageArea();
    // halo atras do capacete
    {
        ColourGradient halo (Colour::fromFloatRGBA (hue[0], hue[1], hue[2], 0.10f + 0.25f * glow), area.getCentreX(), area.getCentreY(),
                             Colours::transparentBlack, area.getCentreX() + area.getWidth() * 0.62f, area.getCentreY(), true);
        g.setGradientFill (halo); g.fillEllipse (area.expanded (area.getWidth() * 0.12f));
    }
    if (helmet.isValid()) g.drawImage (helmet, area, RectanglePlacement::stretchToFit);
    if (visor.isValid())
    {
        const auto col = Colour::fromFloatRGBA (jlimit (0.f, 1.f, hue[0]), jlimit (0.f, 1.f, hue[1]), jlimit (0.f, 1.f, hue[2]), 1.f);
        const float a = 0.10f + 0.9f * glow;
        const auto toArea = AffineTransform::scale (area.getWidth() / (float) visor.getWidth(), area.getHeight() / (float) visor.getHeight())
                                .translated (area.getX(), area.getY());
        // brilho difuso (bloom) em volta da viseira
        const Point<float> vc (area.getX() + area.getWidth() * 0.64f, area.getY() + area.getHeight() * 0.53f);
        for (int k = 3; k >= 1; --k)
        {
            const float s = 1.f + 0.035f * (float) k * (0.4f + glow);
            g.setColour (col.withAlpha (a * 0.16f));
            g.drawImageTransformed (visor, toArea.followedBy (AffineTransform::scale (s, s, vc.x, vc.y)), true);
        }
        g.setColour (col.withAlpha (a * 0.82f));
        g.drawImageTransformed (visor, toArea, true);

        // HUD dentro da viseira: linhas de varredura + nota cantada
        Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (visor, toArea);
        g.setColour (ui::navy().withAlpha (0.10f + 0.1f * glow));
        for (float y = area.getY(); y < area.getBottom(); y += 3.f) g.fillRect (area.getX(), y, area.getWidth(), 1.f);
        const float note = proc.engine.meters.note.load();
        if (note > 0.f)
        {
            g.setColour (ui::cream().withAlpha (0.35f + 0.6f * glow));
            g.setFont (ui::font (area.getHeight() * 0.13f, true, 0.9f));
            g.drawText (noteName (note), Rectangle<float> (vc.x - 90.f, vc.y - 40.f, 180.f, 80.f), Justification::centred);
        }
        // reflexo
        g.setColour (Colours::white.withAlpha (0.06f + 0.04f * std::sin (pulse)));
        g.fillEllipse (vc.x - area.getWidth() * 0.05f, area.getY() + area.getHeight() * 0.28f, area.getWidth() * 0.12f, area.getHeight() * 0.06f);
    }
    // ponto do XY
    const float px = area.getX() + proc.xyPos[0] * area.getWidth();
    const float py = area.getBottom() - proc.xyPos[1] * area.getHeight();
    g.setColour (ui::orange().withAlpha (dragging ? 0.9f : 0.45f));
    g.drawEllipse (px - 9.f, py - 9.f, 18.f, 18.f, 2.f);
    g.fillEllipse (px - 3.f, py - 3.f, 6.f, 6.f);
    if (dragging)
    {
        g.setColour (ui::orange().withAlpha (0.25f));
        g.drawHorizontalLine ((int) py, area.getX(), area.getRight());
        g.drawVerticalLine ((int) px, area.getY(), area.getBottom());
    }
}

void HelmetPad::setFrom (const MouseEvent& e, bool begin, bool end)
{
    const auto a = imageArea();
    proc.setXY ((e.position.x - a.getX()) / a.getWidth(), (a.getBottom() - e.position.y) / a.getHeight(), begin, end);
    repaint();
}
void HelmetPad::mouseDown (const MouseEvent& e) { dragging = true; setFrom (e, true, false); }
void HelmetPad::mouseDrag (const MouseEvent& e) { setFrom (e, false, false); }
void HelmetPad::mouseUp (const MouseEvent& e)   { setFrom (e, false, true); dragging = false; }

// ======================================================================= InspirePanel

InspirePanel::InspirePanel (TheVocFxProcessor& p) : proc (p)
{
    for (int i = 0; i < 6; ++i)
    {
        auto* b = ideaButtons.add (new TextButton());
        b->setClickingTogglesState (false);
        b->onClick = [this, i] { selected = i; proc.applyIdea (i); for (int k = 0; k < ideaButtons.size(); ++k) ideaButtons[k]->setToggleState (k == i, dontSendNotification); };
        addAndMakeVisible (b);
    }
    more.getProperties().set ("filled", true);
    more.onClick = [this] { refresh(); };
    save.onClick = [this] { if (onSave) onSave(); };
    close.onClick = [this] { if (onClose) onClose(); };
    for (auto* b : { &more, &save, &close }) addAndMakeVisible (b);
}

void InspirePanel::refresh()
{
    proc.generateIdeas (6);
    selected = -1;
    const auto& ideas = proc.getIdeas();
    for (int i = 0; i < ideaButtons.size(); ++i)
    {
        ideaButtons[i]->setButtonText (i < (int) ideas.size() ? ideas[(size_t) i].name + "\n" + ideas[(size_t) i].tags : String());
        ideaButtons[i]->setToggleState (false, dontSendNotification);
    }
}

void InspirePanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::navy().withAlpha (0.94f)); g.fillRoundedRectangle (r, 8.f);
    g.setColour (ui::orange()); g.drawRoundedRectangle (r.reduced (1.f), 8.f, 2.f);
    g.setFont (ui::font (28.f, true, 0.85f));
    g.drawText ("INSPIRE", r.removeFromTop (60.f).reduced (24.f, 10.f), Justification::centredLeft);
    g.setColour (ui::cream().withAlpha (0.7f));
    g.setFont (ui::font (13.f));
    g.drawText ("Desbloqueio criativo: clique numa ideia para ouvir ao vivo. Mantem modo, tom e escala da musica.",
                Rectangle<float> (190.f, 18.f, r.getWidth() - 200.f, 30.f), Justification::centredLeft);
}

void InspirePanel::resized()
{
    auto r = getLocalBounds().reduced (24);
    r.removeFromTop (44);
    auto bottom = r.removeFromBottom (44);
    close.setBounds (bottom.removeFromRight (120).reduced (2));
    bottom.removeFromRight (10);
    save.setBounds (bottom.removeFromRight (150).reduced (2));
    more.setBounds (bottom.removeFromLeft (220).reduced (2));
    r.removeFromBottom (12);
    const int cw = r.getWidth() / 3, ch = r.getHeight() / 2;
    for (int i = 0; i < ideaButtons.size(); ++i)
        ideaButtons[i]->setBounds (Rectangle<int> (r.getX() + (i % 3) * cw, r.getY() + (i / 3) * ch, cw, ch).reduced (6));
}

// ======================================================================= EditPanel

EditPanel::EditPanel (AudioProcessorValueTreeState& s) : st (s)
{
    using namespace vfx;
    const auto teal = ui::tealHi(), org = ui::orange();
    addPage ("PITCH & VOICES", {
        { "PITCH", teal, { P_reg, P_key, P_scale, P_corrOn, P_corrSpeed, P_corrStr, P_voiceLvl } },
        { "MIDI", teal, { P_mode, P_polyMode, P_glide } },
        { "AUTO VOICES", teal, { P_autoLvl0, P_autoInt1, P_autoLvl1, P_autoInt2, P_autoLvl2, P_autoInt3, P_autoLvl3 } },
        { "SYNTH", teal, { P_synOct, P_synAttack, P_synRelease, P_synVib, P_synVibRate } } });
    addPage ("ENGINES", {
        { "TRACT", ui::moduleColour (0), { P_trOn, P_trSynth, P_trClarity, P_trShift, P_trNasal, P_trBreath, P_trLvl, P_trPan } },
        { "VOCODER", ui::moduleColour (1), { P_vcOn, P_vcSynth, P_vcMode, P_vcShift, P_vcScale, P_vcLvl, P_vcPan } },
        { "ROBOT", ui::moduleColour (2), { P_rbOn, P_rbSynth, P_rbMode, P_rbBits, P_rbBytes, P_rbBats, P_rbLvl, P_rbPan } },
        { "TALKBOX", ui::moduleColour (3), { P_tbOn, P_tbSynth, P_tbMode, P_tbDrive, P_tbSpeaker, P_tbFormant, P_tbLvl, P_tbPan } },
        { "HARMONY", ui::moduleColour (4), { P_hmOn, P_hmFormant, P_hmChar, P_hmHuman, P_hmLvl, P_hmWidth } } });
    addPage ("FX 1", {
        { "DISTORT", org, { P_dsOn, P_dsType, P_dsDrive, P_dsTone, P_dsMix } },
        { "RING MOD", org, { P_rmOn, P_rmSync, P_rmFreq, P_rmDiv, P_rmLfo, P_rmDepth, P_rmMix } },
        { "FILTER", org, { P_flOn, P_flType, P_flFreq, P_flRes, P_flMix } },
        { "TRANSFORM", org, { P_tfOn, P_tfType, P_tfWidth, P_tfMix } } });
    addPage ("FX 2", {
        { "SHRED", org, { P_shOn, P_shLength, P_shSeq, P_shRateA, P_shRateB, P_shRateC, P_shMix } },
        { "CHORUS", org, { P_chOn, P_chDepth, P_chRate, P_chWidth, P_chMix } },
        { "DELAY", org, { P_dlOn, P_dlSync, P_dlTime, P_dlDiv, P_dlWidth, P_dlFeedback, P_dlMix } } });
    addPage ("OUTPUT", { { "OUTPUT", teal, { P_outMix, P_gate, P_outPan, P_outWidth, P_outGain, P_safety } } });
    for (int i = 0; i < (int) pages.size(); ++i)
    {
        auto* t = tabs.add (new TextButton (pages[(size_t) i].name));
        t->setClickingTogglesState (false);
        t->onClick = [this, i] { show (i); };
        addAndMakeVisible (t);
    }
    show (0);
}

void EditPanel::addPage (const String& name, const std::vector<RowDef>& rows)
{
    Page pg; pg.name = name;
    for (auto& r : rows)
    {
        Row row; row.title = r.title; row.colour = r.colour;
        for (int id : r.ids)
        {
            const bool isOn = vfx::kParams[id].type == vfx::PBOOL && String (vfx::kParams[id].id).endsWith ("On");
            auto* c = all.add (new ParamControl (st, id, isOn ? String ("ON") : String(), row.colour, false));
            addChildComponent (c);
            row.controls.push_back (c);
        }
        pg.rows.push_back (row);
    }
    pages.push_back (pg);
}

void EditPanel::show (int page)
{
    current = page;
    for (auto* c : all) c->setVisible (false);
    for (auto& r : pages[(size_t) page].rows) for (auto* c : r.controls) c->setVisible (true);
    for (int i = 0; i < tabs.size(); ++i) tabs[i]->setToggleState (i == page, dontSendNotification);
    resized(); repaint();
}

void EditPanel::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (ui::navy().withAlpha (0.55f)); g.fillRoundedRectangle (r, 6.f);
    g.setColour (ui::cream().withAlpha (0.15f)); g.drawRoundedRectangle (r.reduced (0.5f), 6.f, 1.f);
    const auto& pg = pages[(size_t) current];
    auto area = getLocalBounds().reduced (12); area.removeFromTop (40);
    const int rh = jmin (84, area.getHeight() / jmax (1, (int) pg.rows.size()));
    for (auto& row : pg.rows)
    {
        auto rr = area.removeFromTop (rh);
        g.setColour (row.colour); g.fillRect (rr.getX(), rr.getY() + 10, 3, rr.getHeight() - 20);
        g.setFont (ui::font (14.f, true, 0.9f));
        g.drawText (row.title, rr.withWidth (110).withTrimmedLeft (10), Justification::centredLeft);
        g.setColour (ui::cream().withAlpha (0.07f)); g.drawHorizontalLine (rr.getBottom() - 1, (float) rr.getX(), (float) rr.getRight());
    }
}

void EditPanel::resized()
{
    auto area = getLocalBounds().reduced (12);
    auto tabRow = area.removeFromTop (30);
    const int tw = tabRow.getWidth() / jmax (1, tabs.size());
    for (auto* t : tabs) t->setBounds (tabRow.removeFromLeft (tw).reduced (3, 0));
    area.removeFromTop (10);
    if (pages.empty()) return;
    const auto& pg = pages[(size_t) current];
    const int rh = jmin (84, area.getHeight() / jmax (1, (int) pg.rows.size()));
    for (auto& row : pg.rows)
    {
        auto rr = area.removeFromTop (rh).reduced (0, 4);
        rr.removeFromLeft (115);
        const int cw = jmin (104, rr.getWidth() / jmax (1, (int) row.controls.size()));
        for (auto* c : row.controls)
        {
            auto cell = rr.removeFromLeft (cw).reduced (4, 0);
            if (c->toggle) cell = cell.withSizeKeepingCentre (cell.getWidth(), 28);
            else if (c->combo) cell = cell.withSizeKeepingCentre (cell.getWidth(), 40);
            c->setBounds (cell);
        }
    }
}

// ======================================================================= Editor

TheVocFxEditor::TheVocFxEditor (TheVocFxProcessor& p)
    : AudioProcessorEditor (&p), proc (p), helmet (p), edit (p.apvts), inspire (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (root);
    auto& s = proc.apvts;

    // ---- topo
    for (auto* b : { &prevBtn, &nextBtn, &presetBtn, &saveBtn, &inspireBtn, &editBtn }) root.addAndMakeVisible (b);
    prevBtn.onClick = [this] { proc.stepPreset (-1); };
    nextBtn.onClick = [this] { proc.stepPreset (1); };
    presetBtn.onClick = [this] { showPresetMenu(); };
    saveBtn.onClick = [this] { showSaveDialog(); };
    inspireBtn.getProperties().set ("filled", true);
    inspireBtn.setTooltip ("Gera presets criativos automaticos (desbloqueio criativo)");
    inspireBtn.onClick = [this] { inspire.refresh(); inspire.setVisible (true); inspire.toFront (false); };
    editBtn.setClickingTogglesState (true);
    editBtn.onClick = [this] { setEditMode (editBtn.getToggleState()); };
    const char* modeNames[3] = { "AUTO", "MIDI", "SIDECHAIN" };
    for (int i = 0; i < 3; ++i)
    {
        modeBtn[i].setButtonText (modeNames[i]);
        modeBtn[i].onClick = [this, i] { if (auto* prm = proc.apvts.getParameter ("mode")) prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i)); };
        root.addAndMakeVisible (modeBtn[i]);
    }
    modeBtn[0].setTooltip ("AUTO: o synth segue a sua voz");
    modeBtn[1].setTooltip ("MIDI: toque as notas/acordes num teclado ou clipe MIDI");
    modeBtn[2].setTooltip ("SIDECHAIN: faz qualquer instrumento 'falar' com a sua voz");
    keyC = std::make_unique<ParamControl> (s, vfx::P_key, " ");
    scaleC = std::make_unique<ParamControl> (s, vfx::P_scale, " ");
    regC = std::make_unique<ParamControl> (s, vfx::P_reg, " ");
    corrC = std::make_unique<ParamControl> (s, vfx::P_corrOn, "TUNE");
    keyC->combo->setTooltip ("Tom da musica"); scaleC->combo->setTooltip ("Escala"); regC->combo->setTooltip ("Registro da voz (LOW / MID / HIGH)");
    for (auto* c : { keyC.get(), scaleC.get(), regC.get(), corrC.get() }) root.addAndMakeVisible (c);

    // ---- motores vocais
    for (int m = 0; m < 5; ++m)
    {
        modOn[m] = std::make_unique<ParamControl> (s, kModOn[m], kModName[m], ui::moduleColour (m), true);
        modLvl[m] = std::make_unique<ParamControl> (s, kModLvl[m], "LEVEL", ui::moduleColour (m));
        modChar[m] = std::make_unique<ParamControl> (s, kModChar[m], kModCharName[m], ui::moduleColour (m));
        modOn[m]->toggle->setButtonText (kModName[m]);
        for (auto* c : { modOn[m].get(), modLvl[m].get(), modChar[m].get() }) root.addAndMakeVisible (c);
    }
    // ---- efeitos
    for (int f = 0; f < 7; ++f)
    {
        fxOn[f] = std::make_unique<ParamControl> (s, kFxOn[f], kFxName[f], ui::orange(), true);
        fxOn[f]->toggle->setButtonText (kFxName[f]);
        fxMain[f] = std::make_unique<ParamControl> (s, kFxMain[f], kFxMainName[f]);
        fxMix[f] = std::make_unique<ParamControl> (s, kFxMix[f], "MIX");
        for (auto* c : { fxOn[f].get(), fxMain[f].get(), fxMix[f].get() }) root.addAndMakeVisible (c);
    }
    // ---- base
    for (int k = 0; k < 3; ++k) { autoInt[k] = std::make_unique<ParamControl> (s, vfx::P_autoInt1 + k * 2, "VOICE " + String (k + 1), ui::moduleColour (4)); root.addAndMakeVisible (*autoInt[k]); }
    voiceC = std::make_unique<ParamControl> (s, vfx::P_voiceLvl, "VOICE", ui::tealHi());
    gateC = std::make_unique<ParamControl> (s, vfx::P_gate, "GATE", ui::tealHi());
    mixC = std::make_unique<ParamControl> (s, vfx::P_outMix, "MIX");
    outC = std::make_unique<ParamControl> (s, vfx::P_outGain, "OUTPUT");
    for (auto* c : { voiceC.get(), gateC.get(), mixC.get(), outC.get() }) root.addAndMakeVisible (c);

    root.addAndMakeVisible (helmet);
    for (int a = 0; a < 2; ++a)
    {
        for (int t = 0; t < kNumXYTargets; ++t) xyBox[a].addItem (String (a == 0 ? "X: " : "Y: ") + kXYTargets[t].label, t + 1);
        xyBox[a].setSelectedId (proc.xyTarget[a] + 1, dontSendNotification);
        xyBox[a].onChange = [this, a] { proc.xyTarget[a] = xyBox[a].getSelectedId() - 1; };
        xyBox[a].setTooltip ("Parametro controlado ao arrastar no capacete");
        root.addAndMakeVisible (xyBox[a]);
    }

    root.addChildComponent (edit);
    root.addChildComponent (inspire);
    inspire.onClose = [this] { inspire.setVisible (false); };
    inspire.onSave = [this] { showSaveDialog(); };

    buildBackground();
    setResizable (true, true);
    setResizeLimits (W * 6 / 10, H * 6 / 10, W * 2, H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) W / (double) H);
    setSize (W, H);
    layout();
    startTimerHz (30);
}

TheVocFxEditor::~TheVocFxEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void TheVocFxEditor::paint (Graphics& g) { g.fillAll (ui::tealLo()); }

void TheVocFxEditor::resized()
{
    const float sc = (float) getWidth() / (float) W;
    root.setBounds (0, 0, W, H);
    root.setTransform (AffineTransform::scale (sc));
}

void TheVocFxEditor::buildBackground()
{
    background = Image (Image::ARGB, W, H, true);
    Graphics g (background);
    ColourGradient bg (Colour (0xff4f9fbc), (float) W * 0.5f, (float) H * 0.45f, ui::tealLo(), 0.f, (float) H, true);
    g.setGradientFill (bg); g.fillAll();
    // textura de tinta (respingos laranja da arte)
    Random r (1983);
    for (int i = 0; i < 520; ++i)
    {
        const float x = r.nextFloat() * W, y = r.nextFloat() * H, d = 0.8f + r.nextFloat() * 2.6f;
        g.setColour ((r.nextFloat() < 0.8f ? ui::orange() : ui::cream()).withAlpha (0.08f + r.nextFloat() * 0.22f));
        g.fillEllipse (x, y, d * (0.6f + r.nextFloat()), d);
    }
    // vinheta
    ColourGradient vig (Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.5f, Colours::black.withAlpha (0.45f), 0.f, 0.f, true);
    g.setGradientFill (vig); g.fillAll();
}

void TheVocFxEditor::layout()
{
    prevBtn.setBounds (268, 14, 30, 32);
    presetBtn.setBounds (302, 14, 290, 32);
    nextBtn.setBounds (596, 14, 30, 32);
    saveBtn.setBounds (634, 14, 84, 32);
    inspireBtn.setBounds (750, 10, 124, 40);
    editBtn.setBounds (884, 14, 96, 32);

    for (int i = 0; i < 3; ++i) modeBtn[i].setBounds (20 + i * 84, 66, 82, 28);
    keyC->setBounds (310, 62, 66, 34);
    scaleC->setBounds (382, 62, 112, 34);
    regC->setBounds (500, 62, 78, 34);
    corrC->setBounds (586, 66, 80, 28);
    xyBox[0].setBounds (700, 68, 136, 24);
    xyBox[1].setBounds (844, 68, 136, 24);

    // motores vocais (esquerda)
    for (int m = 0; m < 5; ++m)
    {
        const int y = 128 + m * 84;
        modOn[m]->setBounds (28, y + 8, 150, 32);
        modLvl[m]->setBounds (170, y + 2, 60, 78);
        modChar[m]->setBounds (234, y + 2, 60, 78);
    }
    // efeitos (direita)
    for (int f = 0; f < 7; ++f)
    {
        const int y = 128 + f * 60;
        fxOn[f]->setBounds (708, y + 13, 126, 32);
        if (vfx::kParams[kFxMain[f]].type == vfx::PCHOICE) fxMain[f]->setBounds (836, y + 6, 76, 42);
        else fxMain[f]->setBounds (846, y, 58, 58);
        fxMix[f]->setBounds (914, y, 58, 58);
    }
    helmet.setBounds (318, 104, 364, 384);
    for (int k = 0; k < 3; ++k) autoInt[k]->setBounds (322 + k * 120, 500, 114, 40);

    voiceC->setBounds (24, 560, 64, 74);
    gateC->setBounds (96, 560, 64, 74);
    mixC->setBounds (812, 560, 64, 74);
    outC->setBounds (882, 560, 64, 74);

    edit.setBounds (20, 104, 960, 446);
    inspire.setBounds (40, 110, 920, 430);
}

void TheVocFxEditor::paintRoot (Graphics& g)
{
    g.drawImageAt (background, 0, 0);
    // logo vintage
    g.setColour (ui::orange());
    g.setFont (ui::font (17.f, true, 0.9f));
    g.drawText ("THE", 24, 6, 60, 18, Justification::left);
    g.setFont (ui::font (46.f, true, 0.78f));
    g.drawText ("VOC", 20, 14, 140, 46, Justification::left);
    g.setFont (ui::font (24.f, true, 0.9f));
    g.drawText ("FX", 120, 30, 50, 26, Justification::left);
    g.setColour (ui::cream().withAlpha (0.55f));
    g.setFont (ui::font (9.5f, true));
    g.drawMultiLineText ("CREATIVE\nVOICE FX\nLIVE", 172, 24, 80);

    if (! editMode)
    {
        auto panel = [&g] (Rectangle<float> r, const String& title)
        {
            g.setColour (ui::navy().withAlpha (0.35f)); g.fillRoundedRectangle (r, 6.f);
            g.setColour (ui::cream().withAlpha (0.14f)); g.drawRoundedRectangle (r, 6.f, 1.f);
            g.setColour (ui::orange()); g.setFont (ui::font (12.f, true));
            g.drawText (title, r.withHeight (22.f).translated (12.f, 2.f), Justification::centredLeft);
        };
        panel ({ 20.f, 104.f, 284.f, 446.f }, "VOCAL ENGINES");
        panel ({ 696.f, 104.f, 284.f, 446.f }, "EFFECTS");
        for (int m = 0; m < 5; ++m)
        {
            const int y = 128 + m * 84;
            const bool on = proc.apvts.getRawParameterValue (vfx::kParams[kModOn[m]].id)->load() > 0.5f;
            if (on) { g.setColour (ui::moduleColour (m).withAlpha (0.10f)); g.fillRoundedRectangle (26.f, (float) y + 2.f, 272.f, 78.f, 5.f); }
            g.setColour (ui::cream().withAlpha (0.45f));
            g.setFont (ui::font (10.5f));
            g.drawText (kModTag[m], 52, y + 42, 120, 16, Justification::left);
        }
        g.setColour (ui::orange()); g.setFont (ui::font (11.f, true));
        g.drawText ("HARMONY VOICES (AUTO)", 322, 488, 360, 12, Justification::centred);
    }

    // barra inferior
    g.setColour (ui::navy().withAlpha (0.4f));
    g.fillRect (0, 556, W, H - 556);
    g.setColour (ui::cream().withAlpha (0.12f));
    g.drawHorizontalLine (556, 0.f, (float) W);
    g.setColour (ui::cream().withAlpha (0.6f));
    g.setFont (ui::font (11.f, true));
    g.drawText ("T H E E D L I M A", 0, 610, W, 16, Justification::centred);
    // nota detectada + modo
    const float note = proc.engine.meters.note.load();
    g.setColour (ui::orange()); g.setFont (ui::font (22.f, true, 0.85f));
    g.drawText (note > 0.f ? noteName (note) : String ("--"), 180, 566, 90, 30, Justification::centredLeft);
    g.setColour (ui::cream().withAlpha (0.5f)); g.setFont (ui::font (10.f, true));
    g.drawText ("PITCH IN", 180, 594, 90, 14, Justification::centredLeft);
    g.drawText (proc.presetName(), 300, 570, 400, 18, Justification::centred);

    // medidor de saida
    const float db = Decibels::gainToDecibels (outMeter, -60.f);
    const float lv = jlimit (0.f, 1.f, (db + 60.f) / 60.f);
    Rectangle<float> mr (958.f, 566.f, 12.f, 62.f);
    g.setColour (ui::navy()); g.fillRoundedRectangle (mr, 2.f);
    g.setColour (db > -3.f ? Colour (0xffff4d4d) : ui::orange());
    g.fillRoundedRectangle (mr.withTop (mr.getBottom() - mr.getHeight() * lv), 2.f);
}

void TheVocFxEditor::setEditMode (bool e)
{
    editMode = e;
    edit.setVisible (e);
    if (e) edit.toFront (false);
    for (int m = 0; m < 5; ++m) for (auto* c : { modOn[m].get(), modLvl[m].get(), modChar[m].get() }) c->setVisible (! e);
    for (int f = 0; f < 7; ++f) for (auto* c : { fxOn[f].get(), fxMain[f].get(), fxMix[f].get() }) c->setVisible (! e);
    for (auto& a : autoInt) a->setVisible (! e);
    helmet.setVisible (! e);
    editBtn.setButtonText (e ? "LIVE" : "EDIT");
    root.repaint();
}

void TheVocFxEditor::timerCallback()
{
    helmet.tick();
    const float o = proc.engine.meters.out.load();
    outMeter = jmax (o, outMeter * 0.85f);
    const int md = (int) proc.apvts.getRawParameterValue ("mode")->load();
    for (int i = 0; i < 3; ++i) modeBtn[i].setToggleState (i == md, dontSendNotification);
    modOn[4]->setEnabled (md != 2); modLvl[4]->setEnabled (md != 2); modChar[4]->setEnabled (md != 2);
    for (auto& a : autoInt) a->setEnabled (md == 0);
    presetBtn.setButtonText (proc.presetName());
    // posicao do XY acompanha os parametros quando nao esta arrastando
    if (! helmet.dragging)
        for (int a = 0; a < 2; ++a)
        {
            const int t = proc.xyTarget[a];
            if (t > 0 && t < kNumXYTargets)
                if (auto* prm = proc.apvts.getParameter (vfx::kParams[kXYTargets[t].param].id)) proc.xyPos[a] = prm->getValue();
            if (xyBox[a].getSelectedId() != t + 1) xyBox[a].setSelectedId (t + 1, dontSendNotification);
        }
    root.repaint (0, 556, W, H - 556);
    root.repaint (20, 104, 284, 446);
}

void TheVocFxEditor::showPresetMenu()
{
    PopupMenu menu;
    const int nf = vfx::numFactoryPresets();
    for (int c = 0; c < vfx::numCategories(); ++c)
    {
        PopupMenu sub;
        for (int i = 0; i < nf; ++i)
            if (String (vfx::kFactory[i].category) == vfx::kCategories[c]) sub.addItem (1 + i, vfx::kFactory[i].name, true, proc.presetName() == vfx::kFactory[i].name);
        menu.addSubMenu (vfx::kCategories[c], sub);
    }
    const auto users = proc.scanUserPresets();
    PopupMenu mine;
    StringArray cats;
    for (auto& u : users) cats.addIfNotAlreadyThere (u.category);
    for (auto& cat : cats)
    {
        PopupMenu sub;
        for (int i = 0; i < users.size(); ++i) if (users[i].category == cat) sub.addItem (1000 + i, users[i].name, true, proc.presetName() == users[i].name);
        mine.addSubMenu (cat, sub);
    }
    if (users.isEmpty()) mine.addItem (999, "(nenhum preset salvo ainda)", false);
    menu.addSeparator();
    menu.addSubMenu ("MEUS PRESETS", mine);
    menu.addSeparator();
    menu.addItem (5001, "Salvar preset atual...");
    menu.addItem (5002, "Abrir pasta de presets");
    menu.showMenuAsync (PopupMenu::Options().withTargetComponent (&presetBtn).withMinimumWidth (presetBtn.getWidth()),
        [this, users, nf] (int r)
        {
            if (r <= 0) return;
            if (r <= nf) proc.loadFactory (r - 1);
            else if (r >= 1000 && r < 1000 + users.size()) proc.loadUserPreset (users[r - 1000].file);
            else if (r == 5001) showSaveDialog();
            else if (r == 5002) TheVocFxProcessor::userPresetFolder().startAsProcess();
        });
}

void TheVocFxEditor::showSaveDialog()
{
    auto* aw = new AlertWindow ("SALVAR PRESET", "Escolha um nome e um estilo para o seu preset.", MessageBoxIconType::NoIcon, this);
    aw->setLookAndFeel (&lnf);
    String nm = proc.presetName();
    if (nm.startsWith ("* ")) nm = nm.substring (2);
    aw->addTextEditor ("name", nm, "Nome:");
    StringArray cats ("MEUS PRESETS");
    for (int c = 1; c < vfx::numCategories(); ++c) cats.add (vfx::kCategories[c]);
    aw->addComboBox ("cat", cats, "Estilo:");
    aw->addButton ("SALVAR", 1, KeyPress (KeyPress::returnKey));
    aw->addButton ("CANCELAR", 0, KeyPress (KeyPress::escapeKey));
    aw->enterModalState (true, ModalCallbackFunction::create ([this, aw] (int result)
    {
        if (result == 1)
        {
            const String name = aw->getTextEditorContents ("name");
            String cat = "MEUS PRESETS";
            if (auto* cb = aw->getComboBoxComponent ("cat")) cat = cb->getText();
            if (! proc.saveUserPreset (name, cat))
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "THE VOC FX", "Nao foi possivel salvar o preset.");
        }
    }), true);
}
