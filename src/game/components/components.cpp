#include "game/components/pickup.h"

namespace vector::game {

PowerUpSlot slot_for(PowerUpType type) {
    switch (type) {
        case PowerUpType::Laser:
        case PowerUpType::IceBlast:
        case PowerUpType::Mine:
        case PowerUpType::Missile:
        case PowerUpType::ScatterShot:
        case PowerUpType::Railgun:
            return PowerUpSlot::Primary;
        default:
            return PowerUpSlot::Special;
    }
}

const char* name_of(PowerUpType type) {
    switch (type) {
        case PowerUpType::None:        return "None";
        case PowerUpType::Laser:       return "Laser";
        case PowerUpType::IceBlast:    return "Ice Blast";
        case PowerUpType::Mine:        return "Mine";
        case PowerUpType::Missile:     return "Missile";
        case PowerUpType::ScatterShot: return "Scatter Shot";
        case PowerUpType::Railgun:     return "Railgun";
        case PowerUpType::Shield:      return "Shield";
        case PowerUpType::RepairPack:  return "Repair Pack";
        case PowerUpType::Cloak:       return "Cloak";
        case PowerUpType::SpeedBoost:  return "Speed Boost";
        case PowerUpType::EMP:         return "EMP";
        case PowerUpType::Teleport:    return "Teleport";
    }
    return "?";
}

}  // namespace vector::game
