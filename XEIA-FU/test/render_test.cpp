// Offline test: renders every factory preset (plus a drive sweep) to demo.wav.
// Build: g++ -O2 -std=c++17 -I../Source render_test.cpp -o render_test
#include "KickEngine.h"
#include "Presets.h"
#include <cstdio>
#include <vector>
#include <cmath>

static void writeWav (const char* path, const std::vector<float>& d, int sr)
{
    FILE* f = std::fopen (path, "wb");
    auto w32 = [&] (uint32_t v) { std::fwrite (&v, 4, 1, f); };
    auto w16 = [&] (uint16_t v) { std::fwrite (&v, 2, 1, f); };
    const uint32_t bytes = (uint32_t) d.size() * 2;
    std::fwrite ("RIFF", 1, 4, f); w32 (36 + bytes); std::fwrite ("WAVEfmt ", 1, 8, f);
    w32 (16); w16 (1); w16 (1); w32 ((uint32_t) sr); w32 ((uint32_t) sr * 2); w16 (2); w16 (16);
    std::fwrite ("data", 1, 4, f); w32 (bytes);
    for (float s : d) { int v = (int) std::lround (std::clamp (s, -1.0f, 1.0f) * 32767.0f); w16 ((uint16_t) (int16_t) v); }
    std::fclose (f);
}

int main()
{
    const int sr = 48000;
    xfu::KickEngine e;
    e.prepare (sr);
    e.setOutputGain (0.7f);
    std::vector<float> out;
    float peak = 0; bool bad = false;

    auto hit = [&] (const xfu::VoiceSettings& s, float drive, float secs) {
        e.setDrive (drive);
        e.trigger (s);
        std::vector<float> buf ((size_t) (secs * sr));
        e.render (buf.data(), (int) buf.size());
        for (float v : buf) { if (! std::isfinite (v)) bad = true; peak = std::max (peak, std::fabs (v)); }
        out.insert (out.end(), buf.begin(), buf.end());
    };

    for (auto& p : xfu::presets())
    {
        xfu::VoiceSettings s { p.tuneHz(), p.decayMs(), p.sweep, p.attack, 1.0f };
        hit (s, p.drive, 0.55f);
    }
    // fast retriggers (choke test)
    xfu::VoiceSettings s;
    for (int i = 0; i < 8; ++i) hit (s, 0.0f, 0.125f);

    writeWav ("demo.wav", out, sr);
    std::printf ("samples=%zu peak=%.3f finite=%s\n", out.size(), peak, bad ? "NO" : "yes");
    return bad ? 1 : 0;
}
