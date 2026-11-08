#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Octave-up pitch shifter (two crossfaded delay taps)
class OctaveUp
{
public:
    void prepare(int windowSamples)
    {
        win = (float) juce::jmax(64, windowSamples);
        size = (int) win * 2 + 8;
        buf.assign((size_t) size, 0.f);
        w = 0;
        phase = 0.f;
    }

    float process(float in)
    {
        buf[(size_t) w] = in;
        float out = 0.f;
        for (int k = 0; k < 2; ++k)
        {
            float ph = phase + 0.5f * (float) k;
            if (ph >= 1.f) ph -= 1.f;
            const float delay = (1.f - ph) * win;
            const float s = std::sin(3.14159265f * ph);
            const float gain = s * s;
            float rp = (float) w - delay;
            while (rp < 0.f) rp += (float) size;
            const int i0 = (int) rp;
            const float fr = rp - (float) i0;
            const int i1 = (i0 + 1) % size;
            out += gain * (buf[(size_t) i0] * (1.f - fr) + buf[(size_t) i1] * fr);
        }
        w = (w + 1) % size;
        phase += 1.f / win;
        if (phase >= 1.f) phase -= 1.f;
        return out;
    }

private:
    std::vector<float> buf;
    float win = 2048.f, phase = 0.f;
    int size = 4104, w = 0;
};

// Reverse swell: records a chunk, plays the previous chunk backwards with a fade window
class ReverseSwell
{
public:
    void prepare(int lengthSamples)
    {
        n = juce::jmax(256, lengthSamples);
        rec.assign((size_t) n, 0.f);
        play.assign((size_t) n, 0.f);
        pos = 0;
    }

    float process(float in)
    {
        rec[(size_t) pos] = in;
        const float win = std::sin(3.14159265f * ((float) pos + 0.5f) / (float) n);
        const float out = play[(size_t) (n - 1 - pos)] * win;
        if (++pos >= n)
        {
            rec.swap(play);
            pos = 0;
        }
        return out;
    }

private:
    std::vector<float> rec, play;
    int n = 44100, pos = 0;
};
