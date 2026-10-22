// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "foundation/environment.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace desktop {

    // fastfetch's summary of the system, as a new terminal can show it: the distribution's
    // logo beside what the system is and runs.
    enum class FetchLogo { large, small, none };

    struct Fastfetch {
        FetchLogo   logo   = FetchLogo::large;
        // A terminal color by its ANSI name, such as blue, for the logo, the title and the
        // keys: shown in the terminal's own color scheme, so it keeps matching it.
        std::string accent = "blue";
        // Everything fastfetch knows, rather than the few things most people look at.
        bool        full   = false;

        bool        operator==(const Fastfetch&) const = default;
    };

    // The accents offered, by ANSI name, with the index of the color in a terminal theme.
    struct FetchAccent {
        std::string_view name;
        std::string_view label;
        std::size_t      color;
    };
    inline constexpr std::array<FetchAccent, 6> fetchAccents{{
        {"blue", "Blue", 4}, {"magenta", "Magenta", 5}, {"cyan", "Cyan", 6}, {"green", "Green", 2}, {"yellow", "Yellow", 3}, {"red", "Red", 1},
    }};

    // $XDG_CONFIG_HOME/fastfetch/config.jsonc, or ~/.config/fastfetch/config.jsonc.
    [[nodiscard]] std::filesystem::path fastfetchConfigFile(const foundation::Environment& environment);

    // The config for `fetch`, its first line saying Lantern wrote it.
    [[nodiscard]] std::string fastfetchConfig(const Fastfetch& fetch);

    // What a config Lantern wrote holds; nothing for any other.
    [[nodiscard]] std::optional<Fastfetch> fastfetchIn(std::string_view config);

    // ~/.bashrc.
    [[nodiscard]] std::filesystem::path bashrcFile(const foundation::Environment& environment);

    // Whether a .bashrc runs fastfetch on a line of its own, as each terminal opens.
    [[nodiscard]] bool greetsWithFastfetch(std::string_view bashrc);

    // The .bashrc running fastfetch at its end, or no longer running it, every other line
    // kept.
    [[nodiscard]] std::string withFastfetchGreeting(std::string_view bashrc, bool on);

}
