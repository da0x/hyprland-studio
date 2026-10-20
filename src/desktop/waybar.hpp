// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "desktop/system.hpp"
#include "foundation/environment.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace desktop {

    // Something the bar needs that the studio does not install, with how to get it.
    struct Requirement {
        std::string                name;
        // Why the bar needs it.
        std::string                purpose;
        std::string                upstream;
        // Its package on Arch Linux.
        std::string                package;
        // Where it was found, or nothing when it is missing.
        std::optional<std::string> found;

        [[nodiscard]] bool         met() const { return found.has_value(); }
    };

    // The command that installs it, for the user to run. Empty on a distribution whose
    // packages the studio does not know.
    [[nodiscard]] std::string installCommand(const Requirement& requirement, const Distribution& distribution);

    // Waybar itself, and the font its icons come from.
    [[nodiscard]] std::vector<Requirement> waybarRequirements(const foundation::Environment& environment);

    // The config Waybar reads when it is started without --config, by Waybar's own rule:
    // config, then config.jsonc, in $WAYBAR_CONFIG_DIR, then in each of
    // $XDG_CONFIG_HOME/waybar, ~/.config/waybar, ~/waybar and `systemFolder`/waybar.
    [[nodiscard]] std::optional<std::filesystem::path> waybarConfig(const foundation::Environment& environment, const std::filesystem::path& systemFolder = "/etc/xdg");

}
