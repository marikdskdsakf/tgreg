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

    // body
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2c0b10), c.x, c.y - r,
                                           juce::Colour(0xff050203), c.x, c.y + r, false));
    g.fillEllipse(c.x - r * 0.76f, c.y - r * 0.76f, r * 1.52f, r * 1.52f);
    g.setColour(silver.withAlpha(0.8f));
    g.drawEllipse(c.x - r * 0.76f, c.y - r * 0.76f, r * 1.52f, r * 1.52f, 1.4f);

    // inner pentagram
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

    // arc track + value
    juce::Path track, val;
    track.addCentredArc(c.x, c.y, r * 0.92f, r * 0.92f, 0.f, startA, endA, true);
    val.addCentredArc(c.x, c.y, r * 0.92f, r * 0.92f, 0.f, startA, ang, true);
    g.setColour(darkRed);
    g.strokePath(track, juce::PathStrokeType(3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour(blood);
    g.strokePath(val, juce::PathStrokeType(3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // pointer (like a blade)
    juce::Path p;
    p.addTriangle(-2.5f, -r * 0.3f, 2.5f, -r * 0.3f, 0.f, -r * 0.72f);
    g.setColour(bone);
    g.fillPath(p, juce::AffineTransform::rotation(ang).translated(c.x, c.y));
}

ZankyoEditor::ZankyoEditor(ZankyoProcessor& p) : AudioProcessorEditor(&p), proc(p)
{
    struct Def { const char* id; const char* en; const juce::CharPointer_UTF8 jp; };
    const Def defs[8] = {
        { "size",     "SIZE",      juce::CharPointer_UTF8("\xe5\xba\x83\xe3\x81\x95") },                      // 広さ
        { "damp",     "DAMPING",   juce::CharPointer_UTF8("\xe6\x8a\x91\xe5\x88\xb6") },                      // 抑制
        { "predelay", "PRE-DELAY", juce::CharPointer_UTF8("\xe5\x89\x8d\xe9\x81\x85\xe5\xbb\xb6") },          // 前遅延
        { "width",    "WIDTH",     juce::CharPointer_UTF8("\xe5\xb9\x85") },                                  // 幅
        { "lowcut",   "LOW CUT",   juce::CharPointer_UTF8("\xe4\xbd\x8e\xe5\x9f\x9f") },                      // 低域
        { "highcut",  "HIGH CUT",  juce::CharPointer_UTF8("\xe9\xab\x98\xe5\x9f\x9f") },                      // 高域
        { "drive",    "DRIVE",     juce::CharPointer_UTF8("\xe6\xad\xaa\xe3\x81\xbf") },                      // 歪み
        { "mix",      "MIX",       juce::CharPointer_UTF8("\xe6\xb7\xb7\xe5\x90\x88") },                      // 混合
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

        k.label.setText(juce::String(defs[i].en) + "\n" + juce::String(defs[i].jp), juce::dontSendNotification);
        k.label.setJustificationType(juce::Justification::centred);
        k.label.setColour(juce::Label::textColourId, bone);
        k.label.setFont(juce::FontOptions(14.f, juce::Font::bold));
        addAndMakeVisible(k.label);

        k.attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.apvts, defs[i].id, k.slider);
    }

    setSize(780, 500);
}

ZankyoEditor::~ZankyoEditor()
{
    for (auto& k : knobs) k.slider.setLookAndFeel(nullptr);
}

void ZankyoEditor::paint(juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();

    // background
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff160508), area.getCentreX(), area.getCentreY(),
                                           juce::Colour(0xff030102), 0.f, 0.f, true));
    g.fillAll();

    // big pentagram in a circle
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

    // double frame
    g.setColour(silver.withAlpha(0.7f));
    g.drawRect(area.reduced(8.f), 1.5f);
    g.setColour(darkRed);
    g.drawRect(area.reduced(13.f), 1.0f);

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
    const float W = area.getWidth(), H = area.getHeight();
    corner(8, 8, 1, 1); corner(W - 8, 8, -1, 1);
    corner(8, H - 8, 1, -1); corner(W - 8, H - 8, -1, -1);

    // title
    g.setColour(bone);
    g.setFont(juce::FontOptions(44.f, juce::Font::bold));
    g.drawText(juce::CharPointer_UTF8("\xe6\xae\x8b\xe9\x9f\xbf"), 30, 20, 140, 60, juce::Justification::centredLeft); // 残響
    g.setColour(blood);
    g.setFont(juce::FontOptions(16.f, juce::Font::bold));
    g.drawText("Z A N K Y O   -   R E V E R B", 30, 72, 400, 20, juce::Justification::centredLeft);

    // blood drips from the top
    struct Drip { float x, len; };
    const Drip drips[] = { { 60, 38 }, { 190, 22 }, { 330, 50 }, { 470, 26 }, { 610, 44 }, { 720, 30 } };
    for (auto& d : drips)
    {
        g.setColour(blood.withAlpha(0.9f));
        g.fillRoundedRectangle(d.x, 0.f, 3.5f, d.len, 1.5f);
        g.fillEllipse(d.x - 2.f, d.len - 3.f, 7.5f, 9.f);
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
