#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace vector::platform {

// Platform shim contract (roadmap §4.2). Desktop builds get a filesystem-
// backed impl; Android unpacks from APK assets; iOS reads from the bundle.
// Game code talks only to this interface.
class IPlatform {
public:
    virtual ~IPlatform() = default;

    virtual bool                 mountAssets()        = 0;
    virtual std::filesystem::path getDataDir()  const = 0;
    virtual std::filesystem::path getPrefsDir() const = 0;
    virtual std::filesystem::path resolveAsset(const std::string& relative) const = 0;

    // Mobile-only; desktop impls return false / no-op.
    virtual void vibrate(int /*milliseconds*/)        {}
    virtual void keepScreenOn(bool /*enabled*/)       {}

    virtual std::string name() const = 0;
};

std::unique_ptr<IPlatform> create_platform();

}  // namespace vector::platform
