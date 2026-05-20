#include "render/particles/emitter_spec.h"

#include "core/logging.h"

#include <nlohmann/json.hpp>

#include <fstream>

namespace vector::render {

namespace {

Color color_from(const nlohmann::json& j, Color fallback) {
    if (!j.is_array() || j.size() < 3) return fallback;
    auto clamp = [](int v) { return static_cast<unsigned char>(std::min(255, std::max(0, v))); };
    Color c;
    c.r = clamp(j[0].get<int>());
    c.g = clamp(j[1].get<int>());
    c.b = clamp(j[2].get<int>());
    c.a = j.size() >= 4 ? clamp(j[3].get<int>()) : static_cast<unsigned char>(255);
    return c;
}

template <typename T>
T pick(const nlohmann::json& obj, const char* key, T fallback) {
    auto it = obj.find(key);
    if (it == obj.end()) return fallback;
    try { return it->get<T>(); } catch (...) { return fallback; }
}

}  // namespace

bool EmitterLibrary::load(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        VECTOR_WARN("particles: no spec file at {}", path.string());
        return false;
    }
    nlohmann::json root;
    try {
        f >> root;
    } catch (const std::exception& ex) {
        VECTOR_ERROR("particles: parse {}: {}", path.string(), ex.what());
        return false;
    }
    if (!root.is_object()) return false;

    specs_.clear();
    for (auto& [name, obj] : root.items()) {
        if (name.rfind("//", 0) == 0) continue;     // skip comment keys
        if (!obj.is_object())          continue;
        EmitterSpec s = EmitterSpec::defaults();
        s.name             = name;
        s.burst_min        = pick(obj, "burst_min",        s.burst_min);
        s.burst_max        = pick(obj, "burst_max",        s.burst_max);
        s.rate_per_second  = pick(obj, "rate",             s.rate_per_second);
        s.speed_min        = pick(obj, "speed_min",        s.speed_min);
        s.speed_max        = pick(obj, "speed_max",        s.speed_max);
        s.spread_rad       = pick(obj, "spread",           s.spread_rad);
        s.lifetime_min     = pick(obj, "lifetime_min",     s.lifetime_min);
        s.lifetime_max     = pick(obj, "lifetime_max",     s.lifetime_max);
        s.size_start_min   = pick(obj, "size_start_min",   s.size_start_min);
        s.size_start_max   = pick(obj, "size_start_max",   s.size_start_max);
        s.size_end_min     = pick(obj, "size_end_min",     s.size_end_min);
        s.size_end_max     = pick(obj, "size_end_max",     s.size_end_max);
        s.drag             = pick(obj, "drag",             s.drag);
        s.additive         = pick(obj, "additive",         s.additive);
        if (auto it = obj.find("color_start"); it != obj.end()) s.color_start = color_from(*it, s.color_start);
        if (auto it = obj.find("color_end");   it != obj.end()) s.color_end   = color_from(*it, s.color_end);
        specs_.emplace(name, s);
    }
    VECTOR_INFO("particles: loaded {} emitter specs", specs_.size());
    return true;
}

const EmitterSpec* EmitterLibrary::find(const std::string& name) const {
    auto it = specs_.find(name);
    return it == specs_.end() ? nullptr : &it->second;
}

}  // namespace vector::render
