// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/main-window.hpp"

#include <print>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <unistd.h>

int main(int argc, char** argv) {
    ui::Options options;
    const std::span arguments{argv, static_cast<std::size_t>(argc)};
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        const std::string_view argument{arguments[index]};
        if (argument == "--verbose") {
            options.verbose = true;
        } else if (argument == "--page" && index + 1 < arguments.size()) {
            options.page = arguments[++index];
        } else if (argument == "--help") {
            std::println("Usage: hyprland-studio [--verbose] [--page TITLE]");
            return 0;
        } else {
            std::println(stderr, "hyprland-studio: unknown option {}", argument);
            return 2;
        }
    }
    const auto ending = ui::run(options);
    if (!ending.restart)
        return ending.code;

    // Start again as the same program, on the page it was on.
    std::vector<std::string> words{arguments[0], "--page", ending.page};
    if (options.verbose)
        words.emplace_back("--verbose");
    std::vector<char*> again;
    for (auto& word : words)
        again.push_back(word.data());
    again.push_back(nullptr);
    ::execv("/proc/self/exe", again.data());
    std::println(stderr, "hyprland-studio: could not start again");
    return 1;
}
