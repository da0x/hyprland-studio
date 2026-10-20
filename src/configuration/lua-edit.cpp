// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/lua-edit.hpp"

#include "syntax/highlight.hpp"

#include <format>
#include <regex>

namespace configuration {

    std::expected<std::string, std::string> withLuaConfigValue(std::string_view config, const std::vector<std::string_view>& path, std::string_view value) {
        const auto found = syntax::luaCallField(config, "hl.config", path);
        if (!found) {
            std::string dotted;
            for (const auto part : path)
                dotted += (dotted.empty() ? "" : ".") + std::string{part};
            return std::unexpected{std::format("the config does not set {} in hl.config({{...}})", dotted)};
        }
        std::string text{config};
        text.replace(found->begin, found->end - found->begin, value);
        return text;
    }

    std::string withLuaConfigValueSet(std::string_view config, const std::vector<std::string_view>& path, std::string_view value) {
        if (auto edited = withLuaConfigValue(config, path, value))
            return std::move(*edited);
        // hl.config({ a = { b = value } }), nested a level per step of the path.
        std::string table{value};
        for (auto step = path.rbegin(); step != path.rend(); ++step)
            table = std::format("{{ {} = {} }}", *step, table);
        std::string text{config};
        if (!text.empty() && !text.ends_with('\n'))
            text += '\n';
        return text + std::format("\nhl.config({})\n", table);
    }

    namespace {

        // Each hl.env("name", "value") at the start of a line: where its value's quotes are.
        struct EnvCall {
            std::size_t valueBegin;
            std::size_t valueEnd;
        };

        std::optional<EnvCall> lastEnv(std::string_view config, std::string_view name) {
            const std::regex pattern{std::format(R"(^[ \t]*hl\.env\(\s*"{}"\s*,\s*"([^"\n]*)\")", std::string{name}), std::regex::multiline};
            std::optional<EnvCall> last;
            for (auto match = std::cregex_iterator{config.data(), config.data() + config.size(), pattern}; match != std::cregex_iterator{}; ++match)
                last = EnvCall{static_cast<std::size_t>(match->position(1)), static_cast<std::size_t>(match->position(1) + match->length(1))};
            return last;
        }

    }

    std::string withLuaEnv(std::string_view config, std::string_view name, std::string_view value) {
        std::string text{config};
        if (const auto found = lastEnv(config, name)) {
            text.replace(found->valueBegin, found->valueEnd - found->valueBegin, value);
            return text;
        }
        if (!text.empty() && !text.ends_with('\n'))
            text += '\n';
        return text + std::format("hl.env(\"{}\", \"{}\")\n", name, value);
    }

    std::optional<std::string> luaEnv(std::string_view config, std::string_view name) {
        const auto found = lastEnv(config, name);
        if (!found)
            return std::nullopt;
        return std::string{config.substr(found->valueBegin, found->valueEnd - found->valueBegin)};
    }

}
