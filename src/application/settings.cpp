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
        if (const auto wallpaper = json.find("wallpaper"); wallpaper != json.end() && wallpaper->is_object()) {
            settings.wallpaperRequirementsDismissed = wallpaper->value("requirementsDismissed", false);
            settings.wallpaperProgram               = wallpaper->value("program", std::string{});
            settings.awwwFit                        = wallpaper->value("awwwFit", std::string{"cover"});
        }
        if (const auto accessibility = json.find("accessibility"); accessibility != json.end() && accessibility->is_object()) {
            settings.contrast = accessibility->value("contrast", std::string{});
            settings.vision   = accessibility->value("vision", std::string{});
        }
        return settings;
    }

    std::expected<void, std::string> saveSettings(const Settings& settings, const std::filesystem::path& studioFolder) {
        auto json = readFile(studioFolder);
        for (const auto* page : {"bar", "wallpaper", "accessibility"})
            if (!json[page].is_object())
                json[page] = nlohmann::json::object();
        json["bar"]["requirementsDismissed"]       = settings.barRequirementsDismissed;
        json["wallpaper"]["requirementsDismissed"] = settings.wallpaperRequirementsDismissed;
        json["wallpaper"]["program"]               = settings.wallpaperProgram;
        json["wallpaper"]["awwwFit"]               = settings.awwwFit;
        json["accessibility"]["contrast"]          = settings.contrast;
        json["accessibility"]["vision"]            = settings.vision;
        return configuration::writeWhole(studioFolder / "settings.json", json.dump(4) + "\n");
    }

}
