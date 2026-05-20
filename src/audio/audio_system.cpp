#include "audio/audio_system.h"

#include "core/logging.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
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

    store(Cue::Fire,             make_wave(0.08f,
        [](float t){ return 800.0f - 1200.0f * t; },
        [](float t){ return 0.45f  * exp_decay(t, 0.04f); }));

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
