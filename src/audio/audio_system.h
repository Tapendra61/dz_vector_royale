#pragma once

#include <array>
#include <raylib.h>

namespace vector::audio {

enum class Cue : unsigned char {
    Fire,
    Hit,
    Explosion,
    PickupAcquired,
    PowerUpActivated,
    LowHpHeartbeat,
    Count
};

// Procedural mini-cues for Phase 1 — no asset files, no licensing concerns.
// Phase 3 swaps these for authored .ogg files.
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&)            = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void play(Cue cue, float volume = 1.0f);

private:
    void generate_all();

    std::array<Sound, static_cast<std::size_t>(Cue::Count)> sounds_{};
    std::array<Wave,  static_cast<std::size_t>(Cue::Count)> waves_{};
    bool ready_{false};
};

}  // namespace vector::audio
