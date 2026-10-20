// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/hyprlang-edit.hpp"

#include <algorithm>
#include <format>
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

}
