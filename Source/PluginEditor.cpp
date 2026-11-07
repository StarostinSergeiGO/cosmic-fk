#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour cyan    { 0xff3df5ff };
    const juce::Colour magenta { 0xffff3fb4 };
    const juce::Colour violet  { 0xff8a4dff };

    // Хелпер: точка на окружности (замена отсутствующего Point::getPointOnCircle)
    inline juce::Point<float> pointOnCircle (juce::Point<float> centre, float radius, float angle)
    {
        return { centre.x + radius * std::cos (angle),
                 centre.y + radius * std::sin (angle) };
    }
}

//==============================================================================
void CosmicLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float pos, float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (24.0f);
    const float arcR = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // деления 0..10
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const bool major = (i % 5 == 0);
        const auto p1 = pointOnCircle (c, arcR + 10.0f, a);
        const auto p2 = pointOnCircle (c, arcR + (major ? 19.0f : 15.0f), a);
        const bool lit = ((float) i / 10.0f) <= pos + 0.001f;
        g.setColour (lit ? cyan.withAlpha (0.9f) : juce::Colours::white.withAlpha (0.25f));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 2.0f : 1.2f);
    }

    // дорожка
    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.strokePath (track, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // светящаяся дуга значения
    juce::Path val;
    val.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (magenta.withAlpha (0.06f * (float) (4 - i)));
        g.strokePath (val, juce::PathStrokeType (6.0f + (float) i * 6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    g.setGradientFill (juce::ColourGradient (cyan, c.x - arcR, c.y, magenta, c.x + arcR, c.y, false));
    g.strokePath (val, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // "планета"-тело ручки
    const float br = arcR - 16.0f;
    g.setColour (violet.withAlpha (0.18f));
    g.fillEllipse (c.x - br - 8.0f, c.y - br - 8.0f, 2.0f * (br + 8.0f), 2.0f * (br + 8.0f));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff9a6bff), c.x - br * 0.5f, c.y - br * 0.6f,
                                             juce::Colour (0xff09031c), c.x + br * 0.7f, c.y + br * 0.9f, true));
    g.fillEllipse (c.x - br, c.y - br, 2.0f * br, 2.0f * br);

    g.setGradientFill (juce::ColourGradient (cyan.withAlpha (0.8f), c.x - br, c.y - br,
                                             magenta.withAlpha (0.6f), c.x + br, c.y + br, false));
    g.drawEllipse (c.x - br, c.y - br, 2.0f * br, 2.0f * br, 1.6f);

    // полосы на планете
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawEllipse (c.x - br * 0.62f, c.y - br * 0.62f, br * 1.24f, br * 1.24f, 1.0f);

    // указатель
    const auto i1 = pointOnCircle (c, br * 0.38f, angle);
    const auto i2 = pointOnCircle (c, br * 0.92f, angle);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawLine (i1.x, i1.y, i2.x, i2.y, 7.0f);
    g.setColour (juce::Colours::white);
    g.drawLine (i1.x, i1.y, i2.x, i2.y, 3.0f);

    // "звезда" на конце дуги
    const auto tip = pointOnCircle (c, arcR, angle);
    g.setColour (cyan.withAlpha (0.35f));
    g.fillEllipse (tip.x - 9.0f, tip.y - 9.0f, 18.0f, 18.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (tip.x - 4.0f, tip.y - 4.0f, 8.0f, 8.0f);
}

juce::Label* CosmicLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = juce::LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, juce::Colours::white);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    return l;
}

//==============================================================================
FeedbackKillerEditor::FeedbackKillerEditor (FeedbackKillerProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setSize (420, 560);

    juce::Random rng (2026);
    for (int i = 0; i < 110; ++i)
        stars.push_back ({ rng.nextFloat() * 420.0f, rng.nextFloat() * 560.0f,
                           0.6f + rng.nextFloat() * 1.8f,
                           rng.nextFloat() * juce::MathConstants<float>::twoPi,
                           0.5f + rng.nextFloat() * 2.0f });

    amountSlider.setLookAndFeel (&laf);
    amountSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    amountSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 100, 34);
    amountSlider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                      juce::MathConstants<float>::pi * 2.75f, true);
    amountSlider.setNumDecimalPlacesToDisplay (1);
    amountSlider.setMouseDragSensitivity (250);
    amountSlider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    amountSlider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (amountSlider);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "amount", amountSlider);
    amountSlider.setDoubleClickReturnValue (true, 5.0);

    startTimerHz (30);
}

FeedbackKillerEditor::~FeedbackKillerEditor()
{
    amountSlider.setLookAndFeel (nullptr);
}

void FeedbackKillerEditor::resized()
{
    amountSlider.setBounds (50, 120, 320, 350);
    renderBackground();
}

void FeedbackKillerEditor::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    if (w <= 0 || h <= 0) return;

    background = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (background);

    juce::ColourGradient base (juce::Colour (0xff03000d), 0.0f, 0.0f,
                               juce::Colour (0xff160838), (float) w * 0.5f, (float) h, false);
    base.addColour (0.55, juce::Colour (0xff0b0524));
    g.setGradientFill (base);
    g.fillAll();

    auto nebula = [&] (float cx, float cy, float r, juce::Colour col)
    {
        g.setGradientFill (juce::ColourGradient (col.withAlpha (0.38f), cx, cy, col.withAlpha (0.0f), cx + r, cy, true));
        g.fillEllipse (cx - r, cy - r, 2.0f * r, 2.0f * r);
    };
    nebula (80.0f, 90.0f, 210.0f, juce::Colour (0xff7b2cff));
    nebula (350.0f, 210.0f, 190.0f, juce::Colour (0xffff2a9d));
    nebula (110.0f, 470.0f, 230.0f, juce::Colour (0xff00b4ff));
    nebula (340.0f, 530.0f, 180.0f, juce::Colour (0xff5a1fd0));

    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRect (0, 0, w, h, 1);
}

void FeedbackKillerEditor::timerCallback()
{
    for (auto& s : stars) s.phase += s.speed * 0.07f;
    orbit += 0.025f;
    locked = proc.activeNotches.load();
    repaint();
}

void FeedbackKillerEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (background, 0, 0);

    // мерцающие звёзды
    for (auto& s : stars)
    {
        const float a = 0.35f + 0.65f * (0.5f + 0.5f * std::sin (s.phase));
        if (s.size > 1.7f)
        {
            g.setColour (cyan.withAlpha (a * 0.22f));
            g.fillEllipse (s.x - 2.5f, s.y - 2.5f, s.size + 5.0f, s.size + 5.0f);
        }
        g.setColour (juce::Colours::white.withAlpha (a));
        g.fillEllipse (s.x, s.y, s.size, s.size);
    }

    // орбита с летящим спутником
    const juce::Point<float> centre (210.0f, 278.0f);
    const auto tf = juce::AffineTransform::rotation (-0.42f, centre.x, centre.y);
    juce::Path ring;
    ring.addEllipse (centre.x - 185.0f, centre.y - 62.0f, 370.0f, 124.0f);
    ring.applyTransform (tf);
    g.setColour (cyan.withAlpha (0.16f));
    g.strokePath (ring, juce::PathStrokeType (1.0f));

    float px = centre.x + 185.0f * std::cos (orbit);
    float py = centre.y + 62.0f * std::sin (orbit);
    tf.transformPoint (px, py);
    g.setColour (magenta.withAlpha (0.35f));
    g.fillEllipse (px - 8.0f, py - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colours::white);
    g.fillEllipse (px - 3.0f, py - 3.0f, 6.0f, 6.0f);

    // заголовок
    g.setColour (cyan.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions().withHeight (13.0f).withStyle ("Bold").withKerningFactor (0.7f)));
    g.drawText ("COSMIC", 0, 26, getWidth(), 18, juce::Justification::centred, false);

    g.setGradientFill (juce::ColourGradient (cyan, 60.0f, 0.0f, magenta, 360.0f, 0.0f, false));
    g.setFont (juce::Font (juce::FontOptions().withHeight (30.0f).withStyle ("Bold").withKerningFactor (0.1f)));
    g.drawText ("FEEDBACK KILLER", 0, 46, getWidth(), 40, juce::Justification::centred, false);

    // подпись ручки
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    g.setFont (juce::Font (juce::FontOptions().withHeight (15.0f).withStyle ("Bold").withKerningFactor (0.5f)));
    g.drawText ("AMOUNT", 0, 474, getWidth(), 22, juce::Justification::centred, false);

    // статус
    const bool active = locked > 0;
    const auto col = active ? magenta : cyan;
    const float pulse = 0.55f + 0.45f * std::sin (orbit * 3.0f);
    g.setColour (col.withAlpha (0.25f * pulse));
    g.fillEllipse (72.0f, 514.0f, 18.0f, 18.0f);
    g.setColour (col);
    g.fillEllipse (77.0f, 519.0f, 8.0f, 8.0f);

    g.setColour (col.withAlpha (0.95f));
    g.setFont (juce::Font (juce::FontOptions().withHeight (13.0f).withStyle ("Bold").withKerningFactor (0.2f)));
    g.drawText (active ? "FEEDBACK SUPPRESSED  x" + juce::String (locked) : juce::String ("SCANNING DEEP SPACE"),
                96, 512, 300, 22, juce::Justification::centredLeft, false);
}
