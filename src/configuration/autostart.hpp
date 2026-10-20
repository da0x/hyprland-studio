// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <expected>
#include <span>
#include <string>
#include <string_view>

namespace configuration {

    // Whether the config starts `command` at login: a line that is exactly
    // hl.exec_cmd("command") in a Lua config, or exec-once = command in hyprland.conf.
    [[nodiscard]] bool startsAtLogin(std::string_view config, bool lua, std::string_view command);

    // `config` starting `command` at login and not starting any of `instead`, with every
    // other byte as it was. In a Lua config the line goes at the end of the first
    // hl.on("hyprland.start", function () ... end) block, indented like its first line, and
    // a block is added at the end when there is none; in hyprland.conf an exec-once line
    // is added at the end. Only lines that are exactly such a call are removed, so a
    // command started in any other way is left alone.
    [[nodiscard]] std::string withAutostart(std::string_view config, bool lua, std::string_view command, std::span<const std::string_view> instead);

}
