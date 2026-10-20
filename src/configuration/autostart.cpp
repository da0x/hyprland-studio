// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/autostart.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <vector>

namespace configuration {

    namespace {

        std::string_view trim(std::string_view text) {
            while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
                text.remove_prefix(1);
            while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
                text.remove_suffix(1);
            return text;
        }

        struct Line {
            std::size_t      begin = 0;
            // Just past its newline, or the end of the text.
            std::size_t      end   = 0;
            std::string_view text;
        };

        std::vector<Line> linesOf(std::string_view config) {
            std::vector<Line> lines;
            std::size_t       at = 0;
            while (at < config.size()) {
                const auto newline = config.find('\n', at);
                const auto stop    = newline == std::string_view::npos ? config.size() : newline;
                lines.push_back({at, newline == std::string_view::npos ? config.size() : newline + 1, config.substr(at, stop - at)});
                at = stop + 1;
            }
            return lines;
        }

        std::string callOf(bool lua, std::string_view command) {
            return lua ? std::format("hl.exec_cmd(\"{}\")", command) : std::format("exec-once = {}", command);
        }

        // The same call with any spacing around the = of hyprland.conf.
        bool isCall(std::string_view line, bool lua, std::string_view command) {
            const auto inner = trim(line);
            if (lua)
                return inner == callOf(true, command);
            const auto equals = inner.find('=');
            return equals != std::string_view::npos && trim(inner.substr(0, equals)) == "exec-once" && trim(inner.substr(equals + 1)) == command;
        }

    }

    bool startsAtLogin(std::string_view config, bool lua, std::string_view command) {
        return std::ranges::any_of(linesOf(config), [&](const Line& line) { return isCall(line.text, lua, command); });
    }

    std::string withAutostart(std::string_view config, bool lua, std::string_view command, std::span<const std::string_view> instead) {
        const auto  lines = linesOf(config);
        std::string text;
        text.reserve(config.size() + 64);
        const bool already = startsAtLogin(config, lua, command);

        // Where the new line goes: before the end of the first start block, in Lua.
        std::optional<std::size_t> blockEnd;
        std::string                indent = "    ";
        if (lua && !already) {
            bool inside = false;
            for (std::size_t index = 0; index < lines.size(); ++index) {
                const auto inner = trim(lines[index].text);
                if (!inside && inner.starts_with("hl.on(\"hyprland.start\"") && inner.ends_with("function ()")) {
                    inside = true;
                    if (index + 1 < lines.size() && trim(lines[index + 1].text) != "end)") {
                        const auto next = lines[index + 1].text;
                        indent          = std::string{next.substr(0, next.find_first_not_of(" \t"))};
                    }
                } else if (inside && inner == "end)" && lines[index].text.find_first_not_of(" \t") == 0) {
                    blockEnd = index;
                    break;
                }
            }
        }

        for (std::size_t index = 0; index < lines.size(); ++index) {
            if (blockEnd && index == *blockEnd)
                text += std::format("{}{}\n", indent, callOf(true, command));
            if (std::ranges::any_of(instead, [&](std::string_view other) { return isCall(lines[index].text, lua, other); }))
                continue;
            text += config.substr(lines[index].begin, lines[index].end - lines[index].begin);
        }

        if (!already && !blockEnd) {
            if (!text.empty() && !text.ends_with('\n'))
                text += '\n';
            if (lua)
                text += std::format("\nhl.on(\"hyprland.start\", function ()\n    {}\nend)\n", callOf(true, command));
            else
                text += std::format("\n{}\n", callOf(false, command));
        }
        return text;
    }

}
