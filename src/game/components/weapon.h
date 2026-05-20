#pragma once

namespace vector::game {

struct WeaponComponent {
    float fire_rate       { 6.0f};  // shots per second
    float bullet_speed    {800.0f};
    float bullet_lifetime { 1.2f};
    float bullet_damage   { 8.0f};
    float bullet_radius   { 3.0f};
    float cooldown        { 0.0f};  // seconds remaining before next shot allowed
};

}  // namespace vector::game
