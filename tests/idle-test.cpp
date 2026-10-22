// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/idle.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {

    TEST(Idle, WritesHypridleFromNothing) {
        const desktop::IdleSettings settings{.dim = 0, .lock = 5, .screensOff = 6, .suspend = 0, .lockBeforeSleep = true};
        const auto                  text = desktop::withIdle("", settings);
        EXPECT_EQ(text, "general {\n    lock_cmd = pidof hyprlock || hyprlock\n    before_sleep_cmd = loginctl lock-session\n"
                        "    after_sleep_cmd = hyprctl dispatch 'hl.dsp.dpms({ action = \"on\" })'\n}\n\n"
                        "listener {\n    timeout = 300\n    on-timeout = loginctl lock-session\n}\n\n"
                        "listener {\n    timeout = 360\n    on-timeout = hyprctl dispatch 'hl.dsp.dpms({ action = \"off\" })'\n"
                        "    on-resume = hyprctl dispatch 'hl.dsp.dpms({ action = \"on\" })'\n}\n");
        EXPECT_EQ(desktop::idleIn(text), settings);
    }

    TEST(Idle, KeepsWhatItDoesNotKnow) {
        const std::string config = "# mine\ngeneral {\n    lock_cmd = hyprlock\n    before_sleep_cmd = loginctl lock-session  # lock first\n}\n\n"
                                   "listener {\n    timeout = 150\n    on-timeout = notify-send away\n}\n\n"
                                   "listener {\n    timeout = 600\n    on-timeout = loginctl lock-session\n}\n";
        const auto read = desktop::idleIn(config);
        EXPECT_EQ(read.lock, 10);
        EXPECT_TRUE(read.lockBeforeSleep);

        const desktop::IdleSettings wanted{.dim = 0, .lock = 15, .screensOff = 0, .suspend = 30, .lockBeforeSleep = false};
        const auto                  text = desktop::withIdle(config, wanted);
        EXPECT_TRUE(text.starts_with("# mine\ngeneral {\n    lock_cmd = hyprlock\n    after_sleep_cmd = hyprctl dispatch")) << text;
        EXPECT_FALSE(text.contains("before_sleep_cmd")) << text;
        EXPECT_TRUE(text.contains("listener {\n    timeout = 150\n    on-timeout = notify-send away\n}\n\nlistener {\n    timeout = 900\n")) << text;
        EXPECT_TRUE(text.ends_with("listener {\n    timeout = 1800\n    on-timeout = systemctl suspend\n}\n")) << text;
        EXPECT_FALSE(text.contains("\n\n\n")) << text;
        EXPECT_EQ(desktop::idleIn(text), wanted);
    }

    TEST(Idle, DrawsTheLockScreenInThePalette) {
        const auto& palette = desktop::waybarPalettes().front();
        const auto  text    = desktop::lockScreen(palette);
        EXPECT_TRUE(desktop::lanternLockScreen(text));
        EXPECT_FALSE(desktop::lanternLockScreen("general {\n}\n"));
        EXPECT_TRUE(text.contains(std::format("outer_color = rgb({})", palette.accent.substr(1)))) << text;
    }

}
