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
        bool                        dark = true;
    };

    // Reads a kitty theme file.
    [[nodiscard]] TerminalTheme terminalTheme(std::string_view name, std::string_view file);

    // The bundled themes, dark and light, from kitty-themes; see terminal-themes/NOTICE.
    [[nodiscard]] std::span<const TerminalTheme> terminalThemes();

    // kitty's folder: $XDG_CONFIG_HOME/kitty, or ~/.config/kitty.
    [[nodiscard]] std::filesystem::path kittyFolder(const foundation::Environment& environment);

    // What gives kitty the theme: its file as current-theme.conf, and the line in kitty.conf
    // that includes it. Empty when kitty is not installed or already uses the theme.
    [[nodiscard]] std::expected<std::vector<configuration::FileChange>, std::string> planTerminal(const TerminalTheme& theme, const foundation::Environment& environment);

    // The bundled theme kitty's current-theme.conf holds, if it holds one exactly.
    [[nodiscard]] const TerminalTheme* currentTerminal(const foundation::Environment& environment);

}
