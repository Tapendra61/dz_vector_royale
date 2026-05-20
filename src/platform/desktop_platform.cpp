#include "platform/desktop_platform.h"

#include "core/logging.h"

#include <cstdlib>
#include <system_error>

namespace fs = std::filesystem;

namespace vector::platform {

namespace {

fs::path executable_dir() {
    std::error_code ec;
    auto cwd = fs::current_path(ec);
    return ec ? fs::path(".") : cwd;
}

fs::path resolve_assets_root() {
    // Search a few candidate locations so the binary works from build dirs
    // *and* from a packaged install.
    const fs::path candidates[] = {
        executable_dir() / "assets",
        executable_dir() / ".." / "assets",
        executable_dir() / ".." / ".." / "assets",
        executable_dir() / ".." / ".." / ".." / "assets",
    };
    for (const auto& c : candidates) {
        std::error_code ec;
        if (fs::is_directory(c, ec)) return fs::weakly_canonical(c, ec);
    }
    return executable_dir() / "assets";
}

fs::path resolve_prefs_dir() {
#if defined(_WIN32)
    if (const char* appdata = std::getenv("APPDATA")) return fs::path(appdata) / "VECTOR";
    return executable_dir() / "prefs";
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME")) return fs::path(home) / "Library" / "Application Support" / "VECTOR";
    return executable_dir() / "prefs";
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) return fs::path(xdg) / "vector";
    if (const char* home = std::getenv("HOME"))           return fs::path(home) / ".config" / "vector";
    return executable_dir() / "prefs";
#endif
}

}  // namespace

DesktopPlatform::DesktopPlatform()
    : assets_root_(resolve_assets_root()),
      data_dir_(executable_dir()),
      prefs_dir_(resolve_prefs_dir()) {}

bool DesktopPlatform::mountAssets() {
    std::error_code ec;
    if (!fs::is_directory(assets_root_, ec)) {
        VECTOR_WARN("platform: assets dir not found at {}", assets_root_.string());
        return false;
    }
    fs::create_directories(prefs_dir_, ec);
    VECTOR_INFO("platform: assets mounted at {}", assets_root_.string());
    VECTOR_INFO("platform: prefs dir         {}", prefs_dir_.string());
    return true;
}

fs::path DesktopPlatform::resolveAsset(const std::string& relative) const {
    return assets_root_ / relative;
}

std::string DesktopPlatform::name() const {
#if defined(_WIN32)
    return "windows";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown-desktop";
#endif
}

std::unique_ptr<IPlatform> create_platform() {
    return std::make_unique<DesktopPlatform>();
}

}  // namespace vector::platform
