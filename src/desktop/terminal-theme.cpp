// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/terminal-theme.hpp"

#include "configuration/hyprlang-edit.hpp"
#include "desktop/system.hpp"

#include <algorithm>

#include <charconv>
#include <format>
#include <fstream>
#include <sstream>

namespace desktop {

    namespace {

        std::string_view trim(std::string_view text) {
            while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
                text.remove_prefix(1);
            while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
                text.remove_suffix(1);
            return text;
        }

        // Perceived lightness of #rrggbb, from 0 to 1.
        double lightness(std::string_view hex) {
            unsigned value = 0;
            if (hex.size() == 7)
                std::from_chars(hex.data() + 1, hex.data() + 7, value, 16);
            const double red = (value >> 16U) & 0xFFU, green = (value >> 8U) & 0xFFU, blue = value & 0xFFU;
            return (0.299 * red + 0.587 * green + 0.114 * blue) / 255.0;
        }

    }

    TerminalTheme terminalTheme(std::string_view name, std::string_view file) {
        TerminalTheme theme;
        theme.name = name;
        theme.file = file;
        std::size_t at = 0;
        while (at < file.size()) {
            const auto end  = std::min(file.find('\n', at), file.size());
            const auto line = trim(file.substr(at, end - at));
            at              = end + 1;
            if (line.empty() || line.front() == '#')
                continue;
            const auto space = line.find_first_of(" \t");
            if (space == std::string_view::npos)
                continue;
            const auto key   = line.substr(0, space);
            const auto value = std::string{trim(line.substr(space))};
            if (key == "background")
                theme.background = value;
            else if (key == "foreground")
                theme.foreground = value;
            else if (key == "cursor")
                theme.cursor = value;
            else if (key.starts_with("color")) {
                unsigned index = 0;
                const auto [stop, failed] = std::from_chars(key.data() + 5, key.data() + key.size(), index);
                if (failed == std::errc{} && stop == key.data() + key.size() && index < theme.colours.size())
                    theme.colours[index] = value;
            }
        }
        if (theme.cursor.empty() || theme.cursor == "none")
            theme.cursor = theme.foreground;
        theme.dark = lightness(theme.background) < 0.5;
        return theme;
    }

    std::filesystem::path kittyFolder(const foundation::Environment& environment) {
        if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"); home && std::filesystem::path{*home}.is_absolute())
            return std::filesystem::path{*home} / "kitty";
        return std::filesystem::path{foundation::variable(environment, "HOME").value_or("")} / ".config" / "kitty";
    }

    std::vector<TerminalFont> terminalFonts() {
        // Open-source monospace fonts in Arch's own repositories.
        std::vector<TerminalFont> fonts{
            {"JetBrains Mono", "ttf-jetbrains-mono", "SIL Open Font License"}, {"Fira Code", "ttf-fira-code", "SIL Open Font License"},
            {"Cascadia Code", "ttf-cascadia-code", "SIL Open Font License"},   {"Source Code Pro", "adobe-source-code-pro-fonts", "SIL Open Font License"},
            {"IBM Plex Mono", "ttf-ibm-plex", "SIL Open Font License"},        {"Iosevka", "ttc-iosevka", "SIL Open Font License"},
            {"Inconsolata", "ttf-inconsolata", "SIL Open Font License"},       {"Monaspace Neon", "otf-monaspace", "SIL Open Font License"},
            {"Hack", "ttf-hack", "MIT"},                                       {"Roboto Mono", "ttf-roboto-mono", "Apache 2.0"},
            {"DejaVu Sans Mono", "ttf-dejavu", "free (Bitstream Vera)"},       {"Noto Sans Mono", "noto-fonts", "SIL Open Font License"},
        };
        const auto installed = monospaceFamilies();
        for (auto& font : fonts)
            font.installed = std::ranges::contains(installed, font.family);
        for (const auto& family : installed)
            if (std::ranges::find(fonts, family, &TerminalFont::family) == fonts.end())
                fonts.push_back({family, "", "", true});
        return fonts;
    }

    std::optional<std::string> kittyFontFamily(const foundation::Environment& environment) {
        std::ifstream     stream{kittyFolder(environment) / "kitty.conf"};
        std::stringstream text;
        text << stream.rdbuf();
        return configuration::spacedValue(text.str(), "font_family");
    }

    double kittyFontSize(const foundation::Environment& environment) {
        std::ifstream     stream{kittyFolder(environment) / "kitty.conf"};
        std::stringstream text;
        text << stream.rdbuf();
        if (const auto value = configuration::spacedValue(text.str(), "font_size")) {
            double size = 0;
            if (const auto [end, failed] = std::from_chars(value->data(), value->data() + value->size(), size); failed == std::errc{} && size > 0)
                return size;
        }
        return 11.0;
    }

    std::expected<std::vector<configuration::FileChange>, std::string> planTerminal(const TerminalTheme& theme, double fontSize, const std::optional<std::string>& fontFamily,
                                                                                   const foundation::Environment& environment) {
        std::vector<configuration::FileChange> changes;
        if (!findProgram("kitty", foundation::variable(environment, "PATH").value_or("")))
            return changes;
        const auto folder = kittyFolder(environment);
        // kitty's own convention, as `kitten themes` follows it: the colours in
        // current-theme.conf, which kitty.conf includes.
        for (auto planned : {configuration::planEdit(folder / "current-theme.conf", [&theme](std::string_view) { return theme.file; }),
                             configuration::planEdit(folder / "kitty.conf", [fontSize, &fontFamily, &environment](std::string_view text) {
                                 std::string result{text};
                                 if (!configuration::hasLine(result, "include current-theme.conf")) {
                                     if (!result.empty() && !result.ends_with('\n'))
                                         result += '\n';
                                     result += "include current-theme.conf\n";
                                 }
                                 // Written only when it changes, so a config without one keeps kitty's default.
                                 if (fontSize != kittyFontSize(environment))
                                     result = configuration::withSpacedValue(result, "font_size", std::format("{:g}", fontSize));
                                 if (fontFamily && fontFamily != kittyFontFamily(environment))
                                     result = configuration::withSpacedValue(result, "font_family", *fontFamily);
                                 return result;
                             })}) {
            if (!planned)
                return std::unexpected{planned.error()};
            if (*planned)
                changes.push_back(std::move(**planned));
        }
        return changes;
    }

    const TerminalTheme* currentTerminal(const foundation::Environment& environment) {
        std::ifstream     stream{kittyFolder(environment) / "current-theme.conf", std::ios::binary};
        std::stringstream text;
        text << stream.rdbuf();
        for (const auto& theme : terminalThemes())
            if (text.str() == theme.file)
                return &theme;
        return nullptr;
    }

}
