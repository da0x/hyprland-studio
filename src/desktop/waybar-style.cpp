// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/waybar-style.hpp"

#include "desktop/waybar.hpp"

#include <array>

namespace desktop {

    namespace {

        // Bytes, not char: the icons are UTF-8 above 127.
        constexpr unsigned char catppuccinMochaConfig[] = {
#embed "waybar-styles/catppuccin-mocha.jsonc"
        };

        constexpr unsigned char catppuccinMochaStyle[] = {
#embed "waybar-styles/catppuccin-mocha.css"
        };

        std::string_view text(std::span<const unsigned char> bytes) {
            return {reinterpret_cast<const char*>(bytes.data()), bytes.size()}; // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast): bytes read as UTF-8 text
        }

        const std::array<WaybarStyle, 1>& styles() {
            static const std::array list{
                WaybarStyle{
                    .name    = "Catppuccin Mocha",
                    .summary = "Dark, in three rounded islands that float at the same distance from the screen's edges as your windows: "
                               "workspaces on the left, the clock in the middle, audio, network and the tray on the right.",
                    .config  = text(catppuccinMochaConfig),
                    .style   = text(catppuccinMochaStyle),
                },
            };
            return list;
        }

        bool under(const std::filesystem::path& file, const std::filesystem::path& folder) {
            const auto relative = file.lexically_relative(folder);
            return !relative.empty() && !relative.native().starts_with("..");
        }

    }

    std::span<const WaybarStyle> waybarStyles() {
        return styles();
    }

    std::filesystem::path waybarUserFolder(const foundation::Environment& environment, const std::filesystem::path& systemFolder) {
        if (const auto config = waybarConfig(environment, systemFolder); config && !under(*config, systemFolder))
            return config->parent_path();
        if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"); home && std::filesystem::path{*home}.is_absolute())
            return std::filesystem::path{*home} / "waybar";
        return std::filesystem::path{foundation::variable(environment, "HOME").value_or("")} / ".config" / "waybar";
    }

    std::expected<std::vector<configuration::FileChange>, std::string> planWaybarStyle(const WaybarStyle& style, const foundation::Environment& environment,
                                                                                      const std::filesystem::path& systemFolder) {
        const auto folder = waybarUserFolder(environment, systemFolder);
        // Keep the name of a config that is already there: Waybar reads config before config.jsonc.
        auto configFile = folder / "config.jsonc";
        if (const auto config = waybarConfig(environment, systemFolder); config && config->parent_path() == folder)
            configFile = *config;

        std::vector<configuration::FileChange> changes;
        for (const auto& [file, text] : {std::pair{configFile, style.config}, std::pair{folder / "style.css", style.style}}) {
            auto change = configuration::planFile(file, std::string{text});
            if (!change)
                return std::unexpected{change.error()};
            changes.push_back(std::move(*change));
        }
        return changes;
    }

}
