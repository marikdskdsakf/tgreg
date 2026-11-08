#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // order: size, damp, predelay, width, mix, lowcut, highcut, drive, shimmer, reverse
    const char* const paramIds[10] = { "size", "damp", "predelay", "width", "mix",
                                       "lowcut", "highcut", "drive", "shimmer", "reverse" };

    struct Preset { const char* name; float v[10]; bool freeze; };

    const Preset presets[] = {
        { "Default",          { 0.65f, 0.50f, 20.f,  1.0f, 0.35f, 80.f,  9000.f,  0.00f, 0.00f, 0.00f }, false },
        { "Crypt",            { 0.80f, 0.65f, 35.f,  1.0f, 0.40f, 120.f, 5500.f,  0.10f, 0.00f, 0.00f }, false },
        { "Haunted Church",   { 0.90f, 0.40f, 60.f,  1.0f, 0.45f, 150.f, 8000.f,  0.05f, 0.35f, 0.00f }, false },
        { "Ghost Choir",      { 0.85f, 0.35f, 45.f,  1.0f, 0.50f, 200.f, 12000.f, 0.00f, 0.80f, 0.00f }, false },
        { "Backwards Spirit", { 0.70f, 0.50f, 0.f,   1.0f, 0.45f, 100.f, 7000.f,  0.10f, 0.00f, 0.70f }, false },
        { "Frozen Void",      { 0.95f, 0.20f, 80.f,  1.0f, 0.60f, 60.f,  6000.f,  0.15f, 0.30f, 0.00f }, true  },
        { "Cursed Room",      { 0.30f, 0.70f, 10.f,  0.7f, 0.30f, 200.f, 4500.f,  0.35f, 0.00f, 0.00f }, false },
        { "Abyss",            { 1.00f, 0.60f, 120.f, 1.0f, 0.70f, 40.f,  3500.f,  0.20f, 0.25f, 0.30f }, false },
    };
    constexpr int numPresets = (int) (sizeof(presets) / sizeof(presets[0]));
}

juce::AudioProcessorValueTreeState::ParameterLayout ZankyoProcessor::createLayout()
{
    using P = juce::AudioParameterFloat;
    using R = juce::NormalisableRange<float>;
    using A = juce::AudioParameterFloatAttributes;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto percent = []
    {
        return A().withStringFromValueFunction([](float v, int) { return juce::String(juce::roundToInt(v * 100.f)) + " %"; })
                  .withValueFromStringFunction([](const juce::String& t) { return t.getFloatValue() / 100.f; });
    };
    auto unit = [](juce::String u)
    {
        return A().withStringFromValueFunction([u](float v, int) { return juce::String(juce::roundToInt(v)) + " " + u; })
                  .withValueFromStringFunction([](const juce::String& t) { return t.getFloatValue(); });
    };

    p.push_back(std::make_unique<P>(juce::ParameterID{ "size", 1 },     "Size",      R(0.f, 1.f), 0.65f, percent()));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "damp", 1 },     "Damping",   R(0.f, 1.f), 0.5f,  percent()));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "predelay", 1 }, "Pre-Delay", R(0.f, 200.f, 0.f, 0.5f), 20.f, unit("ms")));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "width", 1 },    "Width",     R(0.f, 1.f), 1.0f,  percent()));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "mix", 1 },      "Mix",       R(0.f, 1.f), 0.35f, percent()));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "lowcut", 1 },   "Low Cut",   R(20.f, 1000.f, 0.f, 0.4f), 80.f, unit("Hz")));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "highcut", 1 },  "High Cut",  R(1000.f, 20000.f, 0.f, 0.4f), 9000.f, unit("Hz")));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "drive", 1 },    "Drive",     R(0.f, 1.f), 0.f,   percent()));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "shimmer", 1 },  "Shimmer",   R(0.f, 1.f), 0.f,   percent()));
    p.push_back(std::make_unique<P>(juce::ParameterID{ "reverse", 1 },  "Reverse",   R(0.f, 1.f), 0.f,   percent()));
    p.push_back(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{ "freeze", 1 }, "Freeze", false));

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

int ZankyoProcessor::getNumPrograms() { return numPresets; }

const juce::String ZankyoProcessor::getProgramName(int index)
{
    if (index < 0 || index >= numPresets) return {};
    return presets[index].name;
}

void ZankyoProcessor::setCurrentProgram(int index)
{
    if (index < 0 || index >= numPresets) return;
    currentPreset = index;
    const auto& pr = presets[index];
    for (int k = 0; k < 10; ++k)
        if (auto* prm = apvts.getParameter(paramIds[k]))
            prm->setValueNotifyingHost(prm->convertTo0to1(pr.v[k]));
    if (auto* f = apvts.getParameter("freeze"))
        f->setValueNotifyingHost(pr.freeze ? 1.f : 0.f);
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

    shiftL.prepare((int) (0.06 * sampleRate));
    shiftR.prepare((int) (0.06 * sampleRate));
    revL.prepare((int) (0.9 * sampleRate));
    revR.prepare((int) (0.9 * sampleRate));
    fbL = fbR = 0.f;

    shimSm.reset(sampleRate, 0.05);
    revSm.reset(sampleRate, 0.05);
    shimSm.setCurrentAndTargetValue(apvts.getRawParameterValue("shimmer")->load());
    revSm.setCurrentAndTargetValue(apvts.getRawParameterValue("reverse")->load());

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
    const bool freeze = get("freeze") > 0.5f;

    wet.setSize(2, n, false, false, true);
    wet.copyFrom(0, 0, buffer, 0, 0, n);
    wet.copyFrom(1, 0, buffer, chans > 1 ? 1 : 0, 0, n);

    // pre-delay
    preDelay.setDelay((float) (pre * 0.001 * sr));
    juce::dsp::AudioBlock<float> block(wet);
    juce::dsp::ProcessContextReplacing<float> ctx(block);
    preDelay.process(ctx);

    // reverb settings
    juce::Reverb::Parameters rp;
    rp.roomSize = size;
    rp.damping = damp;
    rp.width = width;
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.freezeMode = freeze ? 1.0f : 0.0f;
    reverb.setParameters(rp);

    shimSm.setTargetValue(get("shimmer"));
    revSm.setTargetValue(get("reverse"));

    auto* wl = wet.getWritePointer(0);
    auto* wr = wet.getWritePointer(1);

    for (int i = 0; i < n; ++i)
    {
        const float sh = shimSm.getNextValue();
        const float rv = revSm.getNextValue();

        float l = wl[i] + fbL;
        float r = wr[i] + fbR;
        reverb.processStereo(&l, &r, 1);

        // shimmer: octave up of the tail, partly fed back into the reverb
        const float pl = shiftL.process(l);
        const float pr = shiftR.process(r);
        fbL = std::tanh(pl * sh * 0.5f);
        fbR = std::tanh(pr * sh * 0.5f);
        const float sL = l + pl * sh;
        const float sR = r + pr * sh;

        // reverse swell
        const float bL = revL.process(sL);
        const float bR = revR.process(sR);
        wl[i] = sL * (1.0f - rv * 0.6f) + bL * rv;
        wr[i] = sR * (1.0f - rv * 0.6f) + bR * rv;
    }

    // filters
    lowCut.setCutoffFrequency(lc);
    highCut.setCutoffFrequency(hc);
    lowCut.process(ctx);
    highCut.process(ctx);

    // drive
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

    // dry/wet
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
