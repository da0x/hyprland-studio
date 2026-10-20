// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace configuration {

    // A hyprlang file (hyprtoolkit.conf, hyprpaper.conf) with each `key = value` outside
    // any block set to the value given, and every other byte as it was: an existing line
    // keeps its spacing and comment, and a key the file lacks is added at the end.
    [[nodiscard]] std::string withHyprlangValues(std::string_view config, const std::vector<std::pair<std::string, std::string>>& values);

    // Whether `config` has a line that is exactly `line`, apart from spacing around it.
    [[nodiscard]] bool hasLine(std::string_view config, std::string_view line);

    struct IniValue {
        std::string section;
        std::string key;
        std::string value;
    };

    // An INI file (dunstrc) with each key set in its [section], and every other byte as it
    // was: a line that sets it, commented out or not, keeps its indentation and comment;
    // a key the section lacks is added at the end of the section, and a missing section at
    // the end of the file.
    [[nodiscard]] std::string withIniValues(std::string_view config, const std::vector<IniValue>& values);

}
