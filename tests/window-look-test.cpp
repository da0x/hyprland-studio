// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/file-change.hpp"
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

}
