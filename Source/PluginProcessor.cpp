#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

Bl4deProcessor::Bl4deProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

bool Bl4deProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void Bl4deProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    hopSize = sr > 60000.0 ? 1024 : 512;
    auto k = [this] (double f) { return (float) std::exp (-2.0 * juce::MathConstants<double>::pi * f / sr); };
    aL = k (150.0); a1 = k (1200.0); a2 = k (5000.0); aH = k (7000.0);
    lp = m1 = m2 = h1 = 0;
    accL = accM = accH = accA = 0;
    hopCount = 0;

    delayLen = juce::jmax (1, (int) std::round (0.1 * sr));
    latencySeconds.store (delayLen / sr);
    setLatencySamples (delayLen);
    delayLine.assign (2, std::vector<float> ((size_t) delayLen, 0.0f));
    delayPos = 0;
}

void Bl4deProcessor::pushEvent (const Ev& e)
{
    int s1, z1, s2, z2;
    fifo.prepareToWrite (1, s1, z1, s2, z2);
    if (z1 > 0) evBuf[(size_t) s1] = e; else if (z2 > 0) evBuf[(size_t) s2] = e;
    fifo.finishedWrite (z1 + z2);
}

int Bl4deProcessor::popEvents (std::vector<Ev>& out)
{
    int s1, z1, s2, z2;
    fifo.prepareToRead (fifo.getNumReady(), s1, z1, s2, z2);
    for (int i = 0; i < z1; ++i) out.push_back (evBuf[(size_t) (s1 + i)]);
    for (int i = 0; i < z2; ++i) out.push_back (evBuf[(size_t) (s2 + i)]);
    fifo.finishedRead (z1 + z2);
    return z1 + z2;
}

void Bl4deProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // tempo et position de FL Studio / de l'hôte
    double blockPpq = ppq.load(), curBpm = bpm.load();
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm())          { bpm.store (*b); curBpm = *b; }
            if (auto p = pos->getPpqPosition())  { ppq.store (*p); blockPpq = *p; }
            playing.store (pos->getIsPlaying());
        }
    }

    // analyse (le signal n'est pas modifié)
    const int n = buffer.getNumSamples();
    const int chans = juce::jmin (2, buffer.getNumChannels());
    if (chans == 0) return;
    const float* c0 = buffer.getReadPointer (0);
    const float* c1 = chans > 1 ? buffer.getReadPointer (1) : nullptr;

    for (int i = 0; i < n; ++i)
    {
        const float x = c1 ? 0.5f * (c0[i] + c1[i]) : c0[i];
        lp = (1 - aL) * x + aL * lp;
        m1 = (1 - a1) * x + a1 * m1;
        const float hp = x - m1;
        m2 = (1 - a2) * hp + a2 * m2;
        h1 = (1 - aH) * x + aH * h1;
        const float hh = x - h1;
        accL += lp * lp; accM += m2 * m2; accH += hh * hh; accA += x * x;
        if (++hopCount >= hopSize) analyseHop (blockPpq + (double) (i + 1) / sr * curBpm / 60.0);
    }

    // retard du son = latence déclarée (FL Studio compense, tout reste aligné)
    if (delayLen > 0)
    {
        const int nch = juce::jmin (buffer.getNumChannels(), 2);
        int pos = delayPos;
        for (int i = 0; i < n; ++i)
        {
            for (int ch = 0; ch < nch; ++ch)
            {
                float* d = buffer.getWritePointer (ch);
                auto& dl = delayLine[(size_t) ch];
                const float y = dl[(size_t) pos];
                dl[(size_t) pos] = d[i];
                d[i] = y;
            }
            if (++pos >= delayLen) pos = 0;
        }
        delayPos = pos;
    }
}

void Bl4deProcessor::analyseHop (double hopPpq)
{
    const double hs = (double) hopCount;
    const float eL = (float) std::sqrt (accL / hs), eM = (float) std::sqrt (accM / hs),
                eH = (float) std::sqrt (accH / hs), eA = (float) std::sqrt (accA / hs);
    accL = accM = accH = accA = 0; hopCount = 0;

    const double dt = hs / sr;
    sinceOn += dt; sinceH += dt; sinceA += dt;

    const float fL = juce::jmax (0.0f, eL - prevL), fM = juce::jmax (0.0f, eM - prevM), fH = juce::jmax (0.0f, eH - prevH);
    prevL = eL; prevM = eM; prevH = eH;

    // normalisation par bande (pic qui retombe lentement)
    peakL = juce::jmax (peakL * 0.997f, fL, 1e-4f);
    peakM = juce::jmax (peakM * 0.997f, fM, 1e-4f);
    peakH = juce::jmax (peakH * 0.997f, fH, 1e-4f);
    const float nL = fL / peakL, nM = fM / peakM, nH = fH / peakH;

    // kick / snare : flux bas + médium, seuil adaptatif
    const float on = nL * 0.6f + nM * 0.4f;
    if (on > meanOn * 1.9f + 0.12f && sinceOn > 0.19 && eA > 0.003f)
    {
        sinceOn = 0;
        if (nL >= nM) kicks.fetch_add (1); else snares.fetch_add (1);
    }
    else if (nH > meanH * 2.4f + 0.15f && sinceH > 0.06 && eA > 0.002f)
    {
        sinceH = 0;
        hats.fetch_add (1);
    }
    // accents forts : flux sur toutes les bandes, seuil haut -> les sons les plus marquants
    const float nA = (nL + nM * 0.8f + nH * 0.35f) / 2.15f;
    bool accentNow = false;
    if (nA > meanA * 2.0f + 0.22f && sinceA > 0.17 && eA > 0.004f)
    {
        sinceA = 0;
        accentNow = true;
        accents.fetch_add (1);
    }
    meanA += (nA - meanA) * 0.05f;

    // courbes d'énergie par bande (0..1) + accent éventuel, horodatés en ppq
    envPeakL = juce::jmax (envPeakL * 0.9993f, eL, 1e-4f);
    envPeakM = juce::jmax (envPeakM * 0.9993f, eM, 1e-4f);
    envPeakH = juce::jmax (envPeakH * 0.9993f, eH, 1e-4f);
    pushEvent ({ hopPpq, eL / envPeakL, eM / envPeakM, eH / envPeakH, accentNow ? juce::jlimit (0.1f, 1.0f, nA) : 0.0f });

    meanOn += (on - meanOn) * 0.06f;
    meanH  += (nH - meanH) * 0.06f;

    level.store (juce::jlimit (0.0f, 1.0f, eA * 3.0f));
}

juce::AudioProcessorEditor* Bl4deProcessor::createEditor()
{
    return new Bl4deEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Bl4deProcessor();
}
