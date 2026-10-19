// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/main-window.hpp"

#include <print>
#include <span>
#include <string_view>

int main(int argc, char** argv) {
    ui::Options options;
    for (const std::string_view argument : std::span{argv, static_cast<std::size_t>(argc)}.subspan(1)) {
        if (argument == "--verbose") {
            options.verbose = true;
        } else if (argument == "--help") {
            std::println("Usage: hyprland-studio [--verbose]");
            return 0;
        } else {
            std::println(stderr, "hyprland-studio: unknown option {}", argument);
            return 2;
        }
    }
    return ui::run(options);
}
