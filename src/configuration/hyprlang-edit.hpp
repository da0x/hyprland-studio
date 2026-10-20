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

}
