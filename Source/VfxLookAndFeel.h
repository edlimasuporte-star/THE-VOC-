#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

// Identidade vintage THE VOC FX: azul-petroleo, laranja queimado, creme e azul-marinho (da arte da capa)
namespace ui
{
    inline juce::Colour orange() { return juce::Colour (0xffee7d4d); }
    inline juce::Colour cream()  { return juce::Colour (0xfff2e6d0); }
    inline juce::Colour navy()   { return juce::Colour (0xff0a2233); }
    inline juce::Colour tealHi() { return juce::Colour (0xff5aa9c2); }
    inline juce::Colour tealLo() { return juce::Colour (0xff0b2b3b); }
    inline juce::Colour moduleColour (int m)
    {
        static const juce::uint32 c[5] = { 0xffff6fa8, 0xff53d6ff, 0xff7dff8a, 0xffff9a3c, 0xffffd75e };
        return juce::Colour (c[juce::jlimit (0, 4, m)]);
    }
    inline juce::Font font (float h, bool bold = false, float hscale = 1.f)
    {
        juce::Font f (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain));
        return hscale != 1.f ? f.withHorizontalScale (hscale) : f;
    }
}

class VfxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VfxLookAndFeel()
    {
        using namespace juce;
        setColour (Label::textColourId, ui::cream());
        setColour (Slider::textBoxTextColourId, ui::cream().withAlpha (0.85f));
        setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
        setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
        setColour (Slider::textBoxHighlightColourId, ui::orange().withAlpha (0.5f));
        setColour (ComboBox::textColourId, ui::cream());
        setColour (ComboBox::backgroundColourId, ui::navy());
        setColour (PopupMenu::backgroundColourId, ui::navy());
        setColour (PopupMenu::textColourId, ui::cream());
        setColour (PopupMenu::headerTextColourId, ui::orange());
        setColour (PopupMenu::highlightedBackgroundColourId, ui::orange());
        setColour (PopupMenu::highlightedTextColourId, ui::navy());
        setColour (TextEditor::backgroundColourId, ui::navy());
        setColour (TextEditor::textColourId, ui::cream());
        setColour (TextEditor::outlineColourId, ui::orange());
        setColour (TextEditor::focusedOutlineColourId, ui::orange());
        setColour (AlertWindow::backgroundColourId, ui::navy());
        setColour (AlertWindow::textColourId, ui::cream());
        setColour (AlertWindow::outlineColourId, ui::orange());
        setColour (TextButton::buttonColourId, ui::navy());
        setColour (TextButton::textColourOffId, ui::cream());
        setColour (TextButton::textColourOnId, ui::navy());
        setColour (TextButton::buttonOnColourId, ui::orange());
        setColour (TooltipWindow::backgroundColourId, ui::navy());
        setColour (TooltipWindow::textColourId, ui::cream());
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
        const float rad = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float ang = a0 + pos * (a1 - a0);
        const float al = s.isEnabled() ? 1.f : 0.35f;
        const auto accent = s.getProperties().contains ("accent") ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["accent"]) : ui::orange();
        // corpo do knob (baquelite vintage)
        g.setColour (ui::navy().withAlpha (0.85f * al));
        g.fillEllipse (c.x - rad * 0.72f, c.y - rad * 0.72f, rad * 1.44f, rad * 1.44f);
        g.setColour (ui::cream().withAlpha (0.12f * al));
        g.drawEllipse (c.x - rad * 0.72f, c.y - rad * 0.72f, rad * 1.44f, rad * 1.44f, 1.f);
        juce::Path track, val;
        track.addCentredArc (c.x, c.y, rad * 0.92f, rad * 0.92f, 0.f, a0, a1, true);
        val.addCentredArc (c.x, c.y, rad * 0.92f, rad * 0.92f, 0.f, a0, ang, true);
        g.setColour (ui::cream().withAlpha (0.18f * al));
        g.strokePath (track, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent.withAlpha (al));
        g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const float sn = std::sin (ang), cs = std::cos (ang);
        g.setColour (ui::cream().withAlpha (al));
        g.drawLine (c.x + sn * rad * 0.25f, c.y - cs * rad * 0.25f, c.x + sn * rad * 0.62f, c.y - cs * rad * 0.62f, 2.2f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState();
        const float al = b.isEnabled() ? 1.f : 0.35f;
        if (b.getProperties().contains ("led"))
        {
            // botao de modulo: LED + nome
            const auto led = juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["led"]);
            const float d = std::min (12.f, r.getHeight() - 4.f);
            auto lr = juce::Rectangle<float> (r.getX() + 6.f, r.getCentreY() - d * 0.5f, d, d);
            if (on)
            {
                g.setColour (led.withAlpha (0.35f * al)); g.fillEllipse (lr.expanded (4.f));
                g.setColour (led.withAlpha (al)); g.fillEllipse (lr);
            }
            else { g.setColour (ui::cream().withAlpha (0.35f * al)); g.drawEllipse (lr, 1.2f); }
            g.setColour ((on ? ui::cream() : ui::cream().withAlpha (hl ? 0.85f : 0.6f)).withMultipliedAlpha (al));
            g.setFont (ui::font (r.getHeight() > 26.f ? 17.f : 13.f, true, 0.9f));
            g.drawText (b.getButtonText(), r.withTrimmedLeft (d + 14.f), juce::Justification::centredLeft);
            return;
        }
        if (on) { g.setColour (ui::orange().withAlpha (al)); g.fillRoundedRectangle (r, 3.f); g.setColour (ui::navy()); }
        else
        {
            g.setColour (ui::cream().withAlpha ((hl ? 0.6f : 0.35f) * al)); g.drawRoundedRectangle (r, 3.f, 1.2f);
            g.setColour (ui::cream().withAlpha (0.85f * al));
        }
        g.setFont (ui::font (11.5f, true));
        g.drawText (b.getButtonText(), r.toNearestInt(), juce::Justification::centred);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState() || b.getProperties().contains ("filled");
        if (on) { g.setColour (ui::orange().withBrightness (down ? 0.8f : (hl ? 1.f : 0.93f))); g.fillRoundedRectangle (r, 3.f); }
        else
        {
            g.setColour (ui::navy().withAlpha (down ? 0.9f : 0.6f)); g.fillRoundedRectangle (r, 3.f);
            g.setColour (ui::cream().withAlpha (hl ? 0.7f : 0.35f)); g.drawRoundedRectangle (r, 3.f, 1.2f);
        }
    }
    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        const bool on = b.getToggleState() || b.getProperties().contains ("filled");
        g.setColour (on ? ui::navy() : ui::cream());
        g.setFont (ui::font (b.getHeight() > 30 ? 15.f : 12.f, true));
        g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (4, 1), juce::Justification::centred, 2);
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb) override
    {
        auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        const float al = cb.isEnabled() ? 1.f : 0.35f;
        g.setColour (ui::navy().withAlpha (0.7f)); g.fillRoundedRectangle (r, 3.f);
        g.setColour (ui::orange().withAlpha (0.8f * al)); g.drawRoundedRectangle (r, 3.f, 1.f);
        juce::Path p; p.addTriangle ((float) w - 14.f, (float) h * 0.5f - 2.f, (float) w - 6.f, (float) h * 0.5f - 2.f, (float) w - 10.f, (float) h * 0.5f + 3.f);
        g.setColour (ui::orange().withAlpha (al)); g.fillPath (p);
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (4, 1, box.getWidth() - 18, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return ui::font (12.f, true); }
    juce::Font getLabelFont (juce::Label&) override { return ui::font (11.f); }
    juce::Font getPopupMenuFont() override { return ui::font (14.f); }
    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return ui::font (h > 30 ? 15.f : 12.f, true); }
    juce::Font getAlertWindowTitleFont() override { return ui::font (18.f, true); }
    juce::Font getAlertWindowMessageFont() override { return ui::font (14.f); }
    juce::Font getAlertWindowFont() override { return ui::font (13.f); }
};
