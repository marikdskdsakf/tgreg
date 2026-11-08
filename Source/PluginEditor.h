#pragma once
#include "PluginProcessor.h"
#include <array>

// ---- Gothic knob look ----
struct GothicLook : public juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startA, float endA, juce::Slider&) override;
};

class ZankyoEditor : public juce::AudioProcessorEditor
{
public:
    explicit ZankyoEditor(ZankyoProcessor&);
    ~ZankyoEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    ZankyoProcessor& proc;
    GothicLook look;
    std::array<Knob, 8> knobs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZankyoEditor)
};
