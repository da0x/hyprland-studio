// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/waybar.hpp"

#include <format>
#include <system_error>

namespace desktop {

    namespace {

        // The family of the icons in the studio's Waybar config.
        constexpr auto iconFamily = "Symbols Nerd Font";

    }

    std::string installCommand(const Requirement& requirement, const Distribution& distribution) {
        if (!distribution.arch())
            return {};
        return std::format("sudo pacman -S {}", requirement.package);
    }

    std::vector<Requirement> waybarRequirements(const foundation::Environment& environment) {
        Requirement waybar{
            .name     = "Waybar",
            .purpose  = "The bar at the top of the screen, with your workspaces, the clock, audio, network and the tray.",
            .upstream = "https://github.com/Alexays/Waybar",
            .package  = "waybar",
            .found    = std::nullopt,
        };
        if (const auto program = findProgram("waybar", foundation::variable(environment, "PATH").value_or("")))
            waybar.found = program->string();

        Requirement icons{
            .name     = "Icon font",
            .purpose  = "The bar's icons, for the volume and the network, come from this font. Without it they show as empty boxes.",
            .upstream = "https://www.nerdfonts.com",
            .package  = "ttf-nerd-fonts-symbols",
            .found    = std::nullopt,
        };
        if (hasFontFamily(iconFamily))
            icons.found = iconFamily;

        return {std::move(waybar), std::move(icons)};
    }

    std::optional<std::filesystem::path> waybarConfig(const foundation::Environment& environment, const std::filesystem::path& systemFolder) {
        std::vector<std::filesystem::path> folders;
        if (const auto folder = foundation::variable(environment, "WAYBAR_CONFIG_DIR"))
            folders.emplace_back(*folder);
        if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"))
            folders.push_back(std::filesystem::path{*home} / "waybar");
        if (const auto home = foundation::variable(environment, "HOME")) {
            folders.push_back(std::filesystem::path{*home} / ".config" / "waybar");
            folders.push_back(std::filesystem::path{*home} / "waybar");
        }
        folders.push_back(systemFolder / "waybar");

        for (const auto& folder : folders) {
            for (const auto* name : {"config", "config.jsonc"}) {
                std::error_code error;
                if (const auto file = folder / name; std::filesystem::exists(file, error))
                    return file.lexically_normal();
            }
        }
        return std::nullopt;
    }

}
