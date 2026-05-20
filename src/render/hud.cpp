#include "render/hud.h"

#include "game/components/health.h"
#include "game/components/inventory.h"
#include "game/components/pickup.h"
#include "game/world.h"

#include <raylib.h>

#include <cstdio>

namespace vector::render {

void HUD::drawScreen(const game::World& world, int screen_w, int screen_h) {
    const auto& reg = const_cast<game::World&>(world).registry();
    auto player = world.player();
    if (!reg.valid(player)) return;

    const auto* hp  = reg.try_get<game::HealthComponent>(player);
    const auto* inv = reg.try_get<game::InventoryComponent>(player);
    if (!hp || !inv) return;

    // Low-HP red screen-edge vignette.
    if (hp->fraction() < 0.25f) {
        const float pulse = 0.35f + 0.25f * std::sin(static_cast<float>(GetTime()) * 6.0f);
        const Color tint  = Color{220, 30, 30, static_cast<unsigned char>(255.0f * pulse * 0.45f)};
        DrawRectangleGradientH(0, 0, 80, screen_h, tint, Fade(tint, 0.0f));
        DrawRectangleGradientH(screen_w - 80, 0, 80, screen_h, Fade(tint, 0.0f), tint);
        DrawRectangleGradientV(0, 0, screen_w, 60,            tint, Fade(tint, 0.0f));
        DrawRectangleGradientV(0, screen_h - 60, screen_w, 60, Fade(tint, 0.0f), tint);
    }

    // HP bar bottom-left.
    constexpr int margin = 20;
    constexpr int bar_w  = 240;
    constexpr int bar_h  = 18;
    const int     y      = screen_h - margin - bar_h;
    DrawRectangle(margin - 2, y - 2, bar_w + 4, bar_h + 4, Fade(BLACK, 0.45f));
    DrawRectangle(margin,     y,     bar_w,     bar_h,     Fade(BLACK, 0.55f));
    const int filled = static_cast<int>(bar_w * hp->fraction());
    Color col = hp->fraction() > 0.5f ? GREEN : (hp->fraction() > 0.25f ? YELLOW : RED);
    DrawRectangle(margin, y, filled, bar_h, col);

    char buf[64];
    std::snprintf(buf, sizeof(buf), "%d / %d HP",
                  static_cast<int>(hp->current),
                  static_cast<int>(hp->max));
    DrawText(buf, margin + 6, y + 1, 16, RAYWHITE);

    // Inventory slots top-right.
    const int slot_w = 140;
    const int slot_h = 36;
    const int sx     = screen_w - slot_w - margin;
    const int sy     = margin;
    DrawRectangle(sx, sy,            slot_w, slot_h, Fade(BLACK, 0.45f));
    DrawRectangle(sx, sy + slot_h + 6, slot_w, slot_h, Fade(BLACK, 0.45f));
    DrawText("Q  Primary", sx + 8, sy + 2, 14, Fade(RAYWHITE, 0.75f));
    DrawText(game::name_of(inv->primary), sx + 8, sy + 18, 14, RAYWHITE);
    DrawText("E  Special", sx + 8, sy + slot_h + 8,  14, Fade(RAYWHITE, 0.75f));
    DrawText(game::name_of(inv->special), sx + 8, sy + slot_h + 24, 14, RAYWHITE);
}

}  // namespace vector::render
