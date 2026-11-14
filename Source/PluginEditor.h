#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "PluginProcessor.h"

// L'interface est la page web bl4de (canvas) affichée dans une WebView native.
class Bl4deEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Bl4deEditor (Bl4deProcessor&);
    ~Bl4deEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setStrip (bool on);

    Bl4deProcessor& proc;
    bool strip = false;
    int fullHeight = 450;
    juce::WebBrowserComponent web;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Bl4deEditor)
};
