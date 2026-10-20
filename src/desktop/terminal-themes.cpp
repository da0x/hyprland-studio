// SPDX-License-Identifier: GPL-3.0-or-later
// The bundled terminal themes. Each file is kitty-themes' own, unchanged; see
// terminal-themes/NOTICE.
#include "desktop/terminal-theme.hpp"

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace desktop {

    namespace {

        constexpr unsigned char file_Alabaster[] = {
#embed "terminal-themes/Alabaster.conf"
        };
        constexpr unsigned char file_BlackMetal[] = {
#embed "terminal-themes/BlackMetal.conf"
        };
        constexpr unsigned char file_Catppuccin_Frappe[] = {
#embed "terminal-themes/Catppuccin-Frappe.conf"
        };
        constexpr unsigned char file_Catppuccin_Latte[] = {
#embed "terminal-themes/Catppuccin-Latte.conf"
        };
        constexpr unsigned char file_Catppuccin_Macchiato[] = {
#embed "terminal-themes/Catppuccin-Macchiato.conf"
        };
        constexpr unsigned char file_Catppuccin_Mocha[] = {
#embed "terminal-themes/Catppuccin-Mocha.conf"
        };
        constexpr unsigned char file_Dracula[] = {
#embed "terminal-themes/Dracula.conf"
        };
        constexpr unsigned char file_everforest_dark_medium[] = {
#embed "terminal-themes/everforest_dark_medium.conf"
        };
        constexpr unsigned char file_everforest_light_medium[] = {
#embed "terminal-themes/everforest_light_medium.conf"
        };
        constexpr unsigned char file_GitHub_Dark[] = {
#embed "terminal-themes/GitHub_Dark.conf"
        };
        constexpr unsigned char file_GitHub_Dark_Colorblind[] = {
#embed "terminal-themes/GitHub_Dark_Colorblind.conf"
        };
        constexpr unsigned char file_GitHub_Dark_High_Contrast[] = {
#embed "terminal-themes/GitHub_Dark_High_Contrast.conf"
        };
        constexpr unsigned char file_GitHub_Light[] = {
#embed "terminal-themes/GitHub_Light.conf"
        };
        constexpr unsigned char file_GitHub_Light_Colorblind[] = {
#embed "terminal-themes/GitHub_Light_Colorblind.conf"
        };
        constexpr unsigned char file_gruvbox_dark[] = {
#embed "terminal-themes/gruvbox-dark.conf"
        };
        constexpr unsigned char file_gruvbox_light[] = {
#embed "terminal-themes/gruvbox-light.conf"
        };
        constexpr unsigned char file_kanagawa[] = {
#embed "terminal-themes/kanagawa.conf"
        };
        constexpr unsigned char file_kanagawa_dragon[] = {
#embed "terminal-themes/kanagawa_dragon.conf"
        };
        constexpr unsigned char file_kanagawa_light[] = {
#embed "terminal-themes/kanagawa_light.conf"
        };
        constexpr unsigned char file_Modus_Operandi[] = {
#embed "terminal-themes/Modus_Operandi.conf"
        };
        constexpr unsigned char file_Modus_Vivendi[] = {
#embed "terminal-themes/Modus_Vivendi.conf"
        };
        constexpr unsigned char file_Nightfox[] = {
#embed "terminal-themes/Nightfox.conf"
        };
        constexpr unsigned char file_Nord[] = {
#embed "terminal-themes/Nord.conf"
        };
        constexpr unsigned char file_OneHalfLight[] = {
#embed "terminal-themes/OneHalfLight.conf"
        };
        constexpr unsigned char file_PaperColor_dark[] = {
#embed "terminal-themes/PaperColor_dark.conf"
        };
        constexpr unsigned char file_PaperColor_light[] = {
#embed "terminal-themes/PaperColor_light.conf"
        };
        constexpr unsigned char file_rose_pine[] = {
#embed "terminal-themes/rose-pine.conf"
        };
        constexpr unsigned char file_rose_pine_dawn[] = {
#embed "terminal-themes/rose-pine-dawn.conf"
        };
        constexpr unsigned char file_rose_pine_moon[] = {
#embed "terminal-themes/rose-pine-moon.conf"
        };
        constexpr unsigned char file_Sequoia_Monochrome_Dark[] = {
#embed "terminal-themes/Sequoia_Monochrome_Dark.conf"
        };
        constexpr unsigned char file_Solarized_Dark[] = {
#embed "terminal-themes/Solarized_Dark.conf"
        };
        constexpr unsigned char file_Solarized_Light[] = {
#embed "terminal-themes/Solarized_Light.conf"
        };
        constexpr unsigned char file_tokyo_night_night[] = {
#embed "terminal-themes/tokyo_night_night.conf"
        };
        constexpr unsigned char file_tokyo_night_day[] = {
#embed "terminal-themes/tokyo_night_day.conf"
        };
        constexpr unsigned char file_tokyo_night_storm[] = {
#embed "terminal-themes/tokyo_night_storm.conf"
        };

        struct Bundled {
            std::string_view               name;
            std::span<const unsigned char> file;
            // Made for colour blindness or low vision: high contrast, or colours told apart without red and green.
            bool                           accessible = false;
        };

        constexpr std::array<Bundled, 35> bundled{{
            {"Alabaster", file_Alabaster, false},
            {"Black Metal", file_BlackMetal, false},
            {"Catppuccin Frappé", file_Catppuccin_Frappe, false},
            {"Catppuccin Latte", file_Catppuccin_Latte, false},
            {"Catppuccin Macchiato", file_Catppuccin_Macchiato, false},
            {"Catppuccin Mocha", file_Catppuccin_Mocha, false},
            {"Dracula", file_Dracula, false},
            {"Everforest Dark", file_everforest_dark_medium, false},
            {"Everforest Light", file_everforest_light_medium, false},
            {"GitHub Dark", file_GitHub_Dark, false},
            {"GitHub Dark Colorblind", file_GitHub_Dark_Colorblind, true},
            {"GitHub Dark High Contrast", file_GitHub_Dark_High_Contrast, true},
            {"GitHub Light", file_GitHub_Light, false},
            {"GitHub Light Colorblind", file_GitHub_Light_Colorblind, true},
            {"Gruvbox Dark", file_gruvbox_dark, false},
            {"Gruvbox Light", file_gruvbox_light, false},
            {"Kanagawa", file_kanagawa, false},
            {"Kanagawa Dragon", file_kanagawa_dragon, false},
            {"Kanagawa Light", file_kanagawa_light, false},
            {"Modus Operandi", file_Modus_Operandi, true},
            {"Modus Vivendi", file_Modus_Vivendi, true},
            {"Nightfox", file_Nightfox, false},
            {"Nord", file_Nord, false},
            {"One Half Light", file_OneHalfLight, false},
            {"PaperColor Dark", file_PaperColor_dark, false},
            {"PaperColor Light", file_PaperColor_light, false},
            {"Rosé Pine", file_rose_pine, false},
            {"Rosé Pine Dawn", file_rose_pine_dawn, false},
            {"Rosé Pine Moon", file_rose_pine_moon, false},
            {"Sequoia Monochrome", file_Sequoia_Monochrome_Dark, false},
            {"Solarized Dark", file_Solarized_Dark, false},
            {"Solarized Light", file_Solarized_Light, false},
            {"Tokyo Night", file_tokyo_night_night, false},
            {"Tokyo Night Day", file_tokyo_night_day, false},
            {"Tokyo Night Storm", file_tokyo_night_storm, false},
        }};

    }

    std::span<const TerminalTheme> terminalThemes() {
        static const auto themes = [] {
            std::vector<TerminalTheme> made;
            for (const auto& [name, file, accessible] : bundled) {
                auto theme       = terminalTheme(name, std::string_view{reinterpret_cast<const char*>(file.data()), file.size()}); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast): bytes read as text
                theme.accessible = accessible;
                made.push_back(std::move(theme));
            }
            return made;
        }();
        return themes;
    }

}
