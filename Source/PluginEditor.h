#pragma once
#include "PluginProcessor.h"
class AuroraEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AuroraEditor(AuroraProcessor&);
    void resized() override;
    void paint(juce::Graphics&) override;
private:
    void timerCallback() override;
    AuroraProcessor& processor;
    struct Control
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };
    std::vector<std::unique_ptr<Control>> controls;
    juce::Label meters;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuroraEditor)
};
