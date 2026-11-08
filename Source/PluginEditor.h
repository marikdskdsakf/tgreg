#pragma once
#include "PluginProcessor.h"
#include <array>

struct GothicLook : public juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startA, float endA, juce::Slider&) override;
};

class ZankyoEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ZankyoEditor(ZankyoProcessor&);
    ~ZankyoEditor() override;
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawEye(juce::Graphics&, int eyeIndex, float open);
    void touch(int knobIndex);

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    ZankyoProcessor& proc;
    GothicLook look;
    std::array<Knob, 8> knobs;

    juce::Point<float> eyeC[2] { { 440.f, 62.f }, { 560.f, 62.f } };
    juce::Point<float> gaze[2];
    float bloodAmt = 0.f, blink = 0.f;
    int blinkTicks = -1, activeKnob = -1;
    bool dragging = false;
    juce::uint32 lastTouchMs = 0;
    float drops[4][2] {};
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZankyoEditor)
};
