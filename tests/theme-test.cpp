// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/file-change.hpp"
#include "desktop/theme.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

namespace {

    TEST(Theme, PlansBordersWaybarAndHyprtoolkitAndRecognisesTheResult) {
        static std::atomic<int> counter = 0;
        const auto root = std::filesystem::temp_directory_path() / ("hyprland-studio-theme-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
        std::filesystem::create_directories(root / "home/.config/hypr");
        std::ofstream{root / "home/.config/hypr/hyprland.lua"} << "hl.config({\n    general = {\n        col = {\n            active_border = \"rgba(33ccffee)\",\n"
                                                                 "            inactive_border = \"rgba(595959aa)\",\n        },\n    },\n})\n";
        std::ofstream{root / "home/.config/hypr/hyprtoolkit.conf"} << "# my toolkit\nfont_size = 12\n";
        const foundation::Environment  environment{{"HOME", (root / "home").string()}};
        const configuration::MainConfig config{.file = root / "home/.config/hypr/hyprland.lua", .choice = configuration::Choice::search, .safeMode = false};
        const auto&                    tokyo = desktop::waybarPalettes()[3];

        EXPECT_EQ(desktop::currentTheme(config), nullptr);
        const auto planned = desktop::planTheme(tokyo, desktop::Shape::floating, config, environment);
        ASSERT_TRUE(planned.has_value()) << planned.error();
        // The Hyprland config, Waybar's two files and hyprtoolkit.conf.
        ASSERT_EQ(planned->size(), 4U);
        EXPECT_TRUE((*planned)[0].proposed.contains("active_border = { colors = {\"rgba(7aa2f7ee)\", \"rgba(7dcfffee)\"}, angle = 45 },"));
        EXPECT_TRUE((*planned)[0].proposed.contains("inactive_border = \"rgba(3b4261aa)\","));
        EXPECT_EQ((*planned)[3].proposed.substr(0, 30), "# my toolkit\nfont_size = 12\nba");
        EXPECT_TRUE((*planned)[3].proposed.contains("accent = 0xff7aa2f7\n"));

        ASSERT_TRUE(configuration::applyChanges(*planned, root / "backups", "Theme").has_value());
        EXPECT_EQ(desktop::currentTheme(config), &tokyo);
        const auto again = desktop::planTheme(tokyo, desktop::Shape::floating, config, environment);
        ASSERT_TRUE(again.has_value());
        EXPECT_TRUE(again->empty());
        std::filesystem::remove_all(root);
    }

    TEST(Theme, ThemesKittyWofiAndDunstWhenInstalled) {
        static std::atomic<int> counter = 0;
        const auto root = std::filesystem::temp_directory_path() / ("hyprland-studio-theme-apps-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
        std::filesystem::create_directories(root / "bin");
        for (const auto* program : {"kitty", "wofi", "dunst"}) {
            std::ofstream{root / "bin" / program} << "#!/bin/sh\n";
            std::filesystem::permissions(root / "bin" / program, std::filesystem::perms::owner_all);
        }
        std::filesystem::create_directories(root / "home/.config/hypr");
        std::filesystem::create_directories(root / "home/.config/kitty");
        std::ofstream{root / "home/.config/kitty/kitty.conf"} << "font_size 12\n";
        std::filesystem::create_directories(root / "home/.config/dunst");
        std::ofstream{root / "home/.config/dunst/dunstrc"} << "[global]\n    frame_color = \"#aaaaaa\"\n";
        std::ofstream{root / "home/.config/hypr/hyprland.lua"} << "hl.config({ general = { col = { active_border = \"x\", inactive_border = \"y\" } } })\n";
        const foundation::Environment  environment{{"HOME", (root / "home").string()}, {"PATH", (root / "bin").string()}};
        const configuration::MainConfig config{.file = root / "home/.config/hypr/hyprland.lua", .choice = configuration::Choice::search, .safeMode = false};

        const auto& nord    = desktop::waybarPalettes()[4];
        const auto  planned = desktop::planTheme(nord, desktop::Shape::islands, config, environment);
        ASSERT_TRUE(planned.has_value()) << planned.error();
        const auto find = [&planned](const std::string& name) -> const configuration::FileChange* {
            for (const auto& change : *planned)
                if (change.file.filename() == name)
                    return &change;
            return nullptr;
        };
        ASSERT_NE(find("current-theme.conf"), nullptr);
        EXPECT_TRUE(find("current-theme.conf")->proposed.contains("color14                 #8fbcbb\n"));
        ASSERT_NE(find("kitty.conf"), nullptr);
        EXPECT_EQ(find("kitty.conf")->proposed, "font_size 12\ninclude current-theme.conf\n");
        ASSERT_NE(find("style.css"), nullptr);
        ASSERT_NE(find("dunstrc"), nullptr);
        EXPECT_TRUE(find("dunstrc")->proposed.starts_with("[global]\n    frame_color = \"#88c0d0\"\n"));
        EXPECT_TRUE(find("dunstrc")->proposed.contains("[urgency_critical]\n    background = \"#2e3440\"\n"));
        std::filesystem::remove_all(root);
    }

}
