// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "desktop/waybar-style.hpp"
#include "foundation/environment.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace desktop {

    // What happens while the computer is left alone, in minutes, 0 for never; and whether
    // the screen locks before the computer sleeps. hypridle carries it out.
    struct IdleSettings {
        int  dim             = 0;
        int  lock            = 0;
        int  screensOff      = 0;
        int  suspend         = 0;
        bool lockBeforeSleep = false;

        friend bool operator==(const IdleSettings&, const IdleSettings&) = default;
    };

    // $XDG_CONFIG_HOME/hypr/hypridle.conf or hyprlock.conf, or the same in ~/.config.
    [[nodiscard]] std::filesystem::path hypridleConfigFile(const foundation::Environment& environment);
    [[nodiscard]] std::filesystem::path hyprlockConfigFile(const foundation::Environment& environment);

    // The settings a hypridle.conf carries out. Each listener is known by what it runs when
    // the time is up: brightnessctl dims, loginctl lock-session or hyprlock locks, a dpms
    // dispatch turns the screens off, and systemctl suspend suspends.
    [[nodiscard]] IdleSettings idleIn(std::string_view config);

    // `config` carrying out `settings`: the listeners it knows are replaced, the general
    // block gets the commands that lock and wake the screens, and everything else is kept.
    [[nodiscard]] std::string withIdle(std::string_view config, const IdleSettings& settings);

    // A hyprlock.conf in the palette's colors: the blurred desktop behind a clock and a
    // password field.
    [[nodiscard]] std::string lockScreen(const Palette& palette);

    // Whether hyprlock.conf is one Lantern wrote.
    [[nodiscard]] bool lanternLockScreen(std::string_view config);

    // A brightness to set: a backlight, and a built-in screen for it to light. Some
    // computers with no built-in screen, such as the 2013 Mac Pro, still list one.
    [[nodiscard]] bool hasBacklight(const std::filesystem::path& classes = "/sys/class");

}
