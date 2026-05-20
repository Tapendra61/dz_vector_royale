#include "core/config.h"

#include "core/logging.h"

#include <fstream>
#include <sstream>

namespace vector::core {

std::optional<Config> Config::load_from_file(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        VECTOR_WARN("config: could not open {}", path.string());
        return std::nullopt;
    }
    std::stringstream buf;
    buf << stream.rdbuf();
    try {
        Config cfg;
        cfg.data_ = nlohmann::json::parse(buf.str(), nullptr, true, true);
        return cfg;
    } catch (const std::exception& ex) {
        VECTOR_ERROR("config: parse error in {}: {}", path.string(), ex.what());
        return std::nullopt;
    }
}

Config Config::from_string(std::string_view json_text) {
    Config cfg;
    cfg.data_ = nlohmann::json::parse(json_text, nullptr, true, true);
    return cfg;
}

const nlohmann::json* Config::find(std::string_view dotted_key) const {
    const nlohmann::json* node = &data_;
    std::size_t start = 0;
    while (start <= dotted_key.size()) {
        const auto dot = dotted_key.find('.', start);
        const auto end = (dot == std::string_view::npos) ? dotted_key.size() : dot;
        const auto key = std::string(dotted_key.substr(start, end - start));
        if (!node->is_object()) return nullptr;
        auto it = node->find(key);
        if (it == node->end()) return nullptr;
        node = &(*it);
        if (dot == std::string_view::npos) break;
        start = dot + 1;
    }
    return node;
}

}  // namespace vector::core
