// SPDX-License-Identifier: GPL-3.0-or-later
#include "application/studio-folder.hpp"

namespace application {

    std::filesystem::path studioFolder(const foundation::Environment& environment) {
        if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"); home && std::filesystem::path{*home}.is_absolute())
            return std::filesystem::path{*home} / "hyprland-studio";
        return std::filesystem::path{foundation::variable(environment, "HOME").value_or("")} / ".config" / "hyprland-studio";
    }

}
