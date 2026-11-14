#include "PluginEditor.h"
#include "BinaryData.h"

namespace
{
    std::optional<juce::WebBrowserComponent::Resource> getResource (const juce::String& url)
    {
        const auto path = url.fromFirstOccurrenceOf ("/", false, false);
        if (path.isEmpty() || path == "index.html")
        {
            const auto* start = reinterpret_cast<const std::byte*> (BinaryData::index_html);
            std::vector<std::byte> data (start, start + BinaryData::index_htmlSize);
            return juce::WebBrowserComponent::Resource { std::move (data), "text/html" };
        }
        return std::nullopt;
    }
}

Bl4deEditor::Bl4deEditor (Bl4deProcessor& p)
    : AudioProcessorEditor (&p),
      proc (p),
      web (juce::WebBrowserComponent::Options{}
              #if JUCE_WINDOWS
               .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
               .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
                                            .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("bl4de")))
              #endif
               .withNativeIntegrationEnabled()
               .withKeepPageLoadedWhenBrowserIsHidden()
               .withEventListener ("strip", [this] (const juce::var& v)
               {
                   setStrip ((bool) v.getProperty ("on", false));
               })
               .withResourceProvider ([] (const juce::String& url) { return getResource (url); }))
{
    addAndMakeVisible (web);
    setResizable (true, true);
    setResizeLimits (480, 110, 1920, 1080);
    setSize (800, 450);
    web.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    startTimerHz (60);
}

Bl4deEditor::~Bl4deEditor()
{
    stopTimer();
}

void Bl4deEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::white);
}

void Bl4deEditor::resized()
{
    web.setBounds (getLocalBounds());
    if (! strip) fullHeight = getHeight();
}

void Bl4deEditor::setStrip (bool on)
{
    if (on == strip) return;
    if (on) { fullHeight = getHeight(); strip = true;  setSize (getWidth(), juce::jmax (110, getWidth() / 4)); }
    else    { strip = false; setSize (getWidth(), juce::jmax (fullHeight, getWidth() * 9 / 16)); }
}

void Bl4deEditor::timerCallback()
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("bpm",     proc.bpm.load());
    o->setProperty ("ppq",     proc.ppq.load());
    o->setProperty ("playing", proc.playing.load());
    o->setProperty ("lvl",     (double) proc.level.load());
    o->setProperty ("k",       proc.kicks.load());
    o->setProperty ("s",       proc.snares.load());
    o->setProperty ("h",       proc.hats.load());
    o->setProperty ("a",       proc.accents.load());
    web.emitEventIfBrowserIsVisible ("host", juce::var (o));
}
