#pragma once

#include "platform/iplatform.h"

namespace vector::platform {

class DesktopPlatform final : public IPlatform {
public:
    DesktopPlatform();

    bool                  mountAssets() override;
    std::filesystem::path getDataDir()  const override { return data_dir_; }
    std::filesystem::path getPrefsDir() const override { return prefs_dir_; }
    std::filesystem::path resolveAsset(const std::string& relative) const override;

    std::string name() const override;

private:
    std::filesystem::path assets_root_;
    std::filesystem::path data_dir_;
    std::filesystem::path prefs_dir_;
};

}  // namespace vector::platform
