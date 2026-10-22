// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/fastfetch.hpp"

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

namespace {

    TEST(Fastfetch, WritesAndReadsItsConfig) {
        const desktop::Fastfetch fetch{.logo = desktop::FetchLogo::small, .accent = "magenta", .full = true};
        const auto               text = desktop::fastfetchConfig(fetch);
        EXPECT_EQ(desktop::fastfetchIn(text), fetch);
        const auto json = nlohmann::json::parse(text, nullptr, false, true);
        ASSERT_FALSE(json.is_discarded());
        EXPECT_EQ(json["logo"]["type"], "small");
        EXPECT_EQ(json["logo"]["color"]["1"], "magenta");
        EXPECT_EQ(json["display"]["color"]["keys"], "magenta");
        EXPECT_EQ(json["modules"].front(), "title");

        EXPECT_EQ(desktop::fastfetchIn(desktop::fastfetchConfig({})), desktop::Fastfetch{});
        EXPECT_EQ(desktop::fastfetchIn(desktop::fastfetchConfig({.logo = desktop::FetchLogo::none, .accent = "red", .full = false}))->logo, desktop::FetchLogo::none);
        EXPECT_FALSE(desktop::fastfetchIn("{ \"logo\": null }").has_value());
    }

    TEST(Fastfetch, GreetsInEachNewTerminal) {
        const std::string bashrc = "[[ $- != *i* ]] && return\nalias ls='ls --color=auto'\n";
        const auto        on     = desktop::withFastfetchGreeting(bashrc, true);
        EXPECT_TRUE(desktop::greetsWithFastfetch(on));
        EXPECT_TRUE(on.starts_with(bashrc));
        EXPECT_TRUE(on.ends_with("command -v fastfetch >/dev/null && fastfetch\n"));
        EXPECT_EQ(desktop::withFastfetchGreeting(on, false), bashrc);
        EXPECT_FALSE(desktop::greetsWithFastfetch(bashrc));
        EXPECT_FALSE(desktop::greetsWithFastfetch(desktop::withFastfetchGreeting("echo hi\nfastfetch\n", false)));
    }

}
