// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/window-look.hpp"

#include "configuration/lua-edit.hpp"
#include "syntax/highlight.hpp"

#include <array>
#include <charconv>
#include <format>
#include <fstream>
#include <functional>
#include <sstream>
#include <string_view>
#include <vector>

namespace desktop {

    namespace {

        // Each value: where hl.config keeps it, and the member it is.
        struct Field {
            std::vector<std::string_view> path;
            int WindowLook::*             number = nullptr;
        };

        const std::array<Field, 4> numbers{{
            {{"decoration", "rounding"}, &WindowLook::rounding},
            {{"general", "gaps_in"}, &WindowLook::gapsIn},
            {{"general", "gaps_out"}, &WindowLook::gapsOut},
            {{"general", "border_size"}, &WindowLook::borderSize},
        }};
        const std::vector<std::string_view> resizePath{"general", "resize_on_border"};

        // Hyprland's own defaults, for values a config leaves out.
        constexpr WindowLook hyprlandDefaults{.rounding = 0, .gapsIn = 5, .gapsOut = 20, .borderSize = 1, .resizeOnBorder = false, .floatFilePickers = false};

        constexpr std::string_view pickerRule = "hl.window_rule({\n"
                                                "    -- Open and save dialogs, which the portal shows for other apps.\n"
                                                "    name   = \"float-file-pickers\",\n"
                                                "    match  = { class = \"^xdg-desktop-portal-gtk$\" },\n"
                                                "\n"
                                                "    float  = true,\n"
                                                "    center = true,\n"
                                                "})\n";

        // Where Lantern's rule for file pickers is, from the start of its line to just past
        // its closing line.
        std::optional<std::pair<std::size_t, std::size_t>> pickerRuleIn(std::string_view config) {
            const auto name = config.find("\"float-file-pickers\"");
            if (name == std::string_view::npos)
                return std::nullopt;
            const auto begin = config.rfind("hl.window_rule({", name);
            const auto close = config.find("\n})", name);
            if (begin == std::string_view::npos || close == std::string_view::npos || (begin > 0 && config[begin - 1] != '\n'))
                return std::nullopt;
            const auto end = config.find('\n', close + 1);
            return std::pair{begin, end == std::string_view::npos ? config.size() : end + 1};
        }

        std::string withPickerRule(std::string text, bool on) {
            const auto found = pickerRuleIn(text);
            if (!on && found) {
                auto begin = found->first;
                // With the blank line before it, which it was added with.
                if (begin >= 2 && text[begin - 1] == '\n' && text[begin - 2] == '\n')
                    --begin;
                text.erase(begin, found->second - begin);
            } else if (on && !found) {
                // After the last window rule, or at the end.
                auto last = text.rfind("\nhl.window_rule(");
                if (last == std::string::npos && text.starts_with("hl.window_rule("))
                    last = 0;
                const auto close = last == std::string::npos ? std::string::npos : text.find("\n})", last + 1);
                if (close == std::string::npos) {
                    if (!text.empty() && !text.ends_with('\n'))
                        text += '\n';
                    text += std::format("\n{}", pickerRule);
                } else {
                    const auto end = text.find('\n', close + 1);
                    const auto at  = end == std::string::npos ? text.size() : end + 1;
                    text.insert(at, std::format("\n{}", pickerRule));
                }
            }
            return text;
        }

        std::optional<std::string> luaValue(std::string_view config, const std::vector<std::string_view>& path) {
            const auto found = syntax::luaCallField(config, "hl.config", path);
            if (!found)
                return std::nullopt;
            return std::string{config.substr(found->begin, found->end - found->begin)};
        }

        // 5, or "5 10 5 10", whose first side is taken.
        std::optional<int> numberOf(std::string_view value) {
            if (value.size() >= 2 && value.front() == '"')
                value = value.substr(1, value.size() - 2);
            int        number = 0;
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), number);
            if (error != std::errc{} || (end != value.data() + value.size() && *end != ' '))
                return std::nullopt;
            return number;
        }

    }

    std::optional<WindowLook> currentWindowLook(const configuration::MainConfig& config) {
        if (!config.lua())
            return std::nullopt;
        std::ifstream stream{config.file, std::ios::binary};
        if (!stream)
            return std::nullopt;
        std::stringstream text;
        text << stream.rdbuf();
        WindowLook look = hyprlandDefaults;
        for (const auto& field : numbers)
            if (const auto value = luaValue(text.str(), field.path))
                look.*field.number = numberOf(*value).value_or(look.*field.number);
        if (const auto value = luaValue(text.str(), resizePath))
            look.resizeOnBorder = *value == "true";
        look.floatFilePickers = pickerRuleIn(text.str()).has_value();
        return look;
    }

    BarGeometry barGeometry(const std::optional<WindowLook>& look) {
        if (!look)
            return {};
        return {.rounding = look->rounding, .edgeGap = look->gapsOut, .topGap = look->gapsOut, .border = look->borderSize};
    }

    BarGeometry barGeometry(const configuration::MainConfig& config) {
        return barGeometry(currentWindowLook(config));
    }

    BarGeometry runningBarGeometry() {
        const auto config = configuration::mainConfig(configuration::lanternLaunch());
        return config ? barGeometry(*config) : BarGeometry{};
    }

    std::optional<WaybarStyle> currentBarStyle(const foundation::Environment& environment, const BarGeometry& geometry) {
        // Files written before have half the gap at the top, or, before the bar followed
        // the windows, the old fixed geometry.
        auto style = currentWaybarStyle(environment, geometry);
        if (!style)
            style = currentWaybarStyle(environment, BarGeometry{.rounding = geometry.rounding, .edgeGap = geometry.edgeGap, .topGap = geometry.edgeGap / 2, .border = geometry.border});
        if (!style)
            style = currentWaybarStyle(environment, BarGeometry{});
        if (style)
            style->geometry = geometry;
        return style;
    }

    std::expected<std::vector<configuration::FileChange>, std::string> planBarFollowing(const WindowLook& wanted, const std::optional<WindowLook>& current,
                                                                                       const foundation::Environment& environment) {
        auto style = currentBarStyle(environment, barGeometry(current));
        if (!style)
            return std::vector<configuration::FileChange>{};
        style->geometry = barGeometry(wanted);
        auto planned    = planWaybarStyle(*style, environment);
        if (planned)
            std::erase_if(*planned, [](const auto& change) { return !change.changes(); });
        return planned;
    }

    std::expected<std::optional<configuration::FileChange>, std::string> planWindowLook(const WindowLook& wanted, const configuration::MainConfig& config) {
        if (!config.lua())
            return std::unexpected{"windows are set only in a Lua config (hyprland.lua) so far"};
        const auto current = currentWindowLook(config);
        return configuration::planEdit(config.file, [&wanted, &current](std::string_view config) {
            auto       text = std::string{config};
            const auto set  = [&text](const std::vector<std::string_view>& path, const std::string& value, bool differs) {
                // Only what changes, so a value the config leaves to Hyprland stays left.
                if (differs)
                    text = configuration::withLuaConfigValueSet(text, path, value);
            };
            for (const auto& field : numbers)
                set(field.path, std::to_string(wanted.*field.number), !current || (*current).*field.number != wanted.*field.number);
            set(resizePath, wanted.resizeOnBorder ? "true" : "false", !current || current->resizeOnBorder != wanted.resizeOnBorder);
            return withPickerRule(std::move(text), wanted.floatFilePickers);
        });
    }

}
