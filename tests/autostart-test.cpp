// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/autostart.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace {

    constexpr std::array<std::string_view, 1> hyprpaper{"hyprpaper"};
    constexpr std::array<std::string_view, 1> awww{"awww-daemon"};

    TEST(Autostart, AddsTheLineAtTheEndOfTheStartBlock) {
        const auto config = "-- autostart\n"
                            "hl.on(\"hyprland.start\", function ()\n"
                            "    hl.exec_cmd(\"waybar\")\n"
                            "end)\n"
                            "\n"
                            "hl.env(\"A\", \"b\")\n";
        EXPECT_EQ(configuration::withAutostart(config, true, "awww-daemon", hyprpaper), "-- autostart\n"
                                                                                       "hl.on(\"hyprland.start\", function ()\n"
                                                                                       "    hl.exec_cmd(\"waybar\")\n"
                                                                                       "    hl.exec_cmd(\"awww-daemon\")\n"
                                                                                       "end)\n"
                                                                                       "\n"
                                                                                       "hl.env(\"A\", \"b\")\n");
        EXPECT_FALSE(configuration::startsAtLogin(config, true, "awww-daemon"));
    }

    TEST(Autostart, SwapsOneProgramForTheOther) {
        const auto config = "hl.on(\"hyprland.start\", function ()\n"
                            "\thl.exec_cmd(\"waybar\")\n"
                            "\thl.exec_cmd(\"awww-daemon\")\n"
                            "end)\n";
        const auto swapped = configuration::withAutostart(config, true, "hyprpaper", awww);
        EXPECT_EQ(swapped, "hl.on(\"hyprland.start\", function ()\n"
                           "\thl.exec_cmd(\"waybar\")\n"
                           "\thl.exec_cmd(\"hyprpaper\")\n"
                           "end)\n");
        EXPECT_TRUE(configuration::startsAtLogin(swapped, true, "hyprpaper"));
        EXPECT_FALSE(configuration::startsAtLogin(swapped, true, "awww-daemon"));
    }

    TEST(Autostart, LeavesAConfigThatAlreadyStartsItAsItIs) {
        const auto config = "hl.on(\"hyprland.start\", function ()\n    hl.exec_cmd(\"hyprpaper\")\nend)\n";
        EXPECT_EQ(configuration::withAutostart(config, true, "hyprpaper", awww), config);
    }

    TEST(Autostart, AddsABlockWhenThereIsNone) {
        EXPECT_EQ(configuration::withAutostart("hl.env(\"A\", \"b\")", true, "hyprpaper", awww),
                  "hl.env(\"A\", \"b\")\n\nhl.on(\"hyprland.start\", function ()\n    hl.exec_cmd(\"hyprpaper\")\nend)\n");
    }

    TEST(Autostart, LeavesOtherWaysOfStartingAProgramAlone) {
        const auto config = "hl.on(\"hyprland.start\", function ()\n    hl.exec_cmd(\"awww-daemon --quiet\")\nend)\n";
        const auto edited = configuration::withAutostart(config, true, "hyprpaper", awww);
        EXPECT_TRUE(edited.contains("awww-daemon --quiet"));
    }

    TEST(Autostart, WritesExecOnceInHyprlandConf) {
        const auto config = "exec-once = waybar\nexec-once=hyprpaper\n$mod = SUPER\n";
        EXPECT_EQ(configuration::withAutostart(config, false, "awww-daemon", hyprpaper), "exec-once = waybar\n$mod = SUPER\n\nexec-once = awww-daemon\n");
        EXPECT_TRUE(configuration::startsAtLogin(config, false, "hyprpaper"));
    }

}
