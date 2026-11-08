#pragma once
#include "PluginProcessor.h"
#include <array>
#include <vector>

struct GothicLook : public juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                          float startA, float endA, juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&,
                              bool highlighted, bool down) override;
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
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };
    struct Bat { float x = 0, y = 0, vx = 1, phase = 0, size = 10, baseY = 100; };
    struct Fog { float x = 0, y = 0, r = 200, speed = 10, phase = 0; };

    void timerCallback() override;
    void drawEye(juce::Graphics&, int eyeIndex, float open);
    void drawBat(juce::Graphics&, const Bat&);
    void resetBat(Bat&, bool anywhere);
    void changePreset(int delta);
    void touch(int knobIndex);

    ZankyoProcessor& proc;
    GothicLook look;
    std::array<Knob, 10> knobs;

    juce::ToggleButton freezeButton;
    juce::TextButton presetPrev { "<" }, presetNext { ">" };
    juce::Label presetLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttach;

    juce::Point<float> eyeC[2] { { 470.f, 62.f }, { 580.f, 62.f } };
    juce::Point<float> gaze[2];
    float bloodAmt = 0.f, blink = 0.f, animT = 0.f;
    int blinkTicks = -1, activeKnob = -1;
    bool dragging = false;
    juce::uint32 lastTouchMs = 0;
    float drops[4][2] {};
    Bat bats[3];
    Fog fogs[6];
    std::vector<std::vector<juce::Point<float>>> cracks;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ZankyoEditor)
};
