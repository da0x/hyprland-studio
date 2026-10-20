// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/window-look.hpp"

#include "configuration/lua-edit.hpp"
#include "syntax/highlight.hpp"

#include <array>
#include <charconv>
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
        constexpr WindowLook hyprlandDefaults{.rounding = 0, .gapsIn = 5, .gapsOut = 20, .borderSize = 1, .resizeOnBorder = false};

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
        return look;
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
            return text;
        });
    }

}
