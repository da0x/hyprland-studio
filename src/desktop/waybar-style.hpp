// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "foundation/environment.hpp"

#include <expected>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace desktop {

    // A complete look for Waybar: its config, with the modules and their settings, and its
    // stylesheet. Both are plain Waybar files with nothing of the studio's in them.
    struct WaybarStyle {
        std::string_view name;
        std::string_view summary;
        std::string_view config;
        std::string_view style;
    };

    [[nodiscard]] std::span<const WaybarStyle> waybarStyles();

    // The folder the studio writes Waybar's files to: the user's folder Waybar reads now,
    // or, when it reads the system's example, $XDG_CONFIG_HOME/waybar or ~/.config/waybar.
    [[nodiscard]] std::filesystem::path waybarUserFolder(const foundation::Environment& environment, const std::filesystem::path& systemFolder = "/etc/xdg");

    // The config and the stylesheet that `style` would write, with what is there now.
    [[nodiscard]] std::expected<std::vector<configuration::FileChange>, std::string> planWaybarStyle(const WaybarStyle& style, const foundation::Environment& environment,
                                                                                                    const std::filesystem::path& systemFolder = "/etc/xdg");

}
