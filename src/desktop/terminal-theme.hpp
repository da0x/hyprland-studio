// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "foundation/environment.hpp"

#include <array>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace desktop {

    // A terminal's colour scheme, as kitty writes one: chosen apart from the desktop's
    // palette, since terminal themes are their own family.
    struct TerminalTheme {
        std::string                 name;
        // The theme file as bundled, which becomes kitty's current-theme.conf.
        std::string                 file;
        // Read from the file, for drawing it.
        std::string                 background;
        std::string                 foreground;
        std::string                 cursor;
        std::array<std::string, 16> colours;
        bool                        dark       = true;
        // Made for colour blindness or low vision.
        bool                        accessible = false;
    };

    // Reads a kitty theme file.
    [[nodiscard]] TerminalTheme terminalTheme(std::string_view name, std::string_view file);

    // The bundled themes, dark and light, from kitty-themes; see terminal-themes/NOTICE.
    // They set colours only, so applying one changes nothing else about kitty.
    [[nodiscard]] std::span<const TerminalTheme> terminalThemes();

    // kitty's folder: $XDG_CONFIG_HOME/kitty, or ~/.config/kitty.
    [[nodiscard]] std::filesystem::path kittyFolder(const foundation::Environment& environment);

    // kitty's font size in points: kitty.conf's font_size, or kitty's default, 11.
    [[nodiscard]] double kittyFontSize(const foundation::Environment& environment);

    // What gives kitty the theme and the font size: the theme's file as current-theme.conf,
    // and in kitty.conf the line that includes it and font_size. Empty when kitty is not
    // installed or already has both.
    [[nodiscard]] std::expected<std::vector<configuration::FileChange>, std::string> planTerminal(const TerminalTheme& theme, double fontSize,
                                                                                                  const foundation::Environment& environment);

    // The bundled theme kitty's current-theme.conf holds, if it holds one exactly.
    [[nodiscard]] const TerminalTheme* currentTerminal(const foundation::Environment& environment);

}
