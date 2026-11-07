#include "PluginProcessor.h"
#include "PluginEditor.h"

FeedbackKillerProcessor::FeedbackKillerProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    amountParam = apvts.getRawParameterValue ("amount");
}

juce::AudioProcessorValueTreeState::ParameterLayout FeedbackKillerProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // AMOUNT 0.0 .. 10.0, шаг 0.1, автоматизируется в FL Studio
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "amount", 1 },
        "Amount",
        juce::NormalisableRange<float> (0.0f, 10.0f, 0.1f),
        5.0f,
        juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })
            .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); })));

    return layout;
}

bool FeedbackKillerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void FeedbackKillerProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    holdFrames = (int) (2.0 * sr / hopSize);   // ~2 сек удержания фильтра после исчезновения писка

    windowSum = 0.f;
    for (int i = 0; i < fftSize; ++i)
    {
        window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) fftSize);
        windowSum += window[(size_t) i];
    }

    ring.fill (0.f);
    persistOld.fill (0);
    persistNew.fill (0);
    for (auto& n : notches) n.reset();
    writePos = 0;
    hopCounter = 0;
    activeNotches.store (0);
}

void FeedbackKillerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh == 0) return;

    const float amount = amountParam->load();
    float* ch[2] { buffer.getWritePointer (0), nCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < n; ++i)
    {
        float mono = ch[0][i];
        if (ch[1] != nullptr) mono = 0.5f * (mono + ch[1][i]);

        ring[(size_t) writePos] = mono;
        writePos = (writePos + 1) & (fftSize - 1);

        if (++hopCounter >= hopSize)
        {
            hopCounter = 0;
            analyse (amount);
        }

        for (int c = 0; c < nCh; ++c)
        {
            double x = ch[c][i];
            for (auto& nt : notches)
                if (nt.active) x = nt.process (x, c);
            ch[c][i] = (float) x;
        }
    }

    for (int c = nCh; c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, n);
}

void FeedbackKillerProcessor::analyse (float amount)
{
    if (amount >= 0.05f)
    {
        for (int i = 0; i < fftSize; ++i)
            fftData[(size_t) i] = ring[(size_t) ((writePos + i) & (fftSize - 1))] * window[(size_t) i];
        std::fill (fftData.begin() + fftSize, fftData.end(), 0.f);

        fft.performRealOnlyForwardTransform (fftData.data(), true);

        const float norm = 2.0f / windowSum;
        for (int k = 0; k < numBins; ++k)
        {
            const float re = fftData[(size_t) (2 * k)];
            const float im = fftData[(size_t) (2 * k + 1)];
            db[(size_t) k] = 20.0f * std::log10 (std::sqrt (re * re + im * im) * norm + 1.0e-9f);
        }

        detect (amount);
    }
    else
    {
        persistOld.fill (0);
    }

    updateNotches (amount);
}

// Ищем узкие устойчивые пики, сильно выступающие над соседним спектром — это и есть "фидбэк".
void FeedbackKillerProcessor::detect (float amount)
{
    const float binHz = (float) sampleRate / (float) fftSize;
    const int kMin = juce::jmax (14, (int) (150.0f / binHz));
    const int kMax = juce::jmin (numBins - 14, (int) (12000.0f / binHz));
    if (kMax <= kMin) return;

    const float thresholdDb = 26.0f - amount * 1.6f;                      // чем больше AMOUNT, тем чувствительнее
    const int required = juce::jmax (4, (int) std::round (16.0f - amount * 1.1f)); // и быстрее реакция

    persistNew.fill (0);

    for (int k = kMin; k <= kMax; ++k)
    {
        const float c = db[(size_t) k];
        if (! (c > db[(size_t) k - 1] && c >= db[(size_t) k + 1] && c > -62.0f))
            continue;

        float sum = 0.f;
        for (int j = 4; j <= 10; ++j)
            sum += db[(size_t) (k - j)] + db[(size_t) (k + j)];
        const float avg = sum / 14.0f;

        if (c - avg < thresholdDb)
            continue;

        const int p = 1 + juce::jmax (persistOld[(size_t) k - 1],
                                      juce::jmax (persistOld[(size_t) k], persistOld[(size_t) k + 1]));
        persistNew[(size_t) k] = juce::jmin (p, 1000);

        if (p >= required)
        {
            const float a = db[(size_t) k - 1], b = c, cc = db[(size_t) k + 1];
            const float denom = a - 2.0f * b + cc;
            const float delta = std::abs (denom) > 1.0e-6f ? 0.5f * (a - cc) / denom : 0.0f;
            registerFeedback (((float) k + juce::jlimit (-0.5f, 0.5f, delta)) * binHz);
        }
    }

    persistOld.swap (persistNew);
}

void FeedbackKillerProcessor::registerFeedback (float freq)
{
    for (auto& n : notches)
    {
        if (n.active && std::abs (freq - n.freq) < n.freq * 0.04f)
        {
            n.hold = holdFrames;
            n.freq = 0.7f * n.freq + 0.3f * freq;
            return;
        }
    }

    for (auto& n : notches)
    {
        if (! n.active)
        {
            n.active = true;
            n.freq = freq;
            n.gainDb = 0.f;
            n.hold = holdFrames;
            return;
        }
    }
}

void FeedbackKillerProcessor::updateNotches (float amount)
{
    const float depth = 28.0f * std::pow (juce::jlimit (0.0f, 1.0f, amount / 10.0f), 0.7f);
    const float q = 32.0f - amount * 1.8f;   // выше AMOUNT — шире и глубже вырез
    int count = 0;

    for (auto& n : notches)
    {
        if (! n.active) continue;

        if (amount < 0.05f) n.hold = 0;

        if (n.hold > 0) { --n.hold; n.targetDb = -depth; }
        else            { n.targetDb = 0.f; }

        const float coef = n.targetDb < n.gainDb ? 0.5f : 0.02f;   // быстро давим, плавно отпускаем
        n.gainDb += (n.targetDb - n.gainDb) * coef;

        if (n.targetDb == 0.f && n.gainDb > -0.3f)
        {
            n.reset();
            continue;
        }

        if (n.gainDb < -0.5f) ++count;
        n.setPeaking (sampleRate, n.freq, q, n.gainDb);
    }

    activeNotches.store (count);
}

juce::AudioProcessorEditor* FeedbackKillerProcessor::createEditor()
{
    return new FeedbackKillerEditor (*this);
}

void FeedbackKillerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void FeedbackKillerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FeedbackKillerProcessor();
}
