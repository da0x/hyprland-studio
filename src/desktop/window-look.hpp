// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "configuration/main-config.hpp"

#include <expected>
#include <optional>
#include <string>

namespace desktop {

    // How windows look and are handled: their corners, the gaps around them, their border,
    // and whether dragging that border resizes them. Each is a value in hl.config.
    struct WindowLook {
        // Corner radius, in pixels.
        int  rounding       = 10;
        // Between windows, on each window's side, so two windows are twice this apart.
        int  gapsIn         = 5;
        // Between the windows and the screen's edges.
        int  gapsOut        = 20;
        int  borderSize     = 2;
        // Dragging a window's border or gap resizes it, as on other desktops; holding Super
        // and dragging with the right button resizes it either way.
        bool resizeOnBorder = false;

        friend bool operator==(const WindowLook&, const WindowLook&) = default;
    };

    // The values the Hyprland config sets, with Hyprland's own default for any it does not
    // set; a gap written as four sides reads as its first. Nothing for a config that is not
    // Lua, or that cannot be read.
    [[nodiscard]] std::optional<WindowLook> currentWindowLook(const configuration::MainConfig& config);

    // The change to the Hyprland config (a Lua one; a linked config is followed) that sets
    // `wanted`, each value where the config writes it or in a call of its own at the end,
    // or nothing when it already does.
    [[nodiscard]] std::expected<std::optional<configuration::FileChange>, std::string> planWindowLook(const WindowLook& wanted, const configuration::MainConfig& config);

}
