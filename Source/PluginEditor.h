#pragma once
#include "PluginProcessor.h"
#include "VfxLookAndFeel.h"

// Controle generico ligado a um parametro (knob, botao ou lista)
class ParamControl : public juce::Component
{
public:
    ParamControl (juce::AudioProcessorValueTreeState& s, int paramId, const juce::String& label = {}, juce::Colour accent = ui::orange(), bool led = false);
    void resized() override;
    void paint (juce::Graphics& g) override;
    int id;
    std::unique_ptr<juce::Slider> slider;
    std::unique_ptr<juce::ToggleButton> toggle;
    std::unique_ptr<juce::ComboBox> combo;
private:
    juce::String text;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sa;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ba;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> ca;
};

// O capacete: viseira acende com o som e funciona como XY pad de performance
class HelmetPad : public juce::Component
{
public:
    explicit HelmetPad (TheVocFxProcessor& p);
    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;
    void tick();
    bool dragging = false;
private:
    TheVocFxProcessor& proc;
    juce::Image helmet, visor;
    float glow = 0.f, hue[3] { 1.f, 0.7f, 0.42f }, pulse = 0.f;
    juce::Rectangle<float> imageArea() const;
    void setFrom (const juce::MouseEvent& e, bool begin, bool end);
};

// Painel INSPIRE: varias ideias criativas prontas para testar
class InspirePanel : public juce::Component
{
public:
    explicit InspirePanel (TheVocFxProcessor& p);
    void paint (juce::Graphics& g) override;
    void resized() override;
    void refresh();
    std::function<void()> onSave, onClose;
private:
    TheVocFxProcessor& proc;
    juce::OwnedArray<juce::TextButton> ideaButtons;
    juce::TextButton more { "GERAR MAIS IDEIAS" }, save { "SALVAR ESTE" }, close { "FECHAR" };
    int selected = -1;
};

// Pagina EDIT com todos os parametros
class EditPanel : public juce::Component
{
public:
    explicit EditPanel (juce::AudioProcessorValueTreeState& s);
    void paint (juce::Graphics& g) override;
    void resized() override;
private:
    struct Row { juce::String title; juce::Colour colour; std::vector<ParamControl*> controls; };
    struct Page { juce::String name; std::vector<Row> rows; };
    juce::AudioProcessorValueTreeState& st;
    std::vector<Page> pages;
    juce::OwnedArray<juce::TextButton> tabs;
    juce::OwnedArray<ParamControl> all;
    int current = 0;
    struct RowDef { juce::String title; juce::Colour colour; std::vector<int> ids; };
    void addPage (const juce::String& name, const std::vector<RowDef>& rows);
    void show (int page);
};

class TheVocFxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit TheVocFxEditor (TheVocFxProcessor&);
    ~TheVocFxEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int W = 1000, H = 640;

private:
    class Root : public juce::Component
    {
    public:
        explicit Root (TheVocFxEditor& e) : ed (e) { setOpaque (true); }
        void paint (juce::Graphics& g) override { ed.paintRoot (g); }
        TheVocFxEditor& ed;
    };

    TheVocFxProcessor& proc;
    VfxLookAndFeel lnf;
    Root root { *this };
    juce::Image background;

    // topo
    juce::TextButton prevBtn { "<" }, nextBtn { ">" }, presetBtn, saveBtn { "SALVAR" }, inspireBtn { "INSPIRE" }, editBtn { "EDIT" };
    juce::TextButton modeBtn[3];
    std::unique_ptr<ParamControl> keyC, scaleC, corrC, regC;

    // LIVE
    std::unique_ptr<ParamControl> modOn[5], modLvl[5], modChar[5];
    std::unique_ptr<ParamControl> fxOn[7], fxMain[7], fxMix[7];
    std::unique_ptr<ParamControl> autoInt[3], voiceC, mixC, gateC, outC;
    HelmetPad helmet;
    juce::ComboBox xyBox[2];

    EditPanel edit;
    InspirePanel inspire;
    juce::TooltipWindow tips { this, 600 };
    bool editMode = false;
    float outMeter = 0.f;

    void paintRoot (juce::Graphics& g);
    void buildBackground();
    void layout();
    void timerCallback() override;
    void showPresetMenu();
    void showSaveDialog();
    void setEditMode (bool e);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TheVocFxEditor)
};
