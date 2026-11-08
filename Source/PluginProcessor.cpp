#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout ZankyoProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back(std::make_unique<P>(juce::ParameterID{ "size", 1 },     "Size",      R(0.f, 1.f), 0.65f));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "damp", 1 },     "Damping",   R(0.f, 1.f), 0.5f));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "predelay", 1 }, "Pre-Delay", R(0.f, 200.f, 0.f, 0.5f), 20.f));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "width", 1 },    "Width",     R(0.f, 1.f), 1.0f));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "lowcut", 1 },   "Low Cut",   R(20.f, 1000.f, 0.f, 0.4f), 80.f));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "highcut", 1 },  "High Cut",  R(1000.f, 20000.f, 0.f, 0.4f), 9000.f));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "drive", 1 },    "Drive",     R(0.f, 1.f), 0.f));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "mix", 1 },      "Mix",       R(0.f, 1.f), 0.35f));

    return { p.begin(), p.end() };
}

ZankyoProcessor::ZankyoProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "STATE", createLayout())
{
}

bool ZankyoProcessor::isBusesLayoutSupported(const BusesLayout& l) const
{
    if (l.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    return l.getMainInputChannelSet() == l.getMainOutputChannelSet();
}

void ZankyoProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    reverb.setSampleRate(sampleRate);
    reverb.reset();

    preDelay.setMaximumDelayInSamples((int) (0.25 * sampleRate));
    preDelay.prepare(spec);
    preDelay.reset();

    lowCut.prepare(spec);  lowCut.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highCut.prepare(spec); highCut.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lowCut.reset(); highCut.reset();

    wet.setSize(2, samplesPerBlock);
}

void ZankyoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int chans = juce::jmin(2, buffer.getNumChannels());
    if (chans < 1) return;

    auto get = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    const float size = get("size"), damp = get("damp"), width = get("width");
    const float pre = get("predelay"), lc = get("lowcut"), hc = get("highcut");
    const float drive = get("drive"), mix = get("mix");

    wet.setSize(2, n, false, false, true);
    wet.copyFrom(0, 0, buffer, 0, 0, n);
    wet.copyFrom(1, 0, buffer, chans > 1 ? 1 : 0, 0, n);

    // pre-delay
    preDelay.setDelay((float) (pre * 0.001 * sr));
    juce::dsp::AudioBlock<float> block(wet);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    preDelay.process(ctx);

    // reverb (wet only)
    juce::Reverb::Parameters rp;
    rp.roomSize = size;
    rp.damping = damp;
    rp.width = width;
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.freezeMode = 0.0f;
    reverb.setParameters(rp);
    reverb.processStereo(wet.getWritePointer(0), wet.getWritePointer(1), n);

    // filters
    lowCut.setCutoffFrequency(lc);
    highCut.setCutoffFrequency(hc);
    lowCut.process(ctx);
    highCut.process(ctx);

    // drive (soft saturation on wet)
    if (drive > 0.001f)
    {
        const float g = 1.0f + drive * 8.0f;
        const float norm = 1.0f / std::tanh(g);
        for (int c = 0; c < 2; ++c)
        {
            auto* d = wet.getWritePointer(c);
            for (int i = 0; i < n; ++i)
                d[i] = std::tanh(d[i] * g) * norm;
        }
    }

    // dry/wet mix
    for (int c = 0; c < chans; ++c)
    {
        auto* out = buffer.getWritePointer(c);
        auto* w = wet.getReadPointer(c);
        for (int i = 0; i < n; ++i)
            out[i] = out[i] * (1.0f - mix) + w[i] * mix;
    }
}

juce::AudioProcessorEditor* ZankyoProcessor::createEditor() { return new ZankyoEditor(*this); }

void ZankyoProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void ZankyoProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ZankyoProcessor(); }
