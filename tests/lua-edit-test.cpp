// SPDX-License-Identifier: GPL-3.0-or-later
#include "configuration/hyprlang-edit.hpp"
#include "configuration/lua-edit.hpp"

#include <gtest/gtest.h>

#include <string>

namespace {

    constexpr auto config = R"x(-- my config
local accent = "rgba(ff0000ff)"

hl.config({
    general = {
        gaps_in  = 5,

        col = {
            active_border   = { colors = {"rgba(33ccffee)", "rgba(00ff99ee)"}, angle = 45 },
            inactive_border = "rgba(595959aa)", -- grey
        },
    },
})

hl.config({ decoration = { rounding = 10 } })
)x";

    TEST(LuaEdit, ReplacesOnlyTheValue) {
        const auto edited = configuration::withLuaConfigValue(config, {"general", "col", "inactive_border"}, "\"rgba(45475aaa)\"");
        ASSERT_TRUE(edited.has_value()) << edited.error();
        std::string expected = config;
        expected.replace(expected.find("\"rgba(595959aa)\""), 16, "\"rgba(45475aaa)\"");
        EXPECT_EQ(*edited, expected);
    }

    TEST(LuaEdit, ReplacesATableValueWhole) {
        const auto edited = configuration::withLuaConfigValue(config, {"general", "col", "active_border"}, R"x({ colors = {"rgba(cba6f7ee)", "rgba(89b4faee)"}, angle = 45 })x");
        ASSERT_TRUE(edited.has_value());
        EXPECT_TRUE(edited->contains(R"x(active_border   = { colors = {"rgba(cba6f7ee)", "rgba(89b4faee)"}, angle = 45 },)x"));
        EXPECT_TRUE(edited->contains("local accent = \"rgba(ff0000ff)\""));
    }

    TEST(LuaEdit, FindsAValueInALaterCall) {
        const auto edited = configuration::withLuaConfigValue(config, {"decoration", "rounding"}, "12");
        ASSERT_TRUE(edited.has_value());
        EXPECT_TRUE(edited->contains("hl.config({ decoration = { rounding = 12 } })"));
    }

    TEST(LuaEdit, RefusesWhatTheConfigDoesNotWriteInPlace) {
        EXPECT_FALSE(configuration::withLuaConfigValue(config, {"general", "col", "nogroup_border"}, "\"x\"").has_value());
        EXPECT_FALSE(configuration::withLuaConfigValue("local c = {}\nhl.config(c)\n", {"general"}, "{}").has_value());
    }

    TEST(HyprlangEdit, SetsKeysOutsideBlocksAndAddsMissingOnes) {
        const auto config = "# colours\naccent = 0xff00ffcc   # mine\nwallpaper {\n    accent = 1\n}\ntext=0xffdadada\n";
        const auto edited = configuration::withHyprlangValues(config, {{"accent", "0xffcba6f7"}, {"text", "0xffcdd6f4"}, {"base", "0xff313244"}});
        EXPECT_EQ(edited, "# colours\naccent = 0xffcba6f7   # mine\nwallpaper {\n    accent = 1\n}\ntext=0xffcdd6f4\nbase = 0xff313244\n");
        EXPECT_EQ(configuration::withHyprlangValues("", {{"accent", "0x1"}}), "accent = 0x1\n");
    }

}
