#pragma once
#include "PluginProcessor.h"

class CosmicLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

class FeedbackKillerEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit FeedbackKillerEditor (FeedbackKillerProcessor&);
    ~FeedbackKillerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderBackground();

    struct Star { float x, y, size, phase, speed; };

    FeedbackKillerProcessor& proc;
    CosmicLookAndFeel laf;
    juce::Slider amountSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    std::vector<Star> stars;
    juce::Image background;
    float orbit = 0.f;
    int locked = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedbackKillerEditor)
};
