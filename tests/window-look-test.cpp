// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/file-change.hpp"
#include "desktop/waybar-style.hpp"
#include "desktop/window-look.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

namespace {

    TEST(WindowLook, ReadsAndChangesOnlyWhatDiffers) {
        static std::atomic<int> counter = 0;
        const auto root = std::filesystem::temp_directory_path() / ("hyprland-studio-windows-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
        std::filesystem::create_directories(root);
        const auto lua = root / "hyprland.lua";
        std::ofstream{lua} << "hl.config({\n    general = {\n        gaps_in  = 5,\n        gaps_out = \"20 10 20 10\",\n    },\n    decoration = {\n        rounding = 10,\n    },\n})\n";
        const configuration::MainConfig config{.file = lua, .choice = configuration::Choice::search, .safeMode = false};

        // Border size and resizing are left to Hyprland's defaults; four sides read as the first.
        const auto read = desktop::currentWindowLook(config);
        ASSERT_TRUE(read.has_value());
        EXPECT_EQ(*read, (desktop::WindowLook{.rounding = 10, .gapsIn = 5, .gapsOut = 20, .borderSize = 1, .resizeOnBorder = false}));
        const auto same = desktop::planWindowLook(*read, config);
        ASSERT_TRUE(same.has_value()) << same.error();
        EXPECT_FALSE(same->has_value());

        auto wanted           = *read;
        wanted.rounding       = 0;
        wanted.resizeOnBorder = true;
        const auto planned    = desktop::planWindowLook(wanted, config);
        ASSERT_TRUE(planned.has_value() && planned->has_value());
        const auto& text = (*planned)->proposed;
        EXPECT_TRUE(text.contains("        rounding = 0,\n"));
        EXPECT_TRUE(text.contains("gaps_out = \"20 10 20 10\""));
        EXPECT_TRUE(text.ends_with("\nhl.config({ general = { resize_on_border = true } })\n"));
        EXPECT_FALSE(text.contains("border_size"));

        ASSERT_TRUE(configuration::applyChanges({**planned}, root / "backups", "Windows").has_value());
        EXPECT_EQ(desktop::currentWindowLook(config), wanted);
        std::filesystem::remove_all(root);
    }

    TEST(WindowLook, TheBarFollowsTheWindowsCornersAndGap) {
        static std::atomic<int> counter = 0;
        const auto root = std::filesystem::temp_directory_path() / ("hyprland-studio-bar-follows-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
        std::filesystem::create_directories(root / "home/.config/waybar");
        const foundation::Environment environment{{"HOME", (root / "home").string()}};
        // A bar written before it followed the windows, with the fixed geometry.
        const desktop::WaybarStyle old{desktop::Shape::floating, &desktop::waybarPalettes()[0], {}};
        const auto                 written = desktop::planWaybarStyle(old, environment);
        ASSERT_TRUE(written.has_value());
        ASSERT_TRUE(configuration::applyChanges(*written, root / "backups", "Waybar").has_value());

        const desktop::WindowLook square{.rounding = 0, .gapsIn = 2, .gapsOut = 10, .borderSize = 1, .resizeOnBorder = true};
        const auto                planned = desktop::planBarFollowing(square, desktop::WindowLook{.rounding = 4, .gapsIn = 5, .gapsOut = 30, .borderSize = 2, .resizeOnBorder = false}, environment);
        ASSERT_TRUE(planned.has_value()) << planned.error();
        ASSERT_EQ(planned->size(), 2U);
        EXPECT_TRUE((*planned)[0].proposed.contains("\"margin-top\": 10,"));
        EXPECT_TRUE((*planned)[0].proposed.contains("\"margin-left\": 10,"));
        EXPECT_TRUE((*planned)[1].proposed.contains("border-radius: 0px;"));
        EXPECT_FALSE((*planned)[1].proposed.contains("border-radius: 10px;"));
        // The bar takes the windows' border: their width, the inactive colour, and the
        // active one's gradient on hover, since the corners are square.
        EXPECT_TRUE((*planned)[1].proposed.contains("border: 1px solid alpha(@raised, 0.67);"));
        EXPECT_TRUE((*planned)[1].proposed.contains("window#waybar:hover {\n    border-image: linear-gradient(45deg, alpha(@accent, 0.93), alpha(@audio, 0.93)) 1;"));
        ASSERT_TRUE(configuration::applyChanges(*planned, root / "backups", "Windows").has_value());
        EXPECT_EQ(desktop::currentWaybarStyle(environment, desktop::barGeometry(square)), (desktop::WaybarStyle{old.shape, old.palette, desktop::barGeometry(square)}));

        // A bar written when the top got half the gap is still read, and given all of it.
        const desktop::WaybarStyle halfTop{old.shape, old.palette, {.rounding = 0, .edgeGap = 10, .topGap = 5, .border = std::nullopt}};
        const auto                 rewritten = desktop::planWaybarStyle(halfTop, environment);
        ASSERT_TRUE(rewritten.has_value());
        ASSERT_TRUE(configuration::applyChanges(*rewritten, root / "backups", "Waybar").has_value());
        const auto again = desktop::planBarFollowing(square, square, environment);
        ASSERT_TRUE(again.has_value()) << again.error();
        // The config for the gap, and the stylesheet for the windows' border.
        ASSERT_EQ(again->size(), 2U);
        EXPECT_TRUE((*again)[0].proposed.contains("\"margin-top\": 10,"));

        // A bar the studio did not write is left alone.
        std::ofstream{root / "home/.config/waybar/style.css"} << "/* mine */\n";
        const auto mine = desktop::planBarFollowing(square, square, environment);
        ASSERT_TRUE(mine.has_value());
        EXPECT_TRUE(mine->empty());
        std::filesystem::remove_all(root);
    }

}
