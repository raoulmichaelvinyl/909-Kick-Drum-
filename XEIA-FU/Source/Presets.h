// XEIA-FU — 24 factory patches.
// In PRESET MIDI mode, notes C6..B7 (MIDI 84..107) play these tunings directly.
#pragma once
#include <array>

namespace xfu
{
struct Preset
{
    const char* name;
    float tune;    // knob value, Hz (30..70)
    float decay;   // knob value, ms (40..800)
    float sweep;   // 0..1
    float attack;  // 0..1
    float drive;   // 0..1
    bool tuneX2;
    bool decayX2;

    float tuneHz() const  { return tuneX2 ? tune * 2.0f : tune; }
    float decayMs() const { return decayX2 ? decay * 2.0f : decay; }
};

constexpr int kNumPresets = 24;
constexpr int kPresetFirstNote = 84; // C6 (C4 = middle C = 60)

inline const std::array<Preset, kNumPresets>& presets()
{
    static const std::array<Preset, kNumPresets> p {{
        { "Classic Floor",   48.0f, 320.0f, 0.55f, 0.50f, 0.00f, false, false },
        { "Tight Punch",     52.0f, 180.0f, 0.60f, 0.65f, 0.10f, false, false },
        { "Long Boom",       42.0f, 700.0f, 0.50f, 0.40f, 0.00f, false, false },
        { "Gabber Lead",     55.0f, 450.0f, 0.70f, 0.70f, 0.85f, false, false },
        { "Hard Techno",     50.0f, 260.0f, 0.65f, 0.60f, 0.40f, false, false },
        { "Deep Sub",        36.0f, 780.0f, 0.35f, 0.30f, 0.00f, false, false },
        { "Rumble Seed",     45.0f, 650.0f, 0.45f, 0.35f, 0.25f, false, true  },
        { "Clicky",          54.0f, 150.0f, 0.50f, 0.95f, 0.05f, false, false },
        { "Distorted Thud",  47.0f, 350.0f, 0.60f, 0.55f, 0.60f, false, false },
        { "Hardcore 180",    58.0f, 300.0f, 0.75f, 0.75f, 0.90f, false, false },
        { "Tonal Kick",      62.0f, 500.0f, 0.40f, 0.45f, 0.30f, false, false },
        { "Short Snap",      60.0f,  90.0f, 0.70f, 0.80f, 0.20f, false, false },
        { "Warehouse",       46.0f, 420.0f, 0.55f, 0.50f, 0.35f, false, false },
        { "Mellow",          44.0f, 380.0f, 0.30f, 0.25f, 0.00f, false, false },
        { "Overdriven",      51.0f, 300.0f, 0.60f, 0.60f, 1.00f, false, false },
        { "High Tom Kick",   40.0f, 260.0f, 0.50f, 0.50f, 0.10f, true,  false },
        { "Frenchcore",      63.0f, 330.0f, 0.80f, 0.80f, 0.95f, false, false },
        { "Minimal",         49.0f, 200.0f, 0.45f, 0.40f, 0.00f, false, false },
        { "Industrial",      43.0f, 480.0f, 0.70f, 0.65f, 0.70f, false, false },
        { "Big Room",        50.0f, 550.0f, 0.60f, 0.55f, 0.15f, false, false },
        { "Pitch Dive",      45.0f, 400.0f, 1.00f, 0.50f, 0.20f, false, false },
        { "Soft Clip",       48.0f, 350.0f, 0.50f, 0.45f, 0.40f, false, false },
        { "Zap",             38.0f, 160.0f, 0.90f, 0.70f, 0.30f, true,  false },
        { "Endless",         41.0f, 700.0f, 0.50f, 0.40f, 0.20f, false, true  },
    }};
    return p;
}
} // namespace xfu
