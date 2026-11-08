#include "PluginEditor.h"

static const juce::Colour blood  (0xffd0102a);
static const juce::Colour darkRed(0xff3a1015);
static const juce::Colour bone   (0xfff2e8ea);
static const juce::Colour silver (0xffa9a0a5);

void GothicLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos,
                                  float startA, float endA, juce::Slider&)
{
    auto b = juce::Rectangle<float>((float) x, (float) y, (float) w, (float) h).reduced(6.f);
    const float r = juce::jmin(b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float ang = startA + pos * (endA - startA);

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2c0b10), c.x, c.y - r,
                                           juce::Colour(0xff050203), c.x, c.y + r, false));
    g.fillEllipse(c.x - r * 0.76f, c.y - r * 0.76f, r * 1.52f, r * 1.52f);
    g.setColour(silver.withAlpha(0.8f));
    g.drawEllipse(c.x - r * 0.76f, c.y - r * 0.76f, r * 1.52f, r * 1.52f, 1.4f);

    juce::Path star;
    for (int i = 0; i < 5; ++i)
    {
        float a = juce::MathConstants<float>::twoPi * (float) ((i * 2) % 5) / 5.f - juce::MathConstants<float>::halfPi;
        auto pt = juce::Point<float>(c.x + std::cos(a) * r * 0.5f, c.y + std::sin(a) * r * 0.5f);
        if (i == 0) star.startNewSubPath(pt); else star.lineTo(pt);
    }
    star.closeSubPath();
    g.setColour(darkRed.brighter(0.4f).withAlpha(0.7f));
    g.strokePath(star, juce::PathStrokeType(1.0f));

    juce::Path track, val;
    track.addCentredArc(c.x, c.y, r * 0.92f, r * 0.92f, 0.f, startA, endA, true);
    val.addCentredArc(c.x, c.y, r * 0.92f, r * 0.92f, 0.f, startA, ang, true);
    g.setColour(darkRed);
    g.strokePath(track, juce::PathStrokeType(3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(blood);
    g.strokePath(val, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path p;
    p.addTriangle(-2.5f, -r * 0.3f, 2.5f, -r * 0.3f, 0.f, -r * 0.72f);
    g.setColour(bone);
    g.fillPath(p, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
}

ZankyoEditor::ZankyoEditor(ZankyoProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    struct Def { const char* id; const char* en; const juce::CharPointer_UTF8 jp; };
    const Def defs[8] = {
        { "size",     "SIZE",      juce::CharPointer_UTF8("\xe5\xba\x83\xe3\x81\x95") },
        { "damp",     "DAMPING",   juce::CharPointer_UTF8("\xe6\x8a\x91\xe5\x88\xb6") },
        { "predelay", "PRE-DELAY", juce::CharPointer_UTF8("\xe5\x89\x8d\xe9\x81\x85\xe5\xbb\xb6") },
        { "width",    "WIDTH",     juce::CharPointer_UTF8("\xe5\xb9\x85") },
        { "lowcut",   "LOW CUT",   juce::CharPointer_UTF8("\xe4\xbd\x8e\xe5\x9f\x9f") },
        { "highcut",  "HIGH CUT",  juce::CharPointer_UTF8("\xe9\xab\x98\xe5\x9f\x9f") },
        { "drive",    "DRIVE",     juce::CharPointer_UTF8("\xe6\xad\xaa\xe3\x81\xbf") },
        { "mix",      "MIX",       juce::CharPointer_UTF8("\xe6\xb7\xb7\xe5\x90\x88") },
    };

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& k = knobs[i];
        k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 18);
        k.slider.setLookAndFeel(&look);
        k.slider.setColour(juce::Slider::textBoxTextColourId, silver);
        k.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k.slider.setDoubleClickReturnValue(true, proc.apvts.getParameter(defs[i].id)->convertFrom0to1(
                                                     proc.apvts.getParameter(defs[i].id)->getDefaultValue()));
        addAndMakeVisible(k.slider);

        const int idx = (int) i;
        k.slider.onDragStart = [this, idx] { dragging = true; touch(idx); };
        k.slider.onDragEnd = [this] { dragging = false; lastTouchMs = juce::Time::getMillisecondCounter(); };
        k.slider.onValueChange = [this, idx] { touch(idx); };

        k.label.setText(juce::String(defs[i].en) + "\n" + juce::String(defs[i].jp), juce::dontSendNotification);
        k.label.setJustificationType(juce::Justification::centred);
        k.label.setColour(juce::Label::textColourId, bone);
        k.label.setFont(juce::FontOptions(14.f, juce::Font::bold));
        addAndMakeVisible(k.label);

        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, defs[i].id, k.slider);
    }

    for (auto& d : drops) { d[0] = rng.nextFloat(); d[1] = std::fmod(d[0] + 0.5f, 1.f); }

    setSize(780, 500);
    startTimerHz(30);
}

ZankyoEditor::~ZankyoEditor()
{
    stopTimer();
    for (auto& k : knobs)
    {
        k.slider.onDragStart = nullptr;
        k.slider.onDragEnd = nullptr;
        k.slider.onValueChange = nullptr;
        k.slider.setLookAndFeel(nullptr);
    }
}

void ZankyoEditor::touch(int knobIndex)
{
    activeKnob = knobIndex;
    lastTouchMs = juce::Time::getMillisecondCounter();
}

void ZankyoEditor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounter();
    const bool looking = activeKnob >= 0 && (dragging || (now - lastTouchMs) < 1800);

    // eyes follow the knob being touched
    for (int e = 0; e < 2; ++e)
    {
        juce::Point<float> t(0.f, 0.f);
        if (looking)
        {
            auto kc = knobs[(size_t) activeKnob].slider.getBounds().getCentre().toFloat();
            t.x = juce::jlimit(-1.f, 1.f, (kc.x - eyeC[e].x) / 160.f);
            t.y = juce::jlimit(0.f, 1.f, (kc.y - eyeC[e].y) / 300.f);
        }
        else
        {
            t.x = std::sin((float) now * 0.0007f + (float) e) * 0.25f;
            t.y = 0.1f;
        }
        gaze[e] += (t - gaze[e]) * 0.25f;
    }

    // blinking
    if (blinkTicks < 0 && rng.nextFloat() < 1.0f / 110.0f)
        blinkTicks = 0;
    if (blinkTicks >= 0)
    {
        ++blinkTicks;
        blink = std::sin(juce::MathConstants<float>::pi * (float) blinkTicks / 8.0f);
        if (blinkTicks >= 8) { blinkTicks = -1; blink = 0.f; }
    }

    // amount of blood depends on MIX and SIZE
    const float mix  = proc.apvts.getRawParameterValue("mix")->load();
    const float size = proc.apvts.getRawParameterValue("size")->load();
    float target = std::sqrt(juce::jlimit(0.f, 1.f, mix) * juce::jlimit(0.f, 1.f, size));
    if (target < 0.03f) target = 0.f;
    bloodAmt += (target - bloodAmt) * 0.1f;

    // falling drops
    const float speed = 0.012f + 0.03f * bloodAmt;
    for (auto& d : drops)
        for (auto& v : d)
        {
            v += speed;
            if (v > 1.f) v -= 1.f;
        }

    repaint();
}

void ZankyoEditor::drawEye(juce::Graphics& g, int eyeIndex, float open)
{
    const auto c = eyeC[eyeIndex];
    const float w = 44.f, h = 20.f * open;

    juce::Path eye;
    eye.startNewSubPath(c.x - w, c.y);
    eye.quadraticTo(c.x, c.y - 2.f * h - 0.01f, c.x + w, c.y);
    eye.quadraticTo(c.x, c.y + 2.f * h + 0.01f, c.x - w, c.y);
    eye.closeSubPath();

    g.setColour(juce::Colour(0xffe8dcde));
    g.fillPath(eye);

    g.saveState();
    g.reduceClipRegion(eye, juce::AffineTransform());

    const juce::Point<float> irisC(c.x + gaze[eyeIndex].x * 17.f, c.y + gaze[eyeIndex].y * 7.f);

    // bloodshot veins, more of them when there is more blood
    static const float vs[8][2] = { { -1.f, -0.2f }, { -1.f, 0.3f }, { -0.7f, -0.8f }, { -0.6f, 0.8f },
                                    { 1.f, -0.2f },  { 1.f, 0.3f },  { 0.7f, -0.8f },  { 0.6f, 0.8f } };
    g.setColour(blood.withAlpha(0.2f + 0.7f * bloodAmt));
    for (auto& v : vs)
    {
        juce::Point<float> s(c.x + v[0] * w, c.y + v[1] * h);
        auto e = s + (irisC - s) * 0.65f;
        g.drawLine(s.x, s.y, e.x, e.y, 1.0f);
    }

    g.setColour(juce::Colour(0xff7a0b1c));
    g.fillEllipse(irisC.x - 15.f, irisC.y - 15.f, 30.f, 30.f);
    g.setColour(juce::Colour(0xff2a0509));
    g.drawEllipse(irisC.x - 15.f, irisC.y - 15.f, 30.f, 30.f, 2.f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(irisC.x - 6.f, irisC.y - 6.f, 12.f, 12.f);
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.fillEllipse(irisC.x - 5.f, irisC.y - 6.f, 4.f, 4.f);

    g.restoreState();

    g.setColour(juce::Colour(0xff0a0204));
    g.strokePath(eye, juce::PathStrokeType(2.5f));
}

void ZankyoEditor::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff160508), area.getCentreX(), area.getCentreY(),
                                           juce::Colour(0xff030102), 0.f, 0.f, true));
    g.fillAll();

    const auto c = area.getCentre().translated(0.f, 40.f);
    const float R = 200.f;
    g.setColour(darkRed.withAlpha(0.45f));
    g.drawEllipse(c.x - R, c.y - R, R * 2.f, R * 2.f, 1.5f);
    g.drawEllipse(c.x - R * 0.93f, c.y - R * 0.93f, R * 1.86f, R * 1.86f, 0.8f);
    juce::Path star;
    for (int i = 0; i < 6; ++i)
    {
        float a = juce::MathConstants<float>::twoPi * (float) ((i * 2) % 5) / 5.f - juce::MathConstants<float>::halfPi;
        auto pt = juce::Point<float>(c.x + std::cos(a) * R, c.y + std::sin(a) * R);
        if (i == 0) star.startNewSubPath(pt); else star.lineTo(pt);
    }
    g.setColour(darkRed.withAlpha(0.55f));
    g.strokePath(star, juce::PathStrokeType(1.4f));

    g.setColour(silver.withAlpha(0.7f));
    g.drawRect(area.reduced(8.f), 1.5f);
    g.setColour(darkRed);
    g.drawRect(area.reduced(13.f), 1.0f);

    auto corner = [&](float x, float y, float sx, float sy)
    {
        juce::Path o;
        o.startNewSubPath(x, y + sy * 30.f);
        o.quadraticTo(x, y, x + sx * 30.f, y);
        o.startNewSubPath(x + sx * 6.f, y + sy * 22.f);
        o.quadraticTo(x + sx * 6.f, y + sy * 6.f, x + sx * 22.f, y + sy * 6.f);
        g.setColour(silver);
        g.strokePath(o, juce::PathStrokeType(1.6f));
        g.setColour(blood);
        g.fillEllipse(x + sx * 4.f - 2.5f, y + sy * 4.f - 2.5f, 5.f, 5.f);
    };
    const float W = area.getWidth(), H = area.getHeight();
    corner(8, 8, 1, 1); corner(W - 8, 8, -1, 1);
    corner(8, H - 8, 1, -1); corner(W - 8, H - 8, -1, -1);

    g.setColour(bone);
    g.setFont(juce::FontOptions(44.f, juce::Font::bold));
    g.drawText(juce::CharPointer_UTF8("\xe6\xae\x8b\xe9\x9f\xbf"), 30, 20, 140, 60, juce::Justification::centredLeft);
    g.setColour(blood);
    g.setFont(juce::FontOptions(16.f, juce::Font::bold));
    g.drawText("Z A N K Y O   -   R E V E R B", 30, 72, 400, 20, juce::Justification::centredLeft);

    const float open = juce::jmax(0.06f, 1.f - blink * 0.94f);
    drawEye(g, 0, open);
    drawEye(g, 1, open);
}

void ZankyoEditor::paintOverChildren(juce::Graphics& g)
{
    if (bloodAmt < 0.02f) return;

    const float baseLen = 20.f + 300.f * bloodAmt;
    const float width = 2.5f + 3.5f * bloodAmt;
    static const float lenMul[4] = { 1.0f, 0.75f, 0.9f, 0.65f };

    for (int t = 0; t < 4; ++t)
    {
        const int e = t / 2;
        const float side = (t % 2 == 0) ? -1.f : 1.f;
        const float sx = eyeC[e].x + side * 17.f;
        const float sy = eyeC[e].y + 15.f;
        const float len = baseLen * lenMul[t];
        auto xAt = [&](float y) { return sx + std::sin(y * 0.05f + (float) t * 1.7f) * 1.3f; };

        juce::Path p;
        p.startNewSubPath(sx, sy);
        for (float y = 6.f; y <= len; y += 6.f)
            p.lineTo(xAt(y), sy + y);

        g.setColour(juce::Colour(0xff8e0a1a));
        g.strokePath(p, juce::PathStrokeType(width, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour(blood);
        g.strokePath(p, juce::PathStrokeType(width * 0.45f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float ex = xAt(len);
        g.setColour(juce::Colour(0xff8e0a1a));
        g.fillEllipse(ex - width * 0.9f, sy + len - width * 0.6f, width * 1.8f, width * 1.8f);

        for (int k = 0; k < 2; ++k)
        {
            const float tt = drops[t][k];
            const float y = sy + tt * (len + 40.f);
            g.setColour(juce::Colour(0xffe01630).withAlpha(1.f - tt * 0.7f));
            g.fillEllipse(xAt(juce::jmin(tt * (len + 40.f), len)) - width * 0.5f, y, width, width * 1.7f);
        }
    }
}

void ZankyoEditor::resized()
{
    auto area = getLocalBounds().reduced(30);
    area.removeFromTop(90);
    const int cols = 4, rows = 2;
    const int cw = area.getWidth() / cols, ch = area.getHeight() / rows;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto cell = juce::Rectangle<int>(area.getX() + (int) (i % cols) * cw,
                                         area.getY() + (int) (i / cols) * ch, cw, ch).reduced(8);
        knobs[i].label.setBounds(cell.removeFromTop(38));
        knobs[i].slider.setBounds(cell);
    }
}
