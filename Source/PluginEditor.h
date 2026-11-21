#pragma once
#include "PluginProcessor.h"

namespace jahead
{
    const juce::Colour darkPanel   { 0xff1b1e22 };
    const juce::Colour panelEdge   { 0xff3a4148 };
    const juce::Colour headerBg    { 0xff101418 };
    const juce::Colour accent      { 0xffff8b2c };
    const juce::Colour accent2     { 0xff4fd2ff };
    const juce::Colour textLight   { 0xffe8ecef };
    const juce::Colour ring        { 0xff4a5158 };
    constexpr int designW = 760;
    constexpr int designH = 380;
}

//==============================================================================
class JAHeadLookAndFeel : public juce::LookAndFeel_V4
{
public:
    JAHeadLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};

//==============================================================================
class JustAHeadEditor : public juce::AudioProcessorEditor
{
public:
    explicit JustAHeadEditor (JustAHeadProcessor&);
    ~JustAHeadEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    JustAHeadProcessor& processor;
    JAHeadLookAndFeel lookAndFeel;

    juce::ComboBox headBox, pedalBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> headAtt, pedalAtt;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    Knob gainK, bassK, midK, trebleK, presenceK;
    Knob driveK, toneK, levelK, masterK;

    juce::Label title, headLabel, pedalLabel, pedalTitle, headTitle, masterTitle;
    juce::Label hint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JustAHeadEditor)
};
