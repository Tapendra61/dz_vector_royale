#include "audio/audio_system.h"

#include "core/logging.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

namespace vector::audio {

namespace {

constexpr unsigned kSampleRate    = 44100;
constexpr unsigned kSampleSize    = 16;
constexpr unsigned kChannels      = 1;
constexpr float    kTwoPi         = 6.28318530718f;

// Generic short-burst generator. `freq_hz_at(t)` returns the instantaneous
// frequency at sample-time t (seconds); `amp_at(t)` returns the envelope.
template <typename FreqFn, typename AmpFn>
Wave make_wave(float duration_s, FreqFn freq_hz_at, AmpFn amp_at) {
    const unsigned frame_count = static_cast<unsigned>(duration_s * kSampleRate);
    auto* samples = static_cast<std::int16_t*>(std::malloc(frame_count * sizeof(std::int16_t)));
    float phase = 0.0f;
    for (unsigned i = 0; i < frame_count; ++i) {
        const float t   = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float f   = freq_hz_at(t);
        phase += kTwoPi * f / static_cast<float>(kSampleRate);
        const float v   = std::sin(phase) * amp_at(t);
        samples[i] = static_cast<std::int16_t>(std::clamp(v, -1.0f, 1.0f) * 32000.0f);
    }
    Wave w{};
    w.frameCount = frame_count;
    w.sampleRate = kSampleRate;
    w.sampleSize = kSampleSize;
    w.channels   = kChannels;
    w.data       = samples;
    return w;
}

float exp_decay(float t, float tau) { return std::exp(-t / tau); }

// Weighty cannon thump: a brief noise transient for the click, a mid-band
// sine sweeping from ~520 Hz down to ~90 Hz for the body, and a 60 Hz
// sub-bass layer that carries the chest-thump. Mixed once and clipped.
Wave make_fire_wave() {
    constexpr float dur = 0.20f;
    const unsigned  fc  = static_cast<unsigned>(dur * kSampleRate);
    auto* samples = static_cast<std::int16_t*>(std::malloc(fc * sizeof(std::int16_t)));
    float phase_main = 0.0f;
    float phase_sub  = 0.0f;
    for (unsigned i = 0; i < fc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);

        // Main pitch curve: exponential decay from 520 to ~90 Hz.
        const float main_f = 90.0f + 430.0f * std::exp(-t / 0.035f);
        const float sub_f  = 60.0f;

        phase_main += kTwoPi * main_f / static_cast<float>(kSampleRate);
        phase_sub  += kTwoPi * sub_f  / static_cast<float>(kSampleRate);

        // Initial noise click — very short, gives the percussive punch.
        const float click_env = 0.40f * std::exp(-t / 0.005f);
        const float click     = click_env * (static_cast<float>(std::rand()) /
                                             static_cast<float>(RAND_MAX) * 2.0f - 1.0f);

        // Body — mid sweep + sub-bass, longer decay than the click.
        const float body_env  = 0.70f * std::exp(-t / 0.09f);
        const float body      = body_env * (std::sin(phase_main) * 0.65f
                                          + std::sin(phase_sub)  * 0.55f);

        const float v = std::clamp(click + body, -1.0f, 1.0f);
        samples[i] = static_cast<std::int16_t>(v * 32000.0f);
    }
    Wave w{};
    w.frameCount = fc;
    w.sampleRate = kSampleRate;
    w.sampleSize = kSampleSize;
    w.channels   = kChannels;
    w.data       = samples;
    return w;
}

}  // namespace

AudioSystem::AudioSystem() {
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        VECTOR_WARN("audio: device not ready, cues will be silent");
        return;
    }
    generate_all();
    ready_ = true;
    VECTOR_INFO("audio: ready, {} procedural cues", static_cast<int>(Cue::Count));
}

AudioSystem::~AudioSystem() {
    if (ready_) {
        for (auto& s : sounds_) UnloadSound(s);
        for (auto& w : waves_)  UnloadWave(w);
    }
    CloseAudioDevice();
}

void AudioSystem::generate_all() {
    auto store = [&](Cue c, Wave w) {
        const auto i = static_cast<std::size_t>(c);
        waves_[i]  = w;
        sounds_[i] = LoadSoundFromWave(w);
    };

    store(Cue::Fire, make_fire_wave());

    store(Cue::Hit,              make_wave(0.10f,
        [](float t){ return 220.0f + 80.0f  * std::sin(t * 70.0f); },
        [](float t){ return 0.55f  * exp_decay(t, 0.06f); }));

    store(Cue::Explosion,        make_wave(0.35f,
        [](float t){ return 90.0f  + 60.0f  * std::sin(t * 20.0f); },
        [](float t){ return 0.75f  * exp_decay(t, 0.12f); }));

    store(Cue::PickupAcquired,   make_wave(0.18f,
        [](float t){ return 660.0f + 280.0f * t; },
        [](float t){ return 0.55f  * exp_decay(t, 0.10f); }));

    store(Cue::PowerUpActivated, make_wave(0.22f,
        [](float t){ return 440.0f * std::pow(2.0f, t * 3.5f); },
        [](float t){ return 0.6f   * exp_decay(t, 0.10f); }));

    store(Cue::LowHpHeartbeat,   make_wave(0.16f,
        [](float t){ return 80.0f  + 30.0f * std::sin(t * 25.0f); },
        [](float t){ return 0.50f  * (t < 0.04f ? t / 0.04f : exp_decay(t - 0.04f, 0.08f)); }));
}

void AudioSystem::play(Cue cue, float volume) {
    if (!ready_) return;
    const auto i = static_cast<std::size_t>(cue);
    if (i >= sounds_.size()) return;
    SetSoundVolume(sounds_[i], std::clamp(volume, 0.0f, 1.0f));
    PlaySound(sounds_[i]);
}

}  // namespace vector::audio
