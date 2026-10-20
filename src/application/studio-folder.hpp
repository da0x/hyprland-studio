// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "foundation/environment.hpp"

#include <filesystem>

namespace application {

    // Everything the studio keeps for itself: $XDG_CONFIG_HOME/hyprland-studio, or
    // ~/.config/hyprland-studio. Deleting it removes every trace of the studio.
    [[nodiscard]] std::filesystem::path studioFolder(const foundation::Environment& environment);

    // Where each change the studio writes is recorded, with copies of the files it replaced.
    [[nodiscard]] inline std::filesystem::path backupsFolder(const foundation::Environment& environment) {
        return studioFolder(environment) / "backups";
    }

}
