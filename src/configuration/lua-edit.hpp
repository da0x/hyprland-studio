// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace configuration {

    // A Lua config with the value at `path` in hl.config({...}) replaced by `value`, Lua
    // source, and every other byte as it was. Refuses when the config does not write that
    // value in place, as a literal in an hl.config table: the studio does not rewrite code
    // that computes it.
    [[nodiscard]] std::expected<std::string, std::string> withLuaConfigValue(std::string_view config, const std::vector<std::string_view>& path, std::string_view value);

}
