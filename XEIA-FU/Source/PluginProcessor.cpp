#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int kChromaticRootNote = 36; // C1 plays the knob tuning in CHROMATIC mode
}

juce::AudioProcessorValueTreeState::ParameterLayout XeiaFuProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    const StringArray kVelCurves { "LIN", "EXP", "LOG", "FIX" };
    const StringArray kMidiModes { "PRESET", "CHROMATIC" };

    auto hz = AudioParameterFloatAttributes().withLabel ("Hz")
                  .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " Hz"; });
    auto ms = AudioParameterFloatAttributes().withLabel ("ms")
                  .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " ms"; });
    auto pct = AudioParameterFloatAttributes()
                  .withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; });
    auto db = AudioParameterFloatAttributes().withLabel ("dB")
                  .withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; });

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "tune", 1 }, "Tune",
                     NormalisableRange<float> (30.0f, 70.0f, 0.01f), 48.0f, hz));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "tuneX2", 1 }, "Tune x2", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "decay", 1 }, "Decay",
                     NormalisableRange<float> (40.0f, 800.0f, 0.1f, 0.5f), 320.0f, ms));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { "decayX2", 1 }, "Decay x2", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "sweep", 1 }, "Sweep",
                     NormalisableRange<float> (0.0f, 1.0f), 0.55f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack",
                     NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "drive", 1 }, "Drive",
                     NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "level", 1 }, "Level",
                     NormalisableRange<float> (-24.0f, 6.0f, 0.1f), -3.0f, db));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "velCurve", 1 }, "Velocity Curve", kVelCurves, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "midiMode", 1 }, "MIDI Mode", kMidiModes, 0));

    return { p.begin(), p.end() };
}

XeiaFuProcessor::XeiaFuProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pTune     = apvts.getRawParameterValue ("tune");
    pTuneX2   = apvts.getRawParameterValue ("tuneX2");
    pDecay    = apvts.getRawParameterValue ("decay");
    pDecayX2  = apvts.getRawParameterValue ("decayX2");
    pSweep    = apvts.getRawParameterValue ("sweep");
    pAttack   = apvts.getRawParameterValue ("attack");
    pDrive    = apvts.getRawParameterValue ("drive");
    pLevel    = apvts.getRawParameterValue ("level");
    pVelCurve = apvts.getRawParameterValue ("velCurve");
    pMidiMode = apvts.getRawParameterValue ("midiMode");
}

bool XeiaFuProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void XeiaFuProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    updateDrive();
    engine.setOutputGain (juce::Decibels::decibelsToGain (pLevel->load()));
}

void XeiaFuProcessor::updateDrive()
{
    // Mod wheel (CC1) pushes the clipping from the knob setting up to full.
    const float knob = pDrive->load();
    engine.setDrive (knob + modWheel * (1.0f - knob));
}

void XeiaFuProcessor::triggerNote (int note, float velocity)
{
    xfu::VoiceSettings s;
    s.attack = pAttack->load();
    s.velocity = xfu::applyVelocityCurve (velocity, (int) pVelCurve->load());

    const bool presetMode = pMidiMode->load() < 0.5f;
    const int presetIndex = note - xfu::kPresetFirstNote;

    if (presetMode && presetIndex >= 0 && presetIndex < xfu::kNumPresets)
    {
        const auto& pr = xfu::presets()[(size_t) presetIndex];
        s.baseHz  = pr.tuneHz();
        s.decayMs = pr.decayMs();
        s.sweep   = pr.sweep;
    }
    else
    {
        s.baseHz  = pTune->load()  * (pTuneX2->load()  > 0.5f ? 2.0f : 1.0f);
        s.decayMs = pDecay->load() * (pDecayX2->load() > 0.5f ? 2.0f : 1.0f);
        s.sweep   = pSweep->load();

        if (! presetMode && note >= 0)
            s.baseHz = juce::jlimit (15.0f, 1000.0f,
                                     s.baseHz * std::pow (2.0f, (float) (note - kChromaticRootNote) / 12.0f));
    }

    engine.trigger (s);
    hitFlash.store (true);
}

void XeiaFuProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        triggerNote (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isPitchWheel())
        engine.setPitchBendSemitones ((float) (m.getPitchWheelValue() - 8192) / 8192.0f * 12.0f);
    else if (m.isControllerOfType (1))
    {
        modWheel = (float) m.getControllerValue() / 127.0f;
        updateDrive();
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        engine.chokeAll();
    // Note-offs are ignored: like the hardware, every hit plays its full envelope.
}

void XeiaFuProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    buffer.clear();

    updateDrive();
    engine.setOutputGain (juce::Decibels::decibelsToGain (pLevel->load()));

    if (pendingUiHits.exchange (0) > 0)
        triggerNote (-1, 1.0f);

    float* out = buffer.getWritePointer (0);
    int pos = 0;

    for (const auto meta : midi)
    {
        const int when = juce::jlimit (0, numSamples, meta.samplePosition);
        if (when > pos)
        {
            engine.render (out + pos, when - pos);
            pos = when;
        }
        handleMidi (meta.getMessage());
    }
    if (pos < numSamples)
        engine.render (out + pos, numSamples - pos);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

void XeiaFuProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= xfu::kNumPresets)
        return;

    currentProgram.store (index);
    const auto& pr = xfu::presets()[(size_t) index];

    auto set = [this] (const char* id, float value) {
        if (auto* param = apvts.getParameter (id))
            param->setValueNotifyingHost (param->convertTo0to1 (value));
    };
    set ("tune", pr.tune);
    set ("tuneX2", pr.tuneX2 ? 1.0f : 0.0f);
    set ("decay", pr.decay);
    set ("decayX2", pr.decayX2 ? 1.0f : 0.0f);
    set ("sweep", pr.sweep);
    set ("attack", pr.attack);
    set ("drive", pr.drive);
}

const juce::String XeiaFuProcessor::getProgramName (int index)
{
    if (index < 0 || index >= xfu::kNumPresets)
        return {};
    return xfu::presets()[(size_t) index].name;
}

void XeiaFuProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram.load(), nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void XeiaFuProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram.store ((int) tree.getProperty ("program", 0));
            apvts.replaceState (tree);
        }
    }
}

juce::AudioProcessorEditor* XeiaFuProcessor::createEditor()
{
    return new XeiaFuEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new XeiaFuProcessor();
}
