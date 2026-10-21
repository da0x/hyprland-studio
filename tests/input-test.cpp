// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/file-change.hpp"
#include "desktop/input.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

namespace {

    TEST(Input, ReadsLayoutsAndSplitsTheOptions) {
        static std::atomic<int> counter = 0;
        const auto root = std::filesystem::temp_directory_path() / ("lantern-hyprland-input-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
        std::filesystem::create_directories(root);
        const auto lua = root / "hyprland.lua";
        std::ofstream{lua} << "hl.config({\n    input = {\n        kb_layout  = \"us,ara\",\n        kb_variant = \",qwerty\",\n"
                              "        kb_options = \"grp:alt_shift_toggle,compose:ralt\",\n        sensitivity = 0, -- -1.0 - 1.0\n    },\n})\n";
        const configuration::MainConfig config{.file = lua, .choice = configuration::Choice::search, .safeMode = false};

        const auto read = desktop::currentInput(config);
        ASSERT_TRUE(read.has_value());
        EXPECT_EQ(read->layouts, (std::vector<desktop::Layout>{{"us", ""}, {"ara", "qwerty"}}));
        EXPECT_EQ(read->switchKeys, "grp:alt_shift_toggle");
        EXPECT_EQ(read->capsLock, "");
        EXPECT_EQ(read->otherOptions, std::vector<std::string>{"compose:ralt"});
        EXPECT_EQ(read->repeatRate, 25);
        const auto same = desktop::planInput(*read, config);
        ASSERT_TRUE(same.has_value()) << same.error();
        EXPECT_FALSE(same->has_value());

        // Caps Lock as Escape joins the options, the compose key kept; a layout without a
        // variant leaves no variant behind.
        auto wanted       = *read;
        wanted.capsLock   = "caps:escape";
        wanted.layouts    = {{"us", ""}, {"de", ""}};
        wanted.repeatRate = 40;
        wanted.flat       = true;
        const auto planned = desktop::planInput(wanted, config);
        ASSERT_TRUE(planned.has_value() && planned->has_value());
        const auto& text = (*planned)->proposed;
        EXPECT_TRUE(text.contains("kb_layout  = \"us,de\",\n")) << text;
        EXPECT_TRUE(text.contains("kb_variant = \"\",\n")) << text;
        EXPECT_TRUE(text.contains("kb_options = \"grp:alt_shift_toggle,caps:escape,compose:ralt\",\n")) << text;
        EXPECT_TRUE(text.contains("sensitivity = 0, -- -1.0 - 1.0\n")) << text;
        EXPECT_TRUE(text.contains("hl.config({ input = { repeat_rate = 40 } })")) << text;
        EXPECT_TRUE(text.contains("hl.config({ input = { accel_profile = \"flat\" } })")) << text;

        ASSERT_TRUE(configuration::applyChanges({**planned}, root / "backups", "Keyboard").has_value());
        EXPECT_EQ(desktop::currentInput(config), wanted);
        std::filesystem::remove_all(root);
    }

    TEST(Input, ReadsXkbsLayoutsAndTheirVariants) {
        const auto layouts = desktop::parseLayouts("! model\n  pc105           Generic 105-key PC\n\n! layout\n  us              English (US)\n  ara             Arabic\n\n"
                                                   "! variant\n  chr             us: Cherokee\n  qwerty          ara: Arabic (QWERTY)\n\n! option\n  grp                  Switching\n");
        ASSERT_EQ(layouts.size(), 2U);
        EXPECT_EQ(layouts[0].code, "ara");
        ASSERT_EQ(layouts[0].variants.size(), 1U);
        EXPECT_EQ(layouts[0].variants[0].name, "Arabic (QWERTY)");
        EXPECT_EQ(layouts[1].name, "English (US)");
        EXPECT_EQ(layouts[1].variants[0].code, "chr");
    }

}
