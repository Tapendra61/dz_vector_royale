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

// Compressed-gas release / aerosol-can character: a brief pre-emphasis
// click, then high-passed white noise with a soft decay tail. No tonal
// body — all hiss.
Wave make_fire_wave() {
    constexpr float dur = 0.13f;
    const unsigned  fc  = static_cast<unsigned>(dur * kSampleRate);
    auto* samples = static_cast<std::int16_t*>(std::malloc(fc * sizeof(std::int16_t)));

    // Simple one-pole high-pass to push the noise energy into the upper band:
    //   y[n] = a * (y[n-1] + x[n] - x[n-1])
    // a ≈ 0.92 puts the corner around 1.5 kHz at 44.1 kHz, which gives a
    // clearly "psssht" timbre without losing all the body.
    constexpr float a = 0.92f;
    float prev_x = 0.0f, prev_y = 0.0f;

    for (unsigned i = 0; i < fc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);

        // Envelope: 3 ms attack, ~80 ms exponential decay tail.
        const float env = (t < 0.003f)
            ? (t / 0.003f)
            : std::exp(-(t - 0.003f) / 0.075f);

        // White noise → high-passed noise.
        const float x  = static_cast<float>(std::rand()) /
                         static_cast<float>(RAND_MAX) * 2.0f - 1.0f;
        const float y  = a * (prev_y + x - prev_x);
        prev_x = x;
        prev_y = y;

        const float v = std::clamp(y * env * 0.95f, -1.0f, 1.0f);
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

// Heavier hit (kinetic round impact): noise slap + low body.
Wave make_hit_wave() {
    constexpr float dur = 0.13f;
    const unsigned  fc  = static_cast<unsigned>(dur * kSampleRate);
    auto* samples = static_cast<std::int16_t*>(std::malloc(fc * sizeof(std::int16_t)));
    float phase = 0.0f;
    for (unsigned i = 0; i < fc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float f = 140.0f + 180.0f * std::exp(-t / 0.025f);
        phase += kTwoPi * f / kSampleRate;
        const float noise_env = 0.55f * std::exp(-t / 0.020f);
        const float noise     = noise_env * (static_cast<float>(std::rand()) /
                                             static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
        const float body      = 0.55f * std::exp(-t / 0.05f) * std::sin(phase);
        samples[i] = static_cast<std::int16_t>(std::clamp(noise + body, -1.0f, 1.0f) * 31000.0f);
    }
    Wave w{};
    w.frameCount = fc;
    w.sampleRate = kSampleRate;
    w.sampleSize = kSampleSize;
    w.channels   = kChannels;
    w.data       = samples;
    return w;
}

// Bigger explosion: long noise tail + LFO-modulated sub-bass.
Wave make_explosion_wave() {
    constexpr float dur = 0.60f;
    const unsigned  fc  = static_cast<unsigned>(dur * kSampleRate);
    auto* samples = static_cast<std::int16_t*>(std::malloc(fc * sizeof(std::int16_t)));
    float low_phase = 0.0f;
    for (unsigned i = 0; i < fc; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float low_f = 55.0f + 30.0f * std::sin(t * 18.0f);
        low_phase += kTwoPi * low_f / kSampleRate;
        const float noise_env = 0.80f * std::exp(-t / 0.18f);
        const float noise = noise_env * (static_cast<float>(std::rand()) /
                                         static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
        const float sub_env = 0.85f * std::exp(-t / 0.22f);
        const float sub = sub_env * std::sin(low_phase);
        samples[i] = static_cast<std::int16_t>(std::clamp(noise * 0.55f + sub * 0.70f, -1.0f, 1.0f) * 32000.0f);
    }
    Wave w{};
    w.frameCount = fc;
    w.sampleRate = kSampleRate;
    w.sampleSize = kSampleSize;
    w.channels   = kChannels;
    w.data       = samples;
    return w;
}

// Continuous engine rumble — 4-second loop that's phase-aligned to the
// base frequencies so the loop seam doesn't click.
Wave make_thrust_loop() {
    constexpr float dur = 4.0f;
    const unsigned fc   = static_cast<unsigned>(dur * kSampleRate);
    auto* samples = static_cast<std::int16_t*>(std::malloc(fc * sizeof(std::int16_t)));
    float p1 = 0.0f, p2 = 0.0f, p3 = 0.0f;
    for (unsigned i = 0; i < fc; ++i) {
        // Two near-octave low tones beating against each other, plus a
        // mid-band wobble for grit. All three are integer-cycle-multiples
        // of the loop length so the seam is continuous.
        constexpr float f1 = 60.0f;     // 60 Hz * 4s = 240 cycles
        constexpr float f2 = 90.0f;     // 360 cycles
        constexpr float f3 = 180.0f;    // 720 cycles
        p1 += kTwoPi * f1 / kSampleRate;
        p2 += kTwoPi * f2 / kSampleRate;
        p3 += kTwoPi * f3 / kSampleRate;
        const float v = std::sin(p1) * 0.55f
                      + std::sin(p2) * 0.30f
                      + std::sin(p3) * 0.15f;
        samples[i] = static_cast<std::int16_t>(std::clamp(v * 0.9f, -1.0f, 1.0f) * 28000.0f);
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

    thrust_wave_ = make_thrust_loop();
    thrust_loop_ = LoadSoundFromWave(thrust_wave_);
    SetSoundVolume(thrust_loop_, 0.0f);
    PlaySound(thrust_loop_);
    thrust_ready_ = true;

    ready_ = true;
    VECTOR_INFO("audio: ready, {} cues + thrust loop", static_cast<int>(Cue::Count));
}

AudioSystem::~AudioSystem() {
    if (thrust_ready_) {
        StopSound(thrust_loop_);
        UnloadSound(thrust_loop_);
        UnloadWave(thrust_wave_);
    }
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

    store(Cue::Fire,             make_fire_wave());
    store(Cue::Hit,              make_hit_wave());
    store(Cue::Explosion,        make_explosion_wave());

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

void AudioSystem::set_thrust(float thrust_0_to_1) {
    thrust_target_ = std::clamp(thrust_0_to_1, 0.0f, 1.0f);
}

void AudioSystem::tick(float dt) {
    if (!thrust_ready_) return;

    const float k = 1.0f - std::exp(-6.0f * dt);
    thrust_current_ += (thrust_target_ - thrust_current_) * k;

    SetSoundVolume(thrust_loop_, thrust_current_ * 0.55f);
    SetSoundPitch (thrust_loop_, 0.78f + thrust_current_ * 0.40f);

    // Loop manually — raylib's Sound doesn't auto-loop. Cost is one branch.
    if (!IsSoundPlaying(thrust_loop_)) PlaySound(thrust_loop_);
}

}  // namespace vector::audio
