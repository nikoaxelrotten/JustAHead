#include "PluginEditor.h"

using namespace jahead;
using APVTS = juce::AudioProcessorValueTreeState;

static juce::Font fnt (float h, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (h, style));
}

//==============================================================================
JAHeadLookAndFeel::JAHeadLookAndFeel()
{
    setColour (juce::TextButton::textColourOffId, textLight);
    setColour (juce::Label::textColourId, textLight);
    setColour (juce::ComboBox::textColourId, textLight);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff262c32));
    setColour (juce::ComboBox::outlineColourId, panelEdge);
    setColour (juce::PopupMenu::textColourId, textLight);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff20262c));
    setColour (juce::Slider::textBoxTextColourId, textLight);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff232930));
    setColour (juce::Slider::textBoxOutlineColourId, panelEdge);
}

juce::Font JAHeadLookAndFeel::getLabelFont (juce::Label&)
{
    return fnt (12.0f);
}

juce::Font JAHeadLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return fnt (14.0f);
}

void JAHeadLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                          float pos, float a0, float a1, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float d = juce::jmin (b.getWidth(), b.getHeight());
    auto r = juce::Rectangle<float> (d, d).withCentre (b.getCentre());
    const auto centre = r.getCentre();

    g.setColour (ring);
    g.drawEllipse (r, 3.0f);

    const float angle = a0 + pos * (a1 - a0);
    juce::Path arc;
    arc.addArc (r.reduced (3.0f), a0, angle, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (3.0f));

    const float radius = d * 0.5f - 11.0f;
    const juce::Point<float> tip (centre.x + radius * std::cos (angle),
                                  centre.y + radius * std::sin (angle));
    g.setColour (textLight);
    g.drawLine (centre.x, centre.y, tip.x, tip.y, 2.5f);
    g.setColour (accent);
    g.fillEllipse (centre.x - 3.0f, centre.y - 3.0f, 6.0f, 6.0f);
}

//==============================================================================
JustAHeadEditor::JustAHeadEditor (JustAHeadProcessor& p)
    : juce::AudioProcessorEditor (p), processor (p)
{
    setLookAndFeel (&lookAndFeel);
    setSize (designW, designH);

    title.setText ("JustAHead", juce::dontSendNotification);
    title.setFont (fnt (23.0f, juce::Font::bold));
    title.setColour (juce::Label::textColourId, accent);
    addAndMakeVisible (title);

    hint.setText ("Head + pedales internos  •  salida line level (sin cab IR)", juce::dontSendNotification);
    hint.setFont (fnt (11.0f));
    hint.setColour (juce::Label::textColourId, juce::Colour (0xff9aa4ad));
    addAndMakeVisible (hint);

    headTitle.setText ("HEAD", juce::dontSendNotification);
    headTitle.setFont (fnt (12.0f, juce::Font::bold));
    headTitle.setColour (juce::Label::textColourId, accent2);
    addAndMakeVisible (headTitle);

    pedalTitle.setText ("PEDAL", juce::dontSendNotification);
    pedalTitle.setFont (fnt (12.0f, juce::Font::bold));
    pedalTitle.setColour (juce::Label::textColourId, accent);
    addAndMakeVisible (pedalTitle);

    headLabel.setText ("Modelo", juce::dontSendNotification);
    pedalLabel.setText ("Tipo", juce::dontSendNotification);
    addAndMakeVisible (headLabel);
    addAndMakeVisible (pedalLabel);

    headBox.addItemList (juce::StringArray { "British OD", "Modern 3Ch", "UK Iommi", "American Thick" }, 1);
    pedalBox.addItemList (juce::StringArray { "Off", "Overdrive", "Distortion" }, 1);
    addAndMakeVisible (headBox);
    addAndMakeVisible (pedalBox);

    headAtt.reset (new APVTS::ComboBoxAttachment (processor.apvts, "head", headBox));
    pedalAtt.reset (new APVTS::ComboBoxAttachment (processor.apvts, "pedal", pedalBox));

    auto makeKnob = [this] (Knob& k, const juce::String& id, const juce::String& name, float min, float max)
    {
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 18);
        k.slider.setRange (min, max, 0.1);
        addAndMakeVisible (k.slider);
        k.attachment.reset (new APVTS::SliderAttachment (processor.apvts, id, k.slider));

        k.label.setText (name, juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (fnt (11.0f));
        addAndMakeVisible (k.label);
    };

    makeKnob (gainK,     "gain",     "Gain",     0.0f, 30.0f);
    makeKnob (bassK,     "bass",     "Bass",     0.0f, 10.0f);
    makeKnob (midK,      "mid",      "Mid",      0.0f, 10.0f);
    makeKnob (trebleK,   "treble",   "Treble",   0.0f, 10.0f);
    makeKnob (presenceK, "presence", "Presence", 0.0f, 10.0f);
    makeKnob (driveK,    "drive",    "Drive",    0.0f, 24.0f);
    makeKnob (toneK,     "ptone",    "Tone",     0.0f, 10.0f);
    makeKnob (levelK,    "level",    "Level",  -24.0f, 12.0f);

    makeKnob (masterK, "master", "Master", -24.0f, 12.0f);
    masterTitle.setText ("MASTER", juce::dontSendNotification);
    masterTitle.setFont (fnt (12.0f, juce::Font::bold));
    masterTitle.setColour (juce::Label::textColourId, textLight);
    addAndMakeVisible (masterTitle);
}

JustAHeadEditor::~JustAHeadEditor()
{
    setLookAndFeel (nullptr);
}

void JustAHeadEditor::paint (juce::Graphics& g)
{
    g.fillAll (headerBg);

    g.setColour (darkPanel);
    g.fillRoundedRectangle (14.0f, 54.0f, (float) getWidth() - 28.0f, (float) getHeight() - 68.0f, 10.0f);
    g.setColour (panelEdge);
    g.drawRoundedRectangle (14.0f, 54.0f, (float) getWidth() - 28.0f, (float) getHeight() - 68.0f, 10.0f, 1.0f);

    g.setColour (accent);
    g.fillRect (14, 54, getWidth() - 28, 3);

    // División entre pedal y head.
    g.setColour (panelEdge);
    g.drawVerticalLine (300, 70.0f, (float) getHeight() - 24.0f);
}

void JustAHeadEditor::resized()
{
    title.setBounds (22, 10, 220, 30);
    hint.setBounds (248, 16, 490, 20);

    headTitle.setBounds (320, 68, 120, 20);
    pedalTitle.setBounds (32, 68, 120, 20);

    headLabel.setBounds (320, 94, 50, 22);
    headBox.setBounds (370, 92, 190, 26);
    pedalLabel.setBounds (32, 94, 42, 22);
    pedalBox.setBounds (78, 92, 190, 26);

    const int knobSize = 62;
    const int labelH = 16;

    // Pedal knobs (fila debajo del selector).
    const int py = 132;
    const int px[] = { 40, 118, 196 };
    driveK.slider.setBounds (px[0], py, knobSize, knobSize);
    toneK.slider.setBounds (px[1], py, knobSize, knobSize);
    levelK.slider.setBounds (px[2], py, knobSize, knobSize);
    driveK.label.setBounds (px[0] - 6, py + knobSize + 2, knobSize + 12, labelH);
    toneK.label.setBounds (px[1] - 6, py + knobSize + 2, knobSize + 12, labelH);
    levelK.label.setBounds (px[2] - 6, py + knobSize + 2, knobSize + 12, labelH);

    // Head knobs.
    const int hy = 132;
    const int hx[] = { 326, 400, 474, 548, 622 };
    gainK.slider.setBounds (hx[0], hy, knobSize, knobSize);
    bassK.slider.setBounds (hx[1], hy, knobSize, knobSize);
    midK.slider.setBounds (hx[2], hy, knobSize, knobSize);
    trebleK.slider.setBounds (hx[3], hy, knobSize, knobSize);
    presenceK.slider.setBounds (hx[4], hy, knobSize, knobSize);
    gainK.label.setBounds (hx[0] - 6, hy + knobSize + 2, knobSize + 12, labelH);
    bassK.label.setBounds (hx[1] - 6, hy + knobSize + 2, knobSize + 12, labelH);
    midK.label.setBounds (hx[2] - 6, hy + knobSize + 2, knobSize + 12, labelH);
    trebleK.label.setBounds (hx[3] - 6, hy + knobSize + 2, knobSize + 12, labelH);
    presenceK.label.setBounds (hx[4] - 6, hy + knobSize + 2, knobSize + 12, labelH);

    masterTitle.setBounds (32, 232, 100, 18);
    masterK.slider.setBounds (32, 252, knobSize, knobSize);
    masterK.label.setBounds (26, 252 + knobSize + 2, knobSize + 12, labelH);
}
