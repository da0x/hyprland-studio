// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/hyprlang-edit.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <set>

namespace configuration {

    namespace {

        std::string_view trim(std::string_view text) {
            while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
                text.remove_prefix(1);
            while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
                text.remove_suffix(1);
            return text;
        }

    }

    std::string withHyprlangValues(std::string_view config, const std::vector<std::pair<std::string, std::string>>& values) {
        std::string           text;
        std::set<std::string> written;
        int                   depth = 0;
        std::size_t           at    = 0;
        while (at < config.size()) {
            const auto newline = config.find('\n', at);
            const auto end     = newline == std::string_view::npos ? config.size() : newline;
            const auto line    = config.substr(at, end - at);
            const auto code    = line.substr(0, line.find('#'));
            const auto inner   = trim(code);
            std::string replaced{line};
            if (inner.ends_with('{'))
                ++depth;
            else if (inner == "}" && depth > 0)
                --depth;
            else if (const auto equals = code.find('='); depth == 0 && equals != std::string_view::npos) {
                const auto key   = std::string{trim(code.substr(0, equals))};
                const auto found = std::ranges::find(values, key, &std::pair<std::string, std::string>::first);
                if (found != values.end()) {
                    const auto value   = trim(code.substr(equals + 1));
                    const auto valueAt = value.empty() ? equals + 1 : code.find(value, equals + 1);
                    replaced           = std::format("{}{}{}", line.substr(0, valueAt), value.empty() ? " " + found->second : found->second,
                                                     line.substr(valueAt + value.size()));
                    written.insert(key);
                }
            }
            text += replaced;
            if (newline != std::string_view::npos)
                text += '\n';
            at = end + 1;
        }
        std::string added;
        for (const auto& [key, value] : values)
            if (!written.contains(key))
                added += std::format("{} = {}\n", key, value);
        if (!added.empty()) {
            if (!text.empty() && !text.ends_with('\n'))
                text += '\n';
            text += added;
        }
        return text;
    }

    bool hasLine(std::string_view config, std::string_view line) {
        std::size_t at = 0;
        while (at <= config.size()) {
            const auto end = std::min(config.find('\n', at), config.size());
            if (trim(config.substr(at, end - at)) == line)
                return true;
            at = end + 1;
        }
        return false;
    }

    std::string withIniValues(std::string_view config, const std::vector<IniValue>& values) {
        struct Line {
            std::string text;
            std::string section;
        };
        std::vector<Line> lines;
        std::string       section;
        for (std::size_t at = 0; at <= config.size();) {
            const auto newline = config.find('\n', at);
            const auto end     = newline == std::string_view::npos ? config.size() : newline;
            if (at == config.size() && newline == std::string_view::npos)
                break;
            const auto text  = config.substr(at, end - at);
            const auto inner = trim(text);
            if (inner.starts_with('[') && inner.ends_with(']'))
                section = std::string{inner.substr(1, inner.size() - 2)};
            lines.push_back({std::string{text}, section});
            at = end + 1;
        }

        std::set<std::size_t> done;
        for (std::size_t index = 0; index < values.size(); ++index) {
            const auto& wanted = values[index];
            // A line that sets the key in its section; a live one wins over a commented one.
            std::optional<std::size_t> live;
            std::optional<std::size_t> commented;
            for (std::size_t line = 0; line < lines.size(); ++line) {
                if (lines[line].section != wanted.section)
                    continue;
                auto inner         = trim(lines[line].text);
                const bool comment = inner.starts_with('#') || inner.starts_with(';');
                if (comment)
                    inner = trim(inner.substr(1));
                const auto equals = inner.find('=');
                if (equals == std::string_view::npos || trim(inner.substr(0, equals)) != wanted.key)
                    continue;
                (comment ? commented : live) = comment ? commented.value_or(line) : line;
            }
            const auto found = live ? live : commented;
            if (!found)
                continue;
            auto&      text   = lines[*found].text;
            const auto indent = text.substr(0, text.find_first_not_of(" \t"));
            text              = std::format("{}{} = {}", indent, wanted.key, wanted.value);
            done.insert(index);
        }

        // Keys a section lacks go at its end, before the blank lines that close it.
        std::vector<std::string> missingSections;
        for (std::size_t index = 0; index < values.size(); ++index) {
            if (done.contains(index))
                continue;
            const auto& wanted = values[index];
            std::optional<std::size_t> last;
            for (std::size_t line = 0; line < lines.size(); ++line)
                if (lines[line].section == wanted.section && !trim(lines[line].text).empty())
                    last = line;
            if (!last) {
                if (!std::ranges::contains(missingSections, wanted.section))
                    missingSections.push_back(wanted.section);
                continue;
            }
            lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(*last) + 1, Line{std::format("    {} = {}", wanted.key, wanted.value), wanted.section});
        }

        std::string text;
        for (std::size_t line = 0; line < lines.size(); ++line) {
            text += lines[line].text;
            if (line + 1 < lines.size() || config.ends_with('\n'))
                text += '\n';
        }
        for (const auto& name : missingSections) {
            if (!text.empty() && !text.ends_with('\n'))
                text += '\n';
            text += std::format("\n[{}]\n", name);
            for (const auto& wanted : values)
                if (wanted.section == name)
                    text += std::format("    {} = {}\n", wanted.key, wanted.value);
        }
        return text;
    }

}
