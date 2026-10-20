// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "configuration/main-config.hpp"
#include "desktop/waybar-style.hpp"
#include "foundation/environment.hpp"

#include <expected>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace desktop {

    // One palette for the whole desktop: the windows' borders, Waybar, and hyprtoolkit's
    // apps, the studio among them.

    // The border values hl.config gets, as Lua: a gradient of the accent and the audio
    // colour for the focused window, the raised colour for the others.
    [[nodiscard]] std::string activeBorder(const Palette& palette);
    [[nodiscard]] std::string inactiveBorder(const Palette& palette);

    // hyprtoolkit.conf's colour keys for the palette.
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> hyprtoolkitColours(const Palette& palette);

    // $XDG_CONFIG_HOME/hypr/hyprtoolkit.conf, or ~/.config/hypr/hyprtoolkit.conf.
    [[nodiscard]] std::filesystem::path hyprtoolkitConfigFile(const foundation::Environment& environment);

    // Every file change that gives the desktop `palette`: the Hyprland config (only a Lua
    // config, through its hl.config borders; a linked config is followed), Waybar in
    // `shape`, and hyprtoolkit.conf. Only files that change are listed.
    [[nodiscard]] std::expected<std::vector<configuration::FileChange>, std::string> planTheme(const Palette& palette, Shape shape, const configuration::MainConfig& config,
                                                                                               const foundation::Environment& environment);

    // The palette whose borders the Hyprland config sets now, if it is one of the studio's.
    [[nodiscard]] const Palette* currentTheme(const configuration::MainConfig& config);

}
