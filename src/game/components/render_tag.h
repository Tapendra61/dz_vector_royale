#pragma once

#include <raylib.h>

namespace vector::game {

enum class Shape : unsigned char { TriangleShip, Circle, Pickup };

struct RenderComponent {
    Shape shape  {Shape::TriangleShip};
    Color color  {RAYWHITE};
    float size   {12.0f};  // half-extent in world units
};

}  // namespace vector::game
