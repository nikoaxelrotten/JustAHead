#pragma once
#include "PluginProcessor.h"

namespace jahead
{
    constexpr int designW = 1200;
    constexpr int designH = 700;

    // Un "skin" por head: color de acento, carácter del panel de controles y de las perillas.
    struct Skin
    {
        const char* name;
        juce::Colour accent;
        juce::Colour stripTop, stripBottom;
        juce::Colour knobHi, knobLo, pointer;
        juce::Colour label, value;
    };

    const Skin& skinFor (int head);

    // Colores propios de cada módulo
    const juce::Colour colPedal  { 0xfff0a030 };
    const juce::Colour colMod    { 0xffb36bf0 };
    const juce::Colour colDelay  { 0xff38d39f };
    const juce::Colour colReverb { 0xff7f8cff };
    const juce::Colour colGate   { 0xffd5dae1 };
}

//==============================================================================
class JAHeadLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JAHeadLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getPopupMenuFont() override;
    void drawCornerResizer (juce::Graphics&, int w, int h, bool, bool) override;

    int skin = 0;   // head activo (0..3)
};

//==============================================================================
class JustAHeadEditor : public juce::AudioProcessorEditor
{
public:
    explicit JustAHeadEditor (JustAHeadProcessor&);
    ~JustAHeadEditor() override;
    void resized() override;

private:
    class Content;
    JustAHeadProcessor& proc;
    JAHeadLookAndFeel laf;
    std::unique_ptr<Content> content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustAHeadEditor)
};
