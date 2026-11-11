#include "PluginEditor.h"

using namespace xfu::colours;

namespace
{
juce::Font uiFont (float height, bool bold = true)
{
    auto opts = juce::FontOptions().withHeight (height);
    if (bold)
        opts = opts.withStyle ("Bold");
    return juce::Font (opts);
}
}

//==============================================================================
XeiaLookAndFeel::XeiaLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panelDk);
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::outlineColourId, ink);
    setColour (juce::ComboBox::arrowColourId, ink);
    setColour (juce::PopupMenu::backgroundColourId, band);
    setColour (juce::PopupMenu::textColourId, panel);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, accent);
    setColour (juce::PopupMenu::highlightedTextColourId, panel);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::BubbleComponent::backgroundColourId, band);
    setColour (juce::BubbleComponent::outlineColourId, accent);
    setColour (juce::TooltipWindow::textColourId, panel);
}

void XeiaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    const float knobR = radius - 11.0f;

    // Scale ticks
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (endAngle - startAngle) * (float) i / 10.0f;
        const bool major = (i % 5 == 0);
        g.setColour (ink.withAlpha (major ? 0.8f : 0.45f));
        g.drawLine ({ centre.getPointOnCircumference (radius, a),
                      centre.getPointOnCircumference (radius - (major ? 6.0f : 4.0f), a) },
                    major ? 2.0f : 1.0f);
    }

    // Value arc
    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, knobR + 4.0f, knobR + 4.0f, 0.0f, startAngle, angle, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Drop shadow
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (centre.x - knobR + 1.5f, centre.y - knobR + 3.0f, knobR * 2.0f, knobR * 2.0f);

    // Knob body
    juce::ColourGradient grad (juce::Colour (0xff3d3d40), centre.x, centre.y - knobR,
                               juce::Colour (0xff101011), centre.x, centre.y + knobR, false);
    g.setGradientFill (grad);
    g.fillEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (juce::Colour (0xff050505));
    g.drawEllipse (centre.x - knobR, centre.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.2f);

    // Inner cap
    const float capR = knobR * 0.62f;
    g.setColour (juce::Colour (0xff2a2a2c));
    g.fillEllipse (centre.x - capR, centre.y - capR, capR * 2.0f, capR * 2.0f);

    // Pointer
    g.setColour (panel);
    g.drawLine ({ centre.getPointOnCircumference (knobR * 0.30f, angle),
                  centre.getPointOnCircumference (knobR * 0.88f, angle) }, 3.0f);
}

void XeiaLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    const bool on = b.getToggleState();
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);

    g.setColour (on ? ink : (highlighted ? panelDk.darker (0.05f) : panelDk));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (ink);
    g.drawRoundedRectangle (r, 4.0f, 1.2f);

    const juce::Rectangle<float> ledR (r.getX() + 9.0f, r.getCentreY() - 4.0f, 8.0f, 8.0f);
    if (on)
    {
        g.setColour (accent.withAlpha (0.35f));
        g.fillEllipse (ledR.expanded (3.0f));
    }
    g.setColour (on ? accent : ledOff);
    g.fillEllipse (ledR);

    g.setColour (on ? panel : ink);
    g.setFont (uiFont (11.5f));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (24.0f), juce::Justification::centredLeft);
}

//==============================================================================
void HitPad::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillRoundedRectangle (r.translated (0.0f, pressed ? 1.0f : 3.0f), 6.0f);

    if (pressed)
        r.translate (0.0f, 2.0f);

    g.setColour (pressed ? accent.brighter (0.15f) : accent);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (accent.darker (0.5f));
    g.drawRoundedRectangle (r, 6.0f, 1.5f);

    g.setColour (panel);
    g.setFont (uiFont (20.0f));
    g.drawText ("HIT", r, juce::Justification::centred);
}

//==============================================================================
XeiaFuEditor::XeiaFuEditor (XeiaFuProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    const char* ids[]   = { "tune", "decay", "sweep", "attack", "drive", "level" };
    const char* names[] = { "TUNE", "DECAY", "SWEEP", "ATTACK", "DRIVE", "LEVEL" };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                      juce::MathConstants<float>::pi * 2.75f, true);
        k.slider.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (k.slider);

        k.label.setText (names[i], juce::dontSendNotification);
        k.label.setJustificationType (juce::Justification::centred);
        k.label.setFont (uiFont (13.0f));
        addAndMakeVisible (k.label);

        k.attachment = std::make_unique<SliderAttachment> (proc.apvts, ids[i], k.slider);

        if (auto* param = proc.apvts.getParameter (ids[i]))
            k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    }

    addAndMakeVisible (tuneX2);
    addAndMakeVisible (decayX2);
    tuneX2Att  = std::make_unique<ButtonAttachment> (proc.apvts, "tuneX2", tuneX2);
    decayX2Att = std::make_unique<ButtonAttachment> (proc.apvts, "decayX2", decayX2);

    velBox.addItemList ({ "LIN", "EXP", "LOG", "FIX" }, 1);
    modeBox.addItemList ({ "PRESET", "CHROMATIC" }, 1);
    addAndMakeVisible (velBox);
    addAndMakeVisible (modeBox);
    velAtt  = std::make_unique<ComboAttachment> (proc.apvts, "velCurve", velBox);
    modeAtt = std::make_unique<ComboAttachment> (proc.apvts, "midiMode", modeBox);

    for (auto* l : { &velLabel, &modeLabel })
    {
        l->setFont (uiFont (11.0f));
        l->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*l);
    }
    velLabel.setText ("VELOCITY", juce::dontSendNotification);
    modeLabel.setText ("MIDI MODE", juce::dontSendNotification);

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (juce::String (i + 1).paddedLeft ('0', 2) + "  " + proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    pad.onHit = [this] { proc.triggerFromUI(); };
    addAndMakeVisible (pad);

    setSize (680, 356);
    startTimerHz (30);
}

XeiaFuEditor::~XeiaFuEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void XeiaFuEditor::paint (juce::Graphics& g)
{
    g.fillAll (panel);

    // Header band
    auto header = getLocalBounds().removeFromTop (72);
    g.setColour (band);
    g.fillRect (header);
    g.setColour (accent);
    g.fillRect (header.removeFromBottom (4));

    auto title = uiFont (34.0f);
    title.setExtraKerningFactor (0.06f);
    g.setFont (title);
    g.setColour (panel);
    g.drawText ("XEIA-FU", 24, 8, 260, 40, juce::Justification::centredLeft);

    g.setFont (uiFont (11.0f));
    g.setColour (accent);
    g.drawText ("BASS DRUM SYNTHESIZER", 26, 44, 260, 16, juce::Justification::centredLeft);

    // Hit LED
    if (led > 0.02f)
    {
        g.setColour (accent.withAlpha (0.4f * led));
        g.fillEllipse (ledBounds.expanded (5.0f));
    }
    g.setColour (ledOff.interpolatedWith (accent, led));
    g.fillEllipse (ledBounds);
    g.setColour (juce::Colours::black);
    g.drawEllipse (ledBounds, 1.0f);

    // Divider between knob row and switch row
    g.setColour (ink.withAlpha (0.25f));
    g.drawLine (24.0f, 226.0f, (float) getWidth() - 24.0f, 226.0f, 1.0f);

    // Corner screws
    for (auto pt : { juce::Point<float> (12.0f, 88.0f), juce::Point<float> ((float) getWidth() - 12.0f, 88.0f),
                     juce::Point<float> (12.0f, (float) getHeight() - 12.0f),
                     juce::Point<float> ((float) getWidth() - 12.0f, (float) getHeight() - 12.0f) })
    {
        g.setColour (panelDk.darker (0.2f));
        g.fillEllipse (pt.x - 4.0f, pt.y - 4.0f, 8.0f, 8.0f);
        g.setColour (ink.withAlpha (0.5f));
        g.drawLine (pt.x - 2.5f, pt.y, pt.x + 2.5f, pt.y, 1.0f);
    }

    // Footer hint
    g.setFont (uiFont (10.0f, false));
    g.setColour (ink.withAlpha (0.6f));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("MOD WHEEL \xe2\x86\x92 DRIVE   \xc2\xb7   PITCH BEND \xc2\xb1" "12   \xc2\xb7   PRESET TUNINGS ON C6\xe2\x80\x93" "B7   \xc2\xb7   CHROMATIC ROOT C1")),
                24, getHeight() - 26, getWidth() - 48, 16, juce::Justification::centred);
}

void XeiaFuEditor::resized()
{
    // Header controls
    presetBox.setBounds (getWidth() - 270, 23, 200, 26);
    ledBounds = { (float) getWidth() - 46.0f, 29.0f, 14.0f, 14.0f };

    // Knob row
    const int knobW = 100, knobTop = 92, knobSize = 128;
    const int startX = (getWidth() - knobW * (int) knobs.size()) / 2;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int kx = startX + (int) i * knobW;
        knobs[i].slider.setBounds (kx + (knobW - 96) / 2, knobTop, 96, 96);
        knobs[i].label.setBounds (kx, knobTop + knobSize - 30, knobW, 20);
    }

    // Switch row
    const int rowY = 246;
    tuneX2.setBounds (40, rowY, 100, 30);
    decayX2.setBounds (40, rowY + 38, 100, 30);

    velLabel.setBounds (170, rowY - 4, 110, 16);
    velBox.setBounds (170, rowY + 14, 110, 28);
    modeLabel.setBounds (300, rowY - 4, 140, 16);
    modeBox.setBounds (300, rowY + 14, 140, 28);

    pad.setBounds (getWidth() - 40 - 160, rowY - 4, 160, 76);
}

void XeiaFuEditor::timerCallback()
{
    const float before = led;
    if (proc.hitFlash.exchange (false))
        led = 1.0f;
    else
        led *= 0.82f;
    if (std::abs (before - led) > 0.005f)
        repaint (ledBounds.expanded (8.0f).toNearestInt());

    if (! presetBox.isPopupActive() && presetBox.getSelectedItemIndex() != proc.getCurrentProgram())
        presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
}
