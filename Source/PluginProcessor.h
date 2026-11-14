#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

// bl4de — visualiseur : le son passe sans être modifié.
// Le processeur lit le tempo / la position de l'hôte et détecte kicks, snares et hi-hats.
class Bl4deProcessor : public juce::AudioProcessor
{
public:
    Bl4deProcessor();
    ~Bl4deProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "bl4de"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    // lu par l'éditeur (thread UI)
    std::atomic<double> bpm { 140.0 }, ppq { 0.0 };
    std::atomic<bool>   playing { false };
    std::atomic<float>  level { 0.0f };
    std::atomic<int>    kicks { 0 }, snares { 0 }, hats { 0 }, accents { 0 };

private:
    void analyseHop();

    double sr = 44100.0;
    int hopSize = 512, hopCount = 0;
    float aL = 0, a1 = 0, a2 = 0, aH = 0;     // coefficients passe-bas 1 pôle
    float lp = 0, m1 = 0, m2 = 0, h1 = 0;     // états des filtres
    double accL = 0, accM = 0, accH = 0, accA = 0;
    float prevL = 0, prevM = 0, prevH = 0;
    float peakL = 1e-4f, peakM = 1e-4f, peakH = 1e-4f;
    float meanOn = 0, meanH = 0, meanA = 0;
    double sinceOn = 1.0, sinceH = 1.0, sinceA = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Bl4deProcessor)
};
