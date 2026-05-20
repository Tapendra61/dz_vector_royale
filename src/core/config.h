#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <optional>
#include <string>

namespace vector::core {

class Config {
public:
    static std::optional<Config> load_from_file(const std::filesystem::path& path);

    static Config from_string(std::string_view json_text);

    template <typename T>
    T get(std::string_view key, T fallback) const {
        const auto* node = find(key);
        if (!node) return fallback;
        try { return node->get<T>(); } catch (...) { return fallback; }
    }

    const nlohmann::json& raw() const { return data_; }

private:
    const nlohmann::json* find(std::string_view dotted_key) const;

    nlohmann::json data_;
};

}  // namespace vector::core
