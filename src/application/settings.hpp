// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <expected>
#include <filesystem>
#include <string>

namespace application {

    // The studio's own settings, kept in settings.json in its folder.
    struct Settings {
        // The user has seen that everything the bar needs is installed, and hid the list.
        bool barRequirementsDismissed = false;
    };

    // Missing or unreadable settings are the defaults.
    [[nodiscard]] Settings loadSettings(const std::filesystem::path& studioFolder);

    // Keeps any setting in the file that this version does not know.
    [[nodiscard]] std::expected<void, std::string> saveSettings(const Settings& settings, const std::filesystem::path& studioFolder);

}
