// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace configuration {

    // A Lua config with the value at `path` in hl.config({...}) replaced by `value`, Lua
    // source, and every other byte as it was. Refuses when the config does not write that
    // value in place, as a literal in an hl.config table: Lantern does not rewrite code
    // that computes it.
    [[nodiscard]] std::expected<std::string, std::string> withLuaConfigValue(std::string_view config, const std::vector<std::string_view>& path, std::string_view value);

    // The same, but where the config does not write the value in place, a call of its own
    // that sets it is added at the end, where it is read last and so wins.
    [[nodiscard]] std::string withLuaConfigValueSet(std::string_view config, const std::vector<std::string_view>& path, std::string_view value);

    // A Lua config whose hl.env("name", "...") call sets `value`, a plain string: the last
    // such call is changed in place, or one is added at the end. Calls in comments are left.
    [[nodiscard]] std::string withLuaEnv(std::string_view config, std::string_view name, std::string_view value);

    // The value the config's last hl.env("name", "...") call sets, if any.
    [[nodiscard]] std::optional<std::string> luaEnv(std::string_view config, std::string_view name);

}
