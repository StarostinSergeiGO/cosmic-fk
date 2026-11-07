#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <cmath>

class FeedbackKillerProcessor : public juce::AudioProcessor
{
public:
    FeedbackKillerProcessor();
    ~FeedbackKillerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Cosmic Feedback Killer"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> activeNotches { 0 };   // для индикатора в редакторе

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int fftOrder   = 12;
    static constexpr int fftSize    = 1 << fftOrder;   // 4096
    static constexpr int hopSize    = 512;
    static constexpr int numBins    = fftSize / 2 + 1;
    static constexpr int maxNotches = 8;

    struct Notch
    {
        bool  active = false;
        float freq = 0.f, gainDb = 0.f, targetDb = 0.f;
        int   hold = 0;
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        double z1[2] { 0, 0 }, z2[2] { 0, 0 };

        inline double process (double x, int c) noexcept
        {
            const double y = b0 * x + z1[c];
            z1[c] = b1 * x - a1 * y + z2[c];
            z2[c] = b2 * x - a2 * y;
            return y;
        }

        void reset()
        {
            active = false; freq = 0.f; gainDb = targetDb = 0.f; hold = 0;
            b0 = 1; b1 = b2 = a1 = a2 = 0;
            z1[0] = z1[1] = z2[0] = z2[1] = 0;
        }

        void setPeaking (double sr, double f, double q, double dB)
        {
            const double A  = std::pow (10.0, dB / 40.0);
            const double w0 = 2.0 * juce::MathConstants<double>::pi * f / sr;
            const double cw = std::cos (w0), sw = std::sin (w0);
            const double alpha = sw / (2.0 * q);
            const double a0 = 1.0 + alpha / A;
            b0 = (1.0 + alpha * A) / a0;
            b1 = (-2.0 * cw) / a0;
            b2 = (1.0 - alpha * A) / a0;
            a1 = (-2.0 * cw) / a0;
            a2 = (1.0 - alpha / A) / a0;
        }
    };

    void analyse (float amount);
    void detect (float amount);
    void registerFeedback (float freqHz);
    void updateNotches (float amount);

    std::atomic<float>* amountParam = nullptr;
    double sampleRate = 44100.0;
    int holdFrames = 100;

    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize>     ring {};
    std::array<float, fftSize>     window {};
    std::array<float, fftSize * 2> fftData {};
    std::array<float, numBins>     db {};
    std::array<int, numBins>       persistOld {}, persistNew {};
    std::array<Notch, maxNotches>  notches;
    float windowSum = 1.f;
    int writePos = 0, hopCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FeedbackKillerProcessor)
};
