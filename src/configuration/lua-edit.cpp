// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/lua-edit.hpp"

#include "syntax/highlight.hpp"

#include <format>

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

}
