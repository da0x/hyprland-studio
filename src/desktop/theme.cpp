// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/theme.hpp"

#include "configuration/hyprlang-edit.hpp"
#include "configuration/lua-edit.hpp"
#include "syntax/highlight.hpp"

#include <format>
#include <fstream>
#include <sstream>

namespace desktop {

    namespace {

        // #rrggbb as Hyprland's rgba(rrggbbaa).
        std::string rgba(std::string_view hex, std::string_view alpha) {
            return std::format("rgba({}{})", hex.substr(1), alpha);
        }

        // #rrggbb as hyprlang's 0xaarrggbb.
        std::string argb(std::string_view hex) {
            return std::format("0xff{}", hex.substr(1));
        }

        std::string readAll(const std::filesystem::path& file) {
            std::ifstream     stream{file, std::ios::binary};
            std::stringstream text;
            text << stream.rdbuf();
            return text.str();
        }

        const std::vector<std::string_view> activePath{"general", "col", "active_border"};
        const std::vector<std::string_view> inactivePath{"general", "col", "inactive_border"};

    }

    std::string activeBorder(const Palette& palette) {
        return std::format("{{ colors = {{\"{}\", \"{}\"}}, angle = 45 }}", rgba(palette.accent, "ee"), rgba(palette.audio, "ee"));
    }

    std::string inactiveBorder(const Palette& palette) {
        return std::format("\"{}\"", rgba(palette.raised, "aa"));
    }

    std::vector<std::pair<std::string, std::string>> hyprtoolkitColours(const Palette& palette) {
        return {
            {"background", argb(palette.base)},       {"base", argb(palette.surface)},         {"alternate_base", argb(palette.raised)},
            {"text", argb(palette.text)},             {"bright_text", argb(palette.text)},     {"link_text", argb(palette.audio)},
            {"accent", argb(palette.accent)},         {"accent_secondary", argb(palette.network)},
        };
    }

    std::filesystem::path hyprtoolkitConfigFile(const foundation::Environment& environment) {
        if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"); home && std::filesystem::path{*home}.is_absolute())
            return std::filesystem::path{*home} / "hypr" / "hyprtoolkit.conf";
        return std::filesystem::path{foundation::variable(environment, "HOME").value_or("")} / ".config" / "hypr" / "hyprtoolkit.conf";
    }

    std::expected<std::vector<configuration::FileChange>, std::string> planTheme(const Palette& palette, Shape shape, const configuration::MainConfig& config,
                                                                                 const foundation::Environment& environment) {
        if (!config.lua())
            return std::unexpected{"the theme changes window borders only in a Lua config (hyprland.lua) so far"};
        std::vector<configuration::FileChange> changes;

        std::error_code error;
        const auto      file = std::filesystem::canonical(config.file, error);
        if (error)
            return std::unexpected{std::format("cannot read {}: {}", config.file.string(), error.message())};
        auto hyprland = configuration::planFile(file, "");
        if (!hyprland)
            return std::unexpected{hyprland.error()};
        auto text = configuration::withLuaConfigValue(hyprland->current.value_or(""), activePath, activeBorder(palette));
        if (text)
            text = configuration::withLuaConfigValue(*text, inactivePath, inactiveBorder(palette));
        if (!text)
            return std::unexpected{text.error()};
        hyprland->proposed = std::move(*text);
        if (hyprland->changes())
            changes.push_back(std::move(*hyprland));

        auto waybar = planWaybarStyle(WaybarStyle{shape, &palette}, environment);
        if (!waybar)
            return std::unexpected{waybar.error()};
        for (auto& change : *waybar)
            if (change.changes())
                changes.push_back(std::move(change));

        auto toolkit = configuration::planFile(hyprtoolkitConfigFile(environment), "");
        if (!toolkit)
            return std::unexpected{toolkit.error()};
        toolkit->proposed = configuration::withHyprlangValues(toolkit->current.value_or(""), hyprtoolkitColours(palette));
        if (toolkit->changes())
            changes.push_back(std::move(*toolkit));
        return changes;
    }

    const Palette* currentTheme(const configuration::MainConfig& config) {
        if (!config.lua())
            return nullptr;
        const auto source = readAll(config.file);
        const auto active = syntax::luaCallField(source, "hl.config", activePath);
        if (!active)
            return nullptr;
        const auto shown = std::string_view{source}.substr(active->begin, active->end - active->begin);
        for (const auto& palette : waybarPalettes())
            if (shown == activeBorder(palette))
                return &palette;
        return nullptr;
    }

}
