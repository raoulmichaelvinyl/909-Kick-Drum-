#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

namespace xfu::colours
{
const juce::Colour panel   { 0xffe8e3d6 }; // warm off-white faceplate
const juce::Colour panelDk { 0xffd6d0c2 };
const juce::Colour ink     { 0xff1c1c1e };
const juce::Colour band    { 0xff232325 }; // charcoal header
const juce::Colour accent  { 0xffe2401c }; // signal red
const juce::Colour ledOff  { 0xff7a756b };
}

class XeiaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    XeiaLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
};

// Big momentary pad that fires on mouse-down.
class HitPad : public juce::Component
{
public:
    std::function<void()> onHit;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override { pressed = true; repaint(); if (onHit) onHit(); }
    void mouseUp (const juce::MouseEvent&) override   { pressed = false; repaint(); }
private:
    bool pressed = false;
};

class XeiaFuEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit XeiaFuEditor (XeiaFuProcessor&);
    ~XeiaFuEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    XeiaFuProcessor& proc;
    XeiaLookAndFeel lnf;

    std::array<Knob, 6> knobs;
    juce::ToggleButton tuneX2 { "TUNE x2" }, decayX2 { "DECAY x2" };
    juce::ComboBox velBox, modeBox, presetBox;
    juce::Label velLabel, modeLabel;
    HitPad pad;

    std::unique_ptr<ButtonAttachment> tuneX2Att, decayX2Att;
    std::unique_ptr<ComboAttachment> velAtt, modeAtt;

    float led = 0.0f;
    juce::Rectangle<float> ledBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XeiaFuEditor)
};
