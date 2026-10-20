// SPDX-License-Identifier: GPL-3.0-or-later
#include "application/settings.hpp"

#include "configuration/file-change.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace application {

    namespace {

        nlohmann::json readFile(const std::filesystem::path& studioFolder) {
            std::ifstream stream{studioFolder / "settings.json"};
            if (!stream)
                return nlohmann::json::object();
            auto json = nlohmann::json::parse(stream, nullptr, false);
            return json.is_object() ? json : nlohmann::json::object();
        }

    }

    Settings loadSettings(const std::filesystem::path& studioFolder) {
        const auto json = readFile(studioFolder);
        Settings   settings;
        if (const auto bar = json.find("bar"); bar != json.end() && bar->is_object())
            settings.barRequirementsDismissed = bar->value("requirementsDismissed", false);
        return settings;
    }

    std::expected<void, std::string> saveSettings(const Settings& settings, const std::filesystem::path& studioFolder) {
        auto json = readFile(studioFolder);
        if (!json["bar"].is_object())
            json["bar"] = nlohmann::json::object();
        json["bar"]["requirementsDismissed"] = settings.barRequirementsDismissed;
        return configuration::writeWhole(studioFolder / "settings.json", json.dump(4) + "\n");
    }

}
