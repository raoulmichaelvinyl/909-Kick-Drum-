#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "KickEngine.h"
#include "Presets.h"

class XeiaFuProcessor : public juce::AudioProcessor
{
public:
    XeiaFuProcessor();
    ~XeiaFuProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return xfu::kNumPresets; }
    int getCurrentProgram() override { return currentProgram.load(); }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Called by the editor's HIT pad (message thread → audio thread, lock-free).
    void triggerFromUI() { pendingUiHits.fetch_add (1); }

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<bool> hitFlash { false }; // read by the editor to flash the LED

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void handleMidi (const juce::MidiMessage& m);
    void triggerNote (int note, float velocity); // note < 0 = use knob settings
    void updateDrive();

    xfu::KickEngine engine;
    std::atomic<int> currentProgram { 0 };
    std::atomic<int> pendingUiHits { 0 };
    float modWheel = 0.0f;

    std::atomic<float>* pTune = nullptr;
    std::atomic<float>* pTuneX2 = nullptr;
    std::atomic<float>* pDecay = nullptr;
    std::atomic<float>* pDecayX2 = nullptr;
    std::atomic<float>* pSweep = nullptr;
    std::atomic<float>* pAttack = nullptr;
    std::atomic<float>* pDrive = nullptr;
    std::atomic<float>* pLevel = nullptr;
    std::atomic<float>* pVelCurve = nullptr;
    std::atomic<float>* pMidiMode = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XeiaFuProcessor)
};
