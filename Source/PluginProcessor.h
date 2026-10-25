#pragma once
#include <JuceHeader.h>
#include <array>

class AuroraProcessor : public juce::AudioProcessor
{
public:
    AuroraProcessor();
    void prepareToPlay(double sampleRate, int maximumBlockSize) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Aurora Mastering"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> inputPeak { -100.f }, outputPeak { -100.f }, rmsLevel { -100.f };
private:
    using Filter = juce::dsp::IIR::Filter<float>;
    std::array<Filter, 2> lowShelf, highShelf, lowMid, highMid, hp;
    std::array<float, 2> compEnvelope {};
    juce::AudioBuffer<float> delayBuffer;
    int delayWrite = 0, delayLength = 1;
    double fs = 44100.0;
    float gain(const char* id) const;
    void updateFilters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuroraProcessor)
};
