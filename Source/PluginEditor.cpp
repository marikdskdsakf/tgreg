#include "PluginEditor.h"

static const juce::Colour blood  (0xffd0102a);
static const juce::Colour darkRed(0xff3a1015);
static const juce::Colour bone   (0xfff2e8ea);
static const juce::Colour silver (0xffa9a0a5);

static constexpr float kW = 900.f;
static constexpr float kH = 560.f;

// ---------------------------------------------------------------- look & feel
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

void GothicLook::drawToggleButton(juce::Graphics& g, juce::ToggleButton& btn, bool highlighted, bool)
{
    auto b = btn.getLocalBounds().toFloat().reduced(1.f);
    const bool on = btn.getToggleState();
    g.setColour(on ? juce::Colour(0xff5a0a16) : juce::Colour(0xff12040a));
    g.fillRoundedRectangle(b, 3.f);
    g.setColour(on ? blood : silver.withAlpha(highlighted ? 1.f : 0.7f));
    g.drawRoundedRectangle(b, 3.f, 1.4f);
    g.setColour(on ? bone : silver);
    g.setFont(juce::FontOptions(15.f, juce::Font::bold));
    g.drawText(btn.getButtonText(), b, juce::Justification::centred);
}

void GothicLook::drawButtonBackground(juce::Graphics& g, juce::Button& btn, const juce::Colour&,
                                      bool highlighted, bool down)
{
    auto b = btn.getLocalBounds().toFloat().reduced(1.f);
    g.setColour(down ? juce::Colour(0xff5a0a16) : juce::Colour(0xff12040a));
    g.fillRoundedRectangle(b, 3.f);
    g.setColour(highlighted ? blood : silver.withAlpha(0.7f));
    g.drawRoundedRectangle(b, 3.f, 1.4f);
}

// ---------------------------------------------------------------- editor
ZankyoEditor::ZankyoEditor(ZankyoProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    struct Def { const char* id; const char* en; const juce::CharPointer_UTF8 jp; };
    const Def defs[10] = {
        { "size",     "SIZE",      juce::CharPointer_UTF8("\xe5\xba\x83\xe3\x81\x95") },
        { "damp",     "DAMPING",   juce::CharPointer_UTF8("\xe6\x8a\x91\xe5\x88\xb6") },
        { "predelay", "PRE-DELAY", juce::CharPointer_UTF8("\xe5\x89\x8d\xe9\x81\x85\xe5\xbb\xb6") },
        { "width",    "WIDTH",     juce::CharPointer_UTF8("\xe5\xb9\x85") },
        { "mix",      "MIX",       juce::CharPointer_UTF8("\xe6\xb7\xb7\xe5\x90\x88") },
        { "lowcut",   "LOW CUT",   juce::CharPointer_UTF8("\xe4\xbd\x8e\xe5\x9f\x9f") },
        { "highcut",  "HIGH CUT",  juce::CharPointer_UTF8("\xe9\xab\x98\xe5\x9f\x9f") },
        { "drive",    "DRIVE",     juce::CharPointer_UTF8("\xe6\xad\xaa\xe3\x81\xbf") },
        { "shimmer",  "SHIMMER",   juce::CharPointer_UTF8("\xe8\xbc\x9d\xe3\x81\x8d") },
        { "reverse",  "REVERSE",   juce::CharPointer_UTF8("\xe9\x80\x86\xe8\xa1\x8c") },
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

    // freeze
    freezeButton.setButtonText(juce::String("FREEZE   ") + juce::String(juce::CharPointer_UTF8("\xe5\x87\x8d\xe7\xb5\x90")));
    freezeButton.setLookAndFeel(&look);
    addAndMakeVisible(freezeButton);
    freezeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.apvts, "freeze", freezeButton);

    // presets
    for (auto* b : { &presetPrev, &presetNext })
    {
        b->setLookAndFeel(&look);
        b->setColour(juce::TextButton::textColourOffId, bone);
        b->setColour(juce::TextButton::textColourOnId, bone);
        addAndMakeVisible(*b);
    }
    presetPrev.onClick = [this] { changePreset(-1); };
    presetNext.onClick = [this] { changePreset(1); };

    presetLabel.setJustificationType(juce::Justification::centred);
    presetLabel.setColour(juce::Label::textColourId, bone);
    presetLabel.setColour(juce::Label::backgroundColourId, juce::Colour(0xff12040a));
    presetLabel.setColour(juce::Label::outlineColourId, silver.withAlpha(0.6f));
    presetLabel.setFont(juce::FontOptions(15.f, juce::Font::bold));
    presetLabel.setText(proc.getProgramName(proc.getCurrentProgram()), juce::dontSendNotification);
    addAndMakeVisible(presetLabel);

    // animation state
    for (auto& d : drops) { d[0] = rng.nextFloat(); d[1] = std::fmod(d[0] + 0.5f, 1.f); }
    for (auto& b : bats) resetBat(b, true);

    const float fy[6] = { 130.f, 230.f, 330.f, 430.f, 190.f, 510.f };
    for (int i = 0; i < 6; ++i)
    {
        fogs[i].x = rng.nextFloat() * kW;
        fogs[i].y = fy[i];
        fogs[i].r = 140.f + rng.nextFloat() * 120.f;
        fogs[i].speed = 6.f + rng.nextFloat() * 10.f;
        fogs[i].phase = rng.nextFloat() * 6.28f;
    }

    // cracks along the frame
    {
        juce::Random cr(1234);
        struct Spot { float x, y, nx, ny; };
        const Spot spots[] = {
            { 120, 8, 0, 1 }, { 330, 8, 0, 1 }, { 600, 8, 0, 1 }, { 810, 8, 0, 1 },
            { 90, kH - 8, 0, -1 }, { 300, kH - 8, 0, -1 }, { 540, kH - 8, 0, -1 }, { 800, kH - 8, 0, -1 },
            { 8, 180, 1, 0 }, { 8, 330, 1, 0 }, { 8, 470, 1, 0 },
            { kW - 8, 150, -1, 0 }, { kW - 8, 290, -1, 0 }, { kW - 8, 450, -1, 0 }
        };
        for (auto& s : spots)
        {
            std::vector<juce::Point<float>> poly;
            juce::Point<float> cur(s.x, s.y);
            poly.push_back(cur);
            const float a0 = std::atan2(s.ny, s.nx);
            for (int seg = 0; seg < 8; ++seg)
            {
                const float a = a0 + cr.nextFloat() - 0.5f;
                const float len = 8.f + cr.nextFloat() * 10.f;
                cur = cur + juce::Point<float>(std::cos(a) * len, std::sin(a) * len);
                poly.push_back(cur);
                if (seg == 3)
                {
                    std::vector<juce::Point<float>> br;
                    juce::Point<float> bc = cur;
                    br.push_back(bc);
                    const float ba = a0 + (cr.nextBool() ? 1.f : -1.f) * (0.7f + cr.nextFloat() * 0.5f);
                    for (int k = 0; k < 4; ++k)
                    {
                        const float aa = ba + (cr.nextFloat() - 0.5f) * 0.6f;
                        const float l2 = 6.f + cr.nextFloat() * 8.f;
                        bc = bc + juce::Point<float>(std::cos(aa) * l2, std::sin(aa) * l2);
                        br.push_back(bc);
                    }
                    cracks.push_back(br);
                }
            }
            cracks.push_back(poly);
        }
    }

    setSize((int) kW, (int) kH);
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
    freezeButton.setLookAndFeel(nullptr);
    presetPrev.setLookAndFeel(nullptr);
    presetNext.setLookAndFeel(nullptr);
}

void ZankyoEditor::touch(int knobIndex)
{
    activeKnob = knobIndex;
    lastTouchMs = juce::Time::getMillisecondCounter();
}

void ZankyoEditor::changePreset(int delta)
{
    const int n = proc.getNumPrograms();
    const int idx = (proc.getCurrentProgram() + delta + n) % n;
    proc.setCurrentProgram(idx);
    presetLabel.setText(proc.getProgramName(idx), juce::dontSendNotification);
}

void ZankyoEditor::resetBat(Bat& b, bool anywhere)
{
    b.vx = (1.5f + rng.nextFloat() * 2.f) * (rng.nextBool() ? 1.f : -1.f);
    b.baseY = 40.f + rng.nextFloat() * 480.f;
    b.size = 9.f + rng.nextFloat() * 8.f;
    b.phase = rng.nextFloat() * 6.f;
    b.x = anywhere ? rng.nextFloat() * kW : (b.vx > 0.f ? -70.f : kW + 70.f);
    b.y = b.baseY;
}

void ZankyoEditor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounter();
    const bool looking = activeKnob >= 0 && (dragging || (now - lastTouchMs) < 1800);
    animT += 1.0f / 30.0f;

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
            t.x = std::sin(animT * 0.7f + (float) e) * 0.25f;
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

    // blood amount depends on MIX and SIZE
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

    // bats
    for (auto& b : bats)
    {
        b.x += b.vx;
        b.phase += 0.55f;
        b.y = b.baseY + std::sin(b.x * 0.02f) * 22.f;
        if (b.x < -80.f || b.x > kW + 80.f)
            resetBat(b, false);
    }

    // preset name (it can also change from the host)
    const auto name = proc.getProgramName(proc.getCurrentProgram());
    if (presetLabel.getText() != name)
        presetLabel.setText(name, juce::dontSendNotification);

    repaint();
}

void ZankyoEditor::drawBat(juce::Graphics& g, const Bat& b)
{
    const float s = b.size;
    const float flap = std::sin(b.phase);

    juce::Path wing;
    wing.startNewSubPath(b.x, b.y);
    wing.quadraticTo(b.x - s * 0.6f, b.y - s * 0.9f - flap * s * 0.7f,
                     b.x - s * 1.5f, b.y - flap * s * 0.9f + s * 0.2f);
    wing.quadraticTo(b.x - s * 1.0f, b.y + s * 0.1f, b.x - s * 0.9f, b.y + s * 0.5f);
    wing.quadraticTo(b.x - s * 0.5f, b.y + s * 0.1f, b.x - s * 0.4f, b.y + s * 0.45f);
    wing.lineTo(b.x, b.y + s * 0.35f);
    wing.closeSubPath();

    juce::Path both(wing);
    both.addPath(wing, juce::AffineTransform::scale(-1.f, 1.f, b.x, 0.f));
    both.addEllipse(b.x - s * 0.22f, b.y - s * 0.3f, s * 0.44f, s * 0.8f);
    both.addTriangle(b.x - s * 0.2f, b.y - s * 0.25f, b.x - s * 0.1f, b.y - s * 0.55f, b.x, b.y - s * 0.28f);
    both.addTriangle(b.x + s * 0.2f, b.y - s * 0.25f, b.x + s * 0.1f, b.y - s * 0.55f, b.x, b.y - s * 0.28f);

    g.setColour(juce::Colour(0xff1a0508));
    g.fillPath(both);
    g.setColour(juce::Colour(0xff7a1f2c));
    g.strokePath(both, juce::PathStrokeType(1.0f));
    g.setColour(blood);
    g.fillEllipse(b.x - s * 0.14f, b.y - s * 0.12f, 2.f, 2.f);
    g.fillEllipse(b.x + s * 0.14f - 2.f, b.y - s * 0.12f, 2.f, 2.f);
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
    const juce::Rectangle<float> area(0.f, 0.f, kW, kH);

    // background
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff160508), area.getCentreX(), area.getCentreY(),
                                           juce::Colour(0xff030102), 0.f, 0.f, true));
    g.fillAll();

    // drifting fog
    const float fogAlpha = 0.07f + 0.09f * bloodAmt;
    for (auto& f : fogs)
    {
        const float x = std::fmod(f.x + animT * f.speed, kW + 2.f * f.r) - f.r;
        const float y = f.y + std::sin(animT * 0.5f + f.phase) * 18.f;
        const auto fogCol = juce::Colour(0xffb8a0aa);
        g.setGradientFill(juce::ColourGradient(fogCol.withAlpha(fogAlpha), x, y,
                                               fogCol.withAlpha(0.f), x + f.r, y, true));
        g.fillEllipse(x - f.r, y - f.r * 0.45f, f.r * 2.f, f.r * 0.9f);
    }

    // pentagram
    const auto c = area.getCentre().translated(0.f, 40.f);
    const float R = 220.f;
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

    // bats (behind the knobs)
    for (auto& b : bats)
        drawBat(g, b);

    // frame
    g.setColour(silver.withAlpha(0.7f));
    g.drawRect(area.reduced(8.f), 1.5f);
    g.setColour(darkRed);
    g.drawRect(area.reduced(13.f), 1.0f);

    // cracks on the frame: they spread further when there is more blood
    {
        const float frac = 0.35f + 0.65f * bloodAmt;
        const float pulse = 0.55f + 0.25f * std::sin(animT * 1.3f);
        for (auto& cr : cracks)
        {
            const int cnt = juce::jmax(2, (int) std::ceil(frac * (float) cr.size()));
            juce::Path p;
            p.startNewSubPath(cr[0]);
            for (int k = 1; k < cnt && k < (int) cr.size(); ++k)
                p.lineTo(cr[(size_t) k]);
            g.setColour(juce::Colours::black.withAlpha(0.8f));
            g.strokePath(p, juce::PathStrokeType(3.f));
            if (bloodAmt > 0.05f)
            {
                g.setColour(blood.withAlpha(0.6f * bloodAmt));
                g.strokePath(p, juce::PathStrokeType(1.8f));
            }
            g.setColour(silver.withAlpha(pulse));
            g.strokePath(p, juce::PathStrokeType(1.0f));
        }
    }

    // corner ornaments
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
    corner(8, 8, 1, 1); corner(kW - 8, 8, -1, 1);
    corner(8, kH - 8, 1, -1); corner(kW - 8, kH - 8, -1, -1);

    // title
    g.setColour(bone);
    g.setFont(juce::FontOptions(44.f, juce::Font::bold));
    g.drawText(juce::CharPointer_UTF8("\xe6\xae\x8b\xe9\x9f\xbf"), 30, 20, 140, 60, juce::Justification::centredLeft);
    g.setColour(blood);
    g.setFont(juce::FontOptions(16.f, juce::Font::bold));
    g.drawText("Z A N K Y O   -   R E V E R B", 30, 72, 400, 20, juce::Justification::centredLeft);

    // eyes
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
    presetPrev.setBounds(645, 28, 30, 28);
    presetLabel.setBounds(679, 28, 160, 28);
    presetNext.setBounds(843, 28, 30, 28);
    freezeButton.setBounds(645, 64, 228, 30);

    auto area = getLocalBounds().reduced(30);
    area.removeFromTop(90);
    const int cols = 5, rows = 2;
    const int cw = area.getWidth() / cols, ch = area.getHeight() / rows;

    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto cell = juce::Rectangle<int>(area.getX() + (int) (i % cols) * cw,
                                         area.getY() + (int) (i / cols) * ch, cw, ch).reduced(8);
        knobs[i].label.setBounds(cell.removeFromTop(38));
        knobs[i].slider.setBounds(cell);
    }
}
