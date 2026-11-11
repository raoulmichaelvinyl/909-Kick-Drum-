// XEIA-FU — 909-style bass drum engine.
// Plain C++ (no JUCE) so the DSP can be tested on its own.
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace xfu
{
constexpr float kTwoPi = 6.28318530717958647692f;

// Settings captured when a hit is triggered.
struct VoiceSettings
{
    float baseHz   = 48.0f;   // resting pitch of the drum
    float decayMs  = 320.0f;  // time for the body to fall roughly 40 dB
    float sweep    = 0.55f;   // amount of pitch drop at the start (0..1)
    float attack   = 0.5f;    // click + noise amount (0..1)
    float velocity = 1.0f;    // gain after the velocity curve
};

// 31-bit maximal-length LFSR (x^31 + x^28 + 1) clocked at 300 kHz, like the
// original noise clock. Each output sample averages all LFSR steps that fell
// inside it (box-filter decimation), which tames aliasing at host rates.
class LfsrNoise
{
public:
    void prepare (double sampleRate)
    {
        step = 300000.0 / sampleRate;
        acc = 0.0;
    }

    float next()
    {
        acc += step;
        int n = 0;
        float sum = 0.0f;
        while (acc >= 1.0)
        {
            acc -= 1.0;
            const uint32_t bit = ((state >> 30) ^ (state >> 27)) & 1u;
            state = ((state << 1) | bit) & 0x7FFFFFFFu;
            sum += (state & 1u) ? 1.0f : -1.0f;
            ++n;
        }
        if (n > 0)
            held = sum / (float) n;
        return held;
    }

private:
    uint32_t state = 0x1F2E3D4Cu;
    double step = 6.25, acc = 0.0;
    float held = 0.0f;
};

// One drum hit: a swept oscillator through RC-style envelopes and a
// transistor-style VCA.
class KickVoice
{
public:
    void prepare (double sampleRate) { fs = (float) sampleRate; }

    void start (const VoiceSettings& s)
    {
        set = s;
        phase = 0.0f;           // phase-aware start: every hit begins at the zero crossing
        active = true;
        choking = false;
        chokeGain = 1.0f;

        amp = 1.0f;
        ampCoef = coef (std::max (0.005f, s.decayMs * 0.001f / 4.6f));

        p1 = 1.0f; p1Coef = coef (0.0022f);   // fast part of the pitch drop
        p2 = 1.0f; p2Coef = coef (0.030f);    // slow "settling" part

        clickEnv = 1.0f; clickCoef = coef (0.00035f);
        noiseEnv = 1.0f; noiseCoef = coef (0.004f);
        noisePrev = 0.0f;
    }

    // Short fade so a new hit can cut the old one off without a pop (the 909 is monophonic).
    void choke()
    {
        if (active && ! choking)
        {
            choking = true;
            chokeCoef = coef (0.003f);
        }
    }

    bool isActive() const { return active; }

    float process (float pitchMul, float noise)
    {
        if (! active)
            return 0.0f;

        // Pitch: two exponential RC drops on top of the resting frequency.
        const float d1 = set.sweep * 4.0f;
        const float d2 = set.sweep * 0.9f;
        float f = set.baseHz * pitchMul * (1.0f + d1 * p1 + d2 * p2);
        f = std::min (f, fs * 0.45f);

        // Oscillator: mostly sine, a little triangle for the 909's slightly pointed tone.
        const float tri = phase < 0.25f ? 4.0f * phase
                        : (phase < 0.75f ? 2.0f - 4.0f * phase : 4.0f * phase - 4.0f);
        const float sine = std::sin (kTwoPi * phase);
        const float osc = 0.25f * tri + 0.75f * sine;

        phase += f / fs;
        if (phase >= 1.0f)
            phase -= 1.0f;

        // Click: a one-sided pulse that pushes in the same direction as the
        // oscillator's first half-cycle (so it reinforces the transient instead
        // of cancelling it), plus a high-passed burst of LFSR noise.
        const float hpNoise = 0.5f * (noise - noisePrev);
        noisePrev = noise;
        const float click = set.attack * (0.9f * clickEnv + 0.35f * noiseEnv * hpNoise);

        // Transistor VCA: soft saturation, normalised so a full-scale body stays near unity.
        float x = osc * amp + click;
        x = std::tanh (1.4f * x) * kVcaNorm;
        x *= set.velocity * chokeGain;

        amp *= ampCoef;
        p1 *= p1Coef;
        p2 *= p2Coef;
        clickEnv *= clickCoef;
        noiseEnv *= noiseCoef;

        if (choking)
        {
            chokeGain *= chokeCoef;
            if (chokeGain < 1.0e-4f)
                active = false;
        }
        if (amp < 1.0e-5f && clickEnv < 1.0e-5f && noiseEnv < 1.0e-5f)
            active = false;

        return x;
    }

private:
    float coef (float tauSeconds) const { return std::exp (-1.0f / (fs * tauSeconds)); }

    static constexpr float kVcaNorm = 1.1352f; // 1 / tanh(1.4)

    VoiceSettings set;
    float fs = 48000.0f;
    float phase = 0.0f;
    bool active = false, choking = false;
    float chokeGain = 1.0f, chokeCoef = 1.0f;
    float amp = 0.0f, ampCoef = 0.0f;
    float p1 = 0.0f, p1Coef = 0.0f, p2 = 0.0f, p2Coef = 0.0f;
    float clickEnv = 0.0f, clickCoef = 0.0f, noiseEnv = 0.0f, noiseCoef = 0.0f;
    float noisePrev = 0.0f;
};

// Holds the voices, the shared noise source and the output diode clipper.
class KickEngine
{
public:
    static constexpr int kVoices = 4;

    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        for (auto& v : voices)
            v.prepare (sampleRate);
        noise.prepare (sampleRate);
        dcX = dcY = 0.0f;
        dcR = 1.0f - (kTwoPi * 5.0f / fs);            // ~5 Hz DC blocker
        smoothCoef = std::exp (-1.0f / (fs * 0.005f)); // 5 ms parameter smoothing
    }

    void trigger (const VoiceSettings& s)
    {
        for (auto& v : voices)
            v.choke();

        int slot = -1;
        for (int i = 0; i < kVoices; ++i)
            if (! voices[(size_t) i].isActive()) { slot = i; break; }
        if (slot < 0)
        {
            slot = next;
            next = (next + 1) % kVoices;
        }
        voices[(size_t) slot].start (s);
    }

    void chokeAll()
    {
        for (auto& v : voices)
            v.choke();
    }

    void setPitchBendSemitones (float semis) { pitchMul = std::pow (2.0f, semis / 12.0f); }
    void setDrive (float d)                  { driveTarget = std::clamp (d, 0.0f, 1.0f); }
    void setOutputGain (float g)             { gainTarget = g; }

    void render (float* out, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            drive = driveTarget + smoothCoef * (drive - driveTarget);
            gain  = gainTarget  + smoothCoef * (gain  - gainTarget);

            const float nz = noise.next();
            float s = 0.0f;
            for (auto& v : voices)
                s += v.process (pitchMul, nz);

            s = diodeClip (s);

            // DC blocker (asymmetric clipping adds offset)
            const float y = s - dcX + dcR * dcY;
            dcX = s;
            dcY = y;

            out[i] = y * gain;
        }
    }

private:
    // Asymmetric exponential "diode" curve: the negative side clips a bit
    // earlier, which adds the even harmonics typical of diode clipping.
    float diodeClip (float x) const
    {
        if (drive <= 0.0005f)
            return x;

        const float g = 1.0f + drive * 14.0f;
        const float u = x * g;
        float y = u >= 0.0f ? (1.0f - std::exp (-u))
                            : -0.8f * (1.0f - std::exp (u / 0.8f));
        y /= (1.0f - std::exp (-g));               // keep level roughly constant as drive rises

        const float wet = std::min (1.0f, drive * 4.0f); // smooth onset at low settings
        return x + wet * (y - x);
    }

    std::array<KickVoice, kVoices> voices;
    LfsrNoise noise;
    float fs = 48000.0f;
    int next = 0;
    float pitchMul = 1.0f;
    float drive = 0.0f, driveTarget = 0.0f;
    float gain = 1.0f, gainTarget = 1.0f;
    float smoothCoef = 0.99f;
    float dcX = 0.0f, dcY = 0.0f, dcR = 0.9995f;
};

// Velocity curves: LIN, EXP, LOG, FIXED
inline float applyVelocityCurve (float v, int curve)
{
    v = std::clamp (v, 0.0f, 1.0f);
    switch (curve)
    {
        case 1:  return v * v;          // EXP: soft hits much quieter
        case 2:  return std::sqrt (v);  // LOG: soft hits still strong
        case 3:  return 1.0f;           // FIXED
        default: return v;              // LIN
    }
}
} // namespace xfu
