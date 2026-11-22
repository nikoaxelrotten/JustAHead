#include "PluginEditor.h"
#include "BinaryData.h"

using namespace jahead;
using APVTS = juce::AudioProcessorValueTreeState;

static juce::Font fnt (float h, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (h, style));
}

//==============================================================================
const Skin& jahead::skinFor (int head)
{
    static const Skin skins[4] =
    {
        // British OD: tolex crema y negro, perillas negras
        { "British OD",     juce::Colour (0xfff08a24), juce::Colour (0xffece4cc), juce::Colour (0xffd3c9ab),
                            juce::Colour (0xff55524d), juce::Colour (0xff0c0c0c), juce::Colour (0xfff3ead0),
                            juce::Colour (0xff231f1a), juce::Colour (0xffa84a12) },
        // Modern 3Ch: negro con cromo
        { "Modern 3Ch",     juce::Colour (0xffcfd8e3), juce::Colour (0xff2a2c30), juce::Colour (0xff131416),
                            juce::Colour (0xfff4f6f8), juce::Colour (0xff737a83), juce::Colour (0xff111111),
                            juce::Colour (0xffc9ced6), juce::Colour (0xffffffff) },
        // UK Iommi: tweed negro, perillas rojas
        { "UK Iommi",       juce::Colour (0xffe0303a), juce::Colour (0xff222326), juce::Colour (0xff101113),
                            juce::Colour (0xffff5a5f), juce::Colour (0xff600610), juce::Colour (0xffffdede),
                            juce::Colour (0xffcfd2d6), juce::Colour (0xffff7077) },
        // American Thick: chapa y brillo azul
        { "American Thick", juce::Colour (0xff27b7ff), juce::Colour (0xff171a1f), juce::Colour (0xff090b0e),
                            juce::Colour (0xff98e6ff), juce::Colour (0xff0a5a82), juce::Colour (0xff04222f),
                            juce::Colour (0xffb9d9ea), juce::Colour (0xff6fd6ff) }
    };
    return skins[juce::jlimit (0, 3, head)];
}

//==============================================================================
struct KnobSlider : public juce::Slider
{
    bool ampStyle = false;
    juce::Colour col { 0xffffffff };
};

JAHeadLookAndFeel::JAHeadLookAndFeel()
{
    setColour (juce::TextButton::textColourOffId, juce::Colour (0xffa7adb8));
    setColour (juce::TextButton::textColourOnId, juce::Colour (0xff101114));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xff5a626d));
}

juce::Font JAHeadLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return fnt (12.0f, juce::Font::bold); }

void JAHeadLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    g.setColour (on ? b.findColour (juce::TextButton::buttonOnColourId)
                    : juce::Colour (0xff1d2026).brighter (over ? 0.12f : 0.0f));
    g.fillRoundedRectangle (r, 6.0f);
    if (! on)
    {
        g.setColour (juce::Colour (0xff2e323a));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        if (b.getProperties().contains ("hint"))
        {
            g.setColour (juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["hint"]));
            g.fillRoundedRectangle (r.getX() + 8.0f, r.getBottom() - 4.0f, r.getWidth() - 16.0f, 2.5f, 1.2f);
        }
    }
}

void JAHeadLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool, bool)
{
    g.setColour (juce::Colour (0xff6a717a));
    for (int i = 0; i < 3; ++i)
    {
        const float o = 5.0f + (float) i * 5.0f;
        g.drawLine ((float) w - 3.0f, (float) h - o, (float) w - o, (float) h - 3.0f, 1.6f);
    }
}

void JAHeadLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float a0, float a1, juce::Slider& s)
{
    auto* ks = dynamic_cast<KnobSlider*> (&s);
    const bool amp = ks != nullptr && ks->ampStyle;
    const auto& sk = skinFor (skin);
    const juce::Colour ringCol = amp ? sk.accent : (ks != nullptr ? ks->col : juce::Colours::white);

    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.0f);
    const float D = juce::jmin (b.getWidth(), b.getHeight());
    auto outer = juce::Rectangle<float> (D, D).withCentre (b.getCentre());
    const auto c = outer.getCentre();
    const float angle = a0 + pos * (a1 - a0);

    // Anillo de valor
    const float ringR = D * 0.5f - 2.5f;
    juce::Path track, value;
    track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, a1, true);
    value.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, a0, angle, true);
    g.setColour (juce::Colour (amp && skin == 0 ? 0x30000000 : 0x40ffffff).withMultipliedAlpha (amp && skin == 0 ? 1.0f : 0.5f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    if (amp)
    {
        g.setColour (ringCol.withAlpha (0.25f));
        g.strokePath (value, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setColour (ringCol);
    g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Cuerpo
    auto body = outer.reduced (D * 0.14f);
    const float d = body.getWidth();
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (body.translated (0.0f, 2.5f));
    if (amp)
        g.setGradientFill (juce::ColourGradient (sk.knobHi, body.getX() + d * 0.3f, body.getY() + d * 0.25f,
                                                 sk.knobLo, body.getRight(), body.getBottom(), true));
    else
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff484c54), body.getX() + d * 0.3f, body.getY() + d * 0.25f,
                                                 juce::Colour (0xff121417), body.getRight(), body.getBottom(), true));
    g.fillEllipse (body);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (body, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.14f));
    g.drawEllipse (body.reduced (1.5f), 1.0f);

    // Indicador
    const float r0 = d * 0.18f, r1 = d * 0.46f;
    const float sx = std::sin (angle), sy = -std::cos (angle);
    g.setColour (amp ? sk.pointer : juce::Colours::white);
    g.drawLine (c.x + sx * r0, c.y + sy * r0, c.x + sx * r1, c.y + sy * r1, amp ? 3.2f : 2.6f);
}

//==============================================================================
// Una perilla con nombre y valor
class KnobUnit : public juce::Component
{
public:
    KnobUnit (APVTS& apvts, const juce::String& pid, const juce::String& nm, int size, bool amp,
              juce::Colour modCol, std::function<juce::String (double)> f)
        : name (nm), knobSize (size), fmt (std::move (f)), labelCol (0xff9aa0aa), valueCol (modCol)
    {
        slider.ampStyle = amp;
        slider.col = modCol;
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (190);
        slider.setScrollWheelEnabled (true);
        addAndMakeVisible (slider);
        att = std::make_unique<APVTS::SliderAttachment> (apvts, pid, slider);
        if (auto* prm = apvts.getParameter (pid))
            slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        slider.onValueChange = [this] { repaint(); };
        setTitle (nm);
    }

    void setColours (juce::Colour label, juce::Colour value) { labelCol = label; valueCol = value; repaint(); }

    void resized() override { slider.setBounds ((getWidth() - knobSize) / 2, 0, knobSize, knobSize); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (labelCol);
        g.setFont (fnt (11.5f, juce::Font::bold));
        g.drawText (name, 0, knobSize + 2, getWidth(), 14, juce::Justification::centred);
        g.setColour (valueCol);
        g.setFont (fnt (11.5f));
        g.drawText (fmt (slider.getValue()), 0, knobSize + 16, getWidth(), 14, juce::Justification::centred);
    }

    int totalHeight() const { return knobSize + 31; }

private:
    KnobSlider slider;
    juce::String name;
    int knobSize;
    std::function<juce::String (double)> fmt;
    juce::Colour labelCol, valueCol;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

static juce::String fmt1 (double v)  { return juce::String (v, 1); }
static juce::String fmtDb (double v) { return juce::String (v, 1) + " dB"; }
static juce::String fmtPct (double v){ return juce::String ((int) std::round (v)) + "%"; }
static juce::String fmtMs (double v) { return juce::String ((int) std::round (v)) + " ms"; }

//==============================================================================
// Fila de botones ligada a un parámetro de tipo lista
class ChoiceStrip : public juce::Component
{
public:
    ChoiceStrip (APVTS& apvts, const juce::String& pid, const juce::StringArray& names,
                 std::vector<juce::Colour> accents, int numCols, int rowHeight, int gapPx, int radioId, bool hint)
        : cols (numCols), rowH (rowHeight), gap (gapPx)
    {
        auto* param = apvts.getParameter (pid);
        for (int i = 0; i < names.size(); ++i)
        {
            auto b = std::make_unique<juce::TextButton> (names[i]);
            b->setClickingTogglesState (true);
            b->setRadioGroupId (radioId);
            const auto ac = accents[(size_t) juce::jmin (i, (int) accents.size() - 1)];
            b->setColour (juce::TextButton::buttonOnColourId, ac);
            b->setColour (juce::TextButton::textColourOnId, ac.getPerceivedBrightness() > 0.55f ? juce::Colour (0xff101114) : juce::Colours::white);
            if (hint) b->getProperties().set ("hint", (juce::int64) ac.getARGB());
            b->onClick = [this, i] { att->setValueAsCompleteGesture ((float) i); };
            addAndMakeVisible (*b);
            buttons.push_back (std::move (b));
        }
        att = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            const int idx = (int) std::round (v);
            for (size_t k = 0; k < buttons.size(); ++k)
                buttons[k]->setToggleState ((int) k == idx, juce::dontSendNotification);
            if (onChange) onChange (idx);
        });
    }

    void start() { att->sendInitialUpdate(); }

    void resized() override
    {
        const int n = (int) buttons.size();
        const int bw = (getWidth() - gap * (cols - 1)) / cols;
        for (int i = 0; i < n; ++i)
            buttons[(size_t) i]->setBounds ((i % cols) * (bw + gap), (i / cols) * (rowH + gap), bw, rowH);
    }

    std::function<void (int)> onChange;

private:
    int cols, rowH, gap;
    std::vector<std::unique_ptr<juce::TextButton>> buttons;
    std::unique_ptr<juce::ParameterAttachment> att;
};

//==============================================================================
class JustAHeadEditor::Content : public juce::Component
{
public:
    Content (JustAHeadProcessor& p, JAHeadLookAndFeel& l, std::function<void (float)> scaleCb)
        : proc (p), laf (l), setScale (std::move (scaleCb))
    {
        const void* data[4]  = { BinaryData::amp_british_jpg, BinaryData::amp_modern_jpg, BinaryData::amp_iommi_jpg, BinaryData::amp_thick_jpg };
        const int   sizes[4] = { BinaryData::amp_british_jpgSize, BinaryData::amp_modern_jpgSize, BinaryData::amp_iommi_jpgSize, BinaryData::amp_thick_jpgSize };
        for (int i = 0; i < 4; ++i) heads[i] = juce::ImageCache::getFromMemory (data[i], sizes[i]);

        // Selector de head
        std::vector<juce::Colour> headCols;
        for (int i = 0; i < 4; ++i) headCols.push_back (skinFor (i).accent);
        headStrip = std::make_unique<ChoiceStrip> (p.apvts, "head", juce::StringArray { "British OD", "Modern 3Ch", "UK Iommi", "American Thick" },
                                                   headCols, 4, 38, 8, 201, true);
        headStrip->setBounds (300, 12, 560, 38);
        addAndMakeVisible (*headStrip);

        // Pedales
        pedalStrip = std::make_unique<ChoiceStrip> (p.apvts, "pedal", juce::StringArray { "Off", "Overdrive", "Distortion" },
                                                    std::vector<juce::Colour> { colPedal }, 3, 30, 6, 202, false);
        pedalStrip->setBounds (42, 108, 264, 30);
        pedalStrip->onChange = [this] (int i) { pedal = i; repaint(); };
        addAndMakeVisible (*pedalStrip);

        // Mod
        modStrip = std::make_unique<ChoiceStrip> (p.apvts, "mod", juce::StringArray { "Off", "Chorus", "Flanger", "Phaser", "Tremolo", "Slicer", "Ring Mod" },
                                                  std::vector<juce::Colour> { colMod }, 4, 28, 6, 203, false);
        modStrip->setBounds (42, 502, 384, 62);
        addAndMakeVisible (*modStrip);

        // Perillas de pedal
        addKnob ("drive", "DRIVE", 42, 350, 88, 56, false, colPedal, fmt1);
        addKnob ("ptone", "TONE", 130, 350, 88, 56, false, colPedal, fmt1);
        addKnob ("level", "LEVEL", 218, 350, 88, 56, false, colPedal, fmtDb);

        // Perillas del amp (en el panel de controles de abajo)
        const char* ids[]   = { "gain", "bass", "mid", "treble", "presence", "master" };
        const char* names[] = { "GAIN", "BASS", "MID", "TREBLE", "PRESENCE", "MASTER" };
        for (int i = 0; i < 6; ++i)
            ampKnobs.push_back (addKnob (ids[i], names[i], 340 + i * 139 + 19, 346, 100, 70, true, juce::Colours::white, i == 5 ? fmtDb : fmt1));

        // Efectos
        addKnob ("mrate",  "RATE",     42 + 0 * 128 + 14, 574, 100, 60, false, colMod, fmt1);
        addKnob ("mdepth", "DEPTH",    42 + 1 * 128 + 14, 574, 100, 60, false, colMod, fmt1);
        addKnob ("mmix",   "MIX",      42 + 2 * 128 + 14, 574, 100, 60, false, colMod, fmtPct);
        addKnob ("dtime",  "TIME",     472 + 0 * 108 + 4, 574, 100, 60, false, colDelay, fmtMs);
        addKnob ("dfb",    "FEEDBACK", 472 + 1 * 108 + 4, 574, 100, 60, false, colDelay, fmtPct);
        addKnob ("dmix",   "MIX",      472 + 2 * 108 + 4, 574, 100, 60, false, colDelay, fmtPct);
        addKnob ("rsize",  "SIZE",     842 + 0 * 108 + 2, 574, 100, 60, false, colReverb, fmt1);
        addKnob ("rdamp",  "DAMP",     842 + 1 * 108 + 2, 574, 100, 60, false, colReverb, fmt1);
        addKnob ("rmix",   "MIX",      842 + 2 * 108 + 2, 574, 100, 60, false, colReverb, fmtPct);

        // Botones de escala
        const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        const char* labels[] = { "75%", "100%", "125%", "150%" };
        for (int i = 0; i < 4; ++i)
        {
            auto& b = scaleButtons[(size_t) i];
            b.setButtonText (labels[i]);
            b.setClickingTogglesState (true);
            b.setRadioGroupId (101);
            b.setBounds (940 + i * 58, 16, 52, 30);
            const float s = scales[i];
            b.onClick = [this, s] { setScale (s); };
            addAndMakeVisible (b);
        }

        headStrip->onChange = [this] (int i) { applySkin (i); };
        headStrip->start();
        pedalStrip->start();
        modStrip->start();
    }

    void updateScaleHighlight (float s)
    {
        const float scales[] = { 0.75f, 1.0f, 1.25f, 1.5f };
        for (int i = 0; i < 4; ++i)
            scaleButtons[(size_t) i].setToggleState (std::abs (scales[i] - s) < 0.02f, juce::dontSendNotification);
    }

    void applySkin (int s)
    {
        skin = juce::jlimit (0, 3, s);
        laf.skin = skin;
        const auto& sk = skinFor (skin);
        for (auto* k : ampKnobs) k->setColours (sk.label, sk.value);
        repaint();
        for (auto* k : ampKnobs) k->repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto& sk = skinFor (skin);

        // Fondo
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1a1c21), 600.0f, 0.0f, juce::Colour (0xff0b0c0f), 600.0f, (float) designH, false));
        g.fillAll();
        g.setGradientFill (juce::ColourGradient (sk.accent.withAlpha (0.13f), 600.0f, -40.0f, sk.accent.withAlpha (0.0f), 600.0f, 420.0f, true));
        g.fillAll();
        g.setColour (juce::Colour (0xff2a2d34));
        g.drawRect (getLocalBounds(), 1);

        // Logo
        {
            const auto f = fnt (22.0f, juce::Font::bold);
            auto width = [&f] (const juce::String& t)
            {
                juce::GlyphArrangement ga;
                ga.addLineOfText (f, t, 0.0f, 0.0f);
                return ga.getBoundingBox (0, -1, true).getWidth() + 3.0f;
            };
            float x = 26.0f;
            g.setFont (f);
            const juce::String parts[] = { "JUST ", "A", " HEAD" };
            for (int i = 0; i < 3; ++i)
            {
                g.setColour (i == 1 ? sk.accent : juce::Colours::white);
                g.drawText (parts[i], juce::Rectangle<float> (x, 14.0f, 200.0f, 30.0f), juce::Justification::centredLeft);
                x += width (parts[i]);
            }
            g.setColour (juce::Colour (0xff868c96));
            g.setFont (fnt (10.5f, juce::Font::bold));
            g.drawText ("AMP SIM  -  VST3", 28, 40, 200, 14, juce::Justification::centredLeft);
            g.drawText ("ESCALA", 862, 16, 70, 30, juce::Justification::centredRight);
        }

        // --- Pedales
        drawModule (g, { 24.0f, 72.0f, 300.0f, 380.0f }, colPedal, "PEDALES");
        drawStompbox (g, 174.0f, 158.0f);

        // --- Amp: imagen del head + panel de controles
        {
            const juce::Rectangle<float> amp (340.0f, 72.0f, 836.0f, 380.0f);
            const float imgH = amp.getWidth() * 400.0f / 1266.0f;
            const juce::Rectangle<float> imgR (amp.getX(), amp.getY(), amp.getWidth(), imgH);
            juce::Path clip;
            clip.addRoundedRectangle (amp.getX(), amp.getY(), amp.getWidth(), amp.getHeight(), 10.0f, 10.0f, true, true, true, true);

            g.saveState();
            g.reduceClipRegion (clip);
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (heads[skin], imgR, juce::RectanglePlacement::stretchToFit);

            const juce::Rectangle<float> strip (amp.getX(), imgR.getBottom(), amp.getWidth(), amp.getBottom() - imgR.getBottom());
            g.setGradientFill (juce::ColourGradient (sk.stripTop, 0.0f, strip.getY(), sk.stripBottom, 0.0f, strip.getBottom(), false));
            g.fillRect (strip);
            g.setColour ((skin == 0 ? juce::Colours::black : juce::Colours::white).withAlpha (0.035f));
            for (float yy = strip.getY() + 2.0f; yy < strip.getBottom(); yy += 3.0f)
                g.drawHorizontalLine ((int) yy, strip.getX(), strip.getRight());
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRect (strip.getX(), strip.getY(), strip.getWidth(), 2.0f);
            g.setColour (sk.accent.withAlpha (0.85f));
            g.fillRect (strip.getX(), strip.getY() + 2.0f, strip.getWidth(), 1.5f);
            g.restoreState();

            g.setColour (juce::Colour (0xff262930));
            g.drawRoundedRectangle (amp.reduced (0.5f), 10.0f, 1.0f);

            // Etiqueta de sección
            const juce::Rectangle<float> tag (amp.getX() + 12.0f, amp.getY() + 10.0f, 190.0f, 22.0f);
            g.setColour (juce::Colours::black.withAlpha (0.6f));
            g.fillRoundedRectangle (tag, 11.0f);
            g.setColour (sk.accent);
            g.setFont (fnt (11.0f, juce::Font::bold));
            g.drawText (juce::String ("AMP  -  ") + juce::String (sk.name).toUpperCase(), tag.toNearestInt(), juce::Justification::centred);
        }

        // --- Efectos
        drawModule (g, { 24.0f, 466.0f, 420.0f, 222.0f }, colMod, "MOD");
        drawModule (g, { 454.0f, 466.0f, 360.0f, 222.0f }, colDelay, "DELAY");
        drawModule (g, { 824.0f, 466.0f, 352.0f, 222.0f }, colReverb, "REVERB");
        g.setColour (juce::Colour (0xff6c727c));
        g.setFont (fnt (11.0f));
        g.drawText (juce::String::fromUTF8 ("Est\xc3\xa9reo ping-pong"), 472, 520, 324, 18, juce::Justification::centred);
        g.drawText (juce::String::fromUTF8 ("Reverb FDN de 8 l\xc3\xadneas"), 842, 520, 316, 18, juce::Justification::centred);
    }

private:
    void drawModule (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col, const juce::String& title)
    {
        g.setColour (juce::Colour (0xff15171c));
        g.fillRoundedRectangle (r, 10.0f);
        g.setColour (juce::Colour (0xff262930));
        g.drawRoundedRectangle (r.reduced (0.5f), 10.0f, 1.0f);
        juce::Path top;
        top.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), 4.0f, 10.0f, 10.0f, true, true, false, false);
        g.setColour (col);
        g.fillPath (top);
        g.setFont (fnt (12.5f, juce::Font::bold));
        g.drawText (title, (int) r.getX() + 18, (int) r.getY() + 12, 200, 18, juce::Justification::centredLeft);
    }

    void drawStompbox (juce::Graphics& g, float cx, float top)
    {
        const juce::Rectangle<float> b (cx - 62.0f, top, 124.0f, 170.0f);
        const juce::Colour body = pedal == 1 ? juce::Colour (0xff3f9d54) : pedal == 2 ? juce::Colour (0xff1b1b1e) : juce::Colour (0xff3a3f46);
        const juce::Colour ink  = pedal == 2 ? colPedal : juce::Colours::white;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (b.translated (0.0f, 4.0f), 10.0f);
        g.setGradientFill (juce::ColourGradient (body.brighter (0.25f), b.getX(), b.getY(), body.darker (0.35f), b.getRight(), b.getBottom(), false));
        g.fillRoundedRectangle (b, 10.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawRoundedRectangle (b.reduced (1.0f), 10.0f, 1.0f);
        for (int i = 0; i < 3; ++i)
        {
            const auto k = juce::Rectangle<float> (20.0f, 20.0f).withCentre ({ b.getX() + 24.0f + (float) i * 38.0f, b.getY() + 24.0f });
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillEllipse (k);
            g.setColour (juce::Colours::white.withAlpha (0.8f));
            g.drawLine (k.getCentreX(), k.getCentreY(), k.getCentreX() + 4.0f, k.getY() + 3.0f, 1.8f);
        }
        g.setColour (ink);
        g.setFont (fnt (13.0f, juce::Font::bold));
        g.drawText (pedal == 1 ? "OVERDRIVE" : pedal == 2 ? "DISTORTION" : "BYPASS", b.withTrimmedTop (56.0f).withHeight (24.0f).toNearestInt(), juce::Justification::centred);
        const auto led = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ b.getCentreX(), b.getY() + 62.0f + 38.0f });
        g.setColour (pedal == 0 ? juce::Colour (0xff4a1a1a) : juce::Colour (0xffff3b30));
        g.fillEllipse (led);
        const auto sw = juce::Rectangle<float> (42.0f, 42.0f).withCentre ({ b.getCentreX(), b.getBottom() - 34.0f });
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f2f5), sw.getX(), sw.getY(), juce::Colour (0xff7d838c), sw.getRight(), sw.getBottom(), false));
        g.fillEllipse (sw);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.drawEllipse (sw, 1.2f);
    }

    KnobUnit* addKnob (const juce::String& pid, const juce::String& name, int x, int y, int w, int size, bool amp,
                       juce::Colour col, std::function<juce::String (double)> fmt)
    {
        auto u = std::make_unique<KnobUnit> (proc.apvts, pid, name, size, amp, col, std::move (fmt));
        u->setBounds (x, y, w, u->totalHeight());
        addAndMakeVisible (*u);
        auto* raw = u.get();
        knobs.push_back (std::move (u));
        return raw;
    }

    JustAHeadProcessor& proc;
    JAHeadLookAndFeel& laf;
    std::function<void (float)> setScale;
    int skin = 0, pedal = 0;
    juce::Image heads[4];
    std::unique_ptr<ChoiceStrip> headStrip, pedalStrip, modStrip;
    std::vector<std::unique_ptr<KnobUnit>> knobs;
    std::vector<KnobUnit*> ampKnobs;
    std::array<juce::TextButton, 4> scaleButtons;
};

//==============================================================================
JustAHeadEditor::JustAHeadEditor (JustAHeadProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    // Leer el tamaño guardado antes de nada: setResizeLimits() dispara resized() y lo pisaría.
    const int savedW = juce::jlimit (600, 1800, (int) proc.apvts.state.getProperty ("uiWidth", designW));
    content = std::make_unique<Content> (p, laf, [this] (float s)
    {
        setSize (juce::roundToInt ((float) designW * s), juce::roundToInt ((float) designH * s));
    });
    content->setLookAndFeel (&laf);
    addAndMakeVisible (*content);

    setResizeLimits (600, 350, 1800, 1050);
    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) designW / (double) designH);

    setSize (savedW, juce::roundToInt ((double) savedW * designH / designW));
}

JustAHeadEditor::~JustAHeadEditor()
{
    content->setLookAndFeel (nullptr);
}

void JustAHeadEditor::resized()
{
    const float s = (float) getWidth() / (float) designW;
    content->setBounds (0, 0, designW, designH);
    content->setTransform (juce::AffineTransform::scale (s));
    content->updateScaleHighlight (s);
    proc.apvts.state.setProperty ("uiWidth", getWidth(), nullptr);
}
