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

// Procedural cues. Phase 3 adds a continuous thrust rumble layered on top.
// Phase 7+ swaps these for authored .ogg files.
class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    AudioSystem(const AudioSystem&)            = delete;
    AudioSystem& operator=(const AudioSystem&) = delete;

    void play(Cue cue, float volume = 1.0f);

    // Continuous low rumble. Pass thrust = 0..1; the system smoothly fades
    // its loop volume in and modulates pitch around the target value.
    void set_thrust(float thrust_0_to_1);

    void tick(float dt);

private:
    void generate_all();

    std::array<Sound, static_cast<std::size_t>(Cue::Count)> sounds_{};
    std::array<Wave,  static_cast<std::size_t>(Cue::Count)> waves_{};

    Wave    thrust_wave_  {};
    Sound   thrust_loop_  {};
    Music   thrust_music_ {};
    bool    thrust_ready_ {false};
    float   thrust_target_{0.0f};
    float   thrust_current_{0.0f};

    bool ready_{false};
};

}  // namespace vector::audio
