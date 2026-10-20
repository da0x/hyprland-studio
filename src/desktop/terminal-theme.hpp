// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "foundation/environment.hpp"

#include <array>
#include <optional>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace desktop {

    // How a theme is adjusted for the person reading it, applied to every theme alike.
    enum class Contrast {
        standard,
        // Text and every colour at least 7:1 against the background (WCAG AAA), comments
        // at least 4.5:1, each keeping its hue.
        high,
    };
    enum class Vision {
        typical,
        // Protanopia and deuteranopia: red and green move to orange and blue.
        redGreen,
        // Tritanopia: blue and yellow move to hues told apart without them.
        blueYellow,
    };

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
        bool                        dark     = true;
        // How it was adjusted from the theme bundled.
        Contrast                    contrast = Contrast::standard;
        Vision                      vision   = Vision::typical;
    };

    // The theme adjusted: its colours, and its file with each changed value replaced. It
    // keeps its name; see adjustmentName.
    [[nodiscard]] TerminalTheme adjusted(const TerminalTheme& theme, Contrast contrast, Vision vision);

    // What was adjusted, said after a theme's name: "high contrast, for red-green colour
    // blindness", or empty for neither.
    [[nodiscard]] std::string adjustmentName(Contrast contrast, Vision vision);

    // Reads a kitty theme file.
    [[nodiscard]] TerminalTheme terminalTheme(std::string_view name, std::string_view file);

    // The bundled themes, dark and light, from kitty-themes; see terminal-themes/NOTICE.
    // They set colours only, so applying one changes nothing else about kitty.
    [[nodiscard]] std::span<const TerminalTheme> terminalThemes();

    // Every bundled theme adjusted alike, in the same order. Made once for each choice and
    // kept, so a theme's address stays the same for as long as the studio runs.
    [[nodiscard]] std::span<const TerminalTheme> terminalThemes(Contrast contrast, Vision vision);

    // kitty's folder: $XDG_CONFIG_HOME/kitty, or ~/.config/kitty.
    [[nodiscard]] std::filesystem::path kittyFolder(const foundation::Environment& environment);

    // A font for the terminal: one installed, or one the studio recommends.
    struct TerminalFont {
        std::string family;
        // Its package on Arch Linux, for a recommended font; empty for one only installed.
        std::string package;
        // Its licence, for a recommended font.
        std::string licence;
        bool        installed = false;
    };

    // The monospace fonts installed, and the open-source ones the studio recommends, which
    // are listed whether installed or not: the recommended first, then the rest by name.
    [[nodiscard]] std::vector<TerminalFont> terminalFonts();

    // kitty's font family: kitty.conf's font_family, or nothing for kitty's default.
    [[nodiscard]] std::optional<std::string> kittyFontFamily(const foundation::Environment& environment);

    // kitty's font size in points: kitty.conf's font_size, or kitty's default, 11.
    [[nodiscard]] double kittyFontSize(const foundation::Environment& environment);

    // What gives kitty the theme and the font: the theme's file as current-theme.conf, and in
    // kitty.conf the line that includes it, font_size, and font_family when one is given.
    // Empty when kitty is not installed or already has them all.
    [[nodiscard]] std::expected<std::vector<configuration::FileChange>, std::string> planTerminal(const TerminalTheme& theme, double fontSize,
                                                                                                  const std::optional<std::string>& fontFamily,
                                                                                                  const foundation::Environment& environment);

    // The bundled theme kitty's current-theme.conf holds, if it holds one exactly.
    [[nodiscard]] const TerminalTheme* currentTerminal(const foundation::Environment& environment);

}
