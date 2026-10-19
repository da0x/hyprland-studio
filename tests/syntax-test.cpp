// SPDX-License-Identifier: GPL-3.0-or-later
#include "syntax/highlight.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

namespace {

    std::string readFixture() {
        std::ifstream      file{std::filesystem::path{FIXTURES} / "lua-config" / "hyprland.lua"};
        std::ostringstream text;
        text << file.rdbuf();
        return text.str();
    }

    // The kind of the span that covers all of `piece`, found after `after`, if one does.
    std::optional<syntax::Kind> kindOf(const std::string& source, const std::vector<syntax::Span>& spans, const std::string& piece, const std::string& after = {}) {
        const auto start = source.find(after);
        const auto at    = source.find(piece, start == std::string::npos ? 0 : start);
        if (at == std::string::npos)
            return std::nullopt;
        for (const auto& span : spans)
            if (span.begin <= at && at + piece.size() <= span.end)
                return span.kind;
        return std::nullopt;
    }

}

TEST(Syntax, ColoursEachPartOfAConfig) {
    const auto source = readFixture();
    const auto spans  = syntax::highlightLua(source);

    EXPECT_EQ(kindOf(source, spans, "-- A config written for the tests"), syntax::Kind::comment);
    EXPECT_EQ(kindOf(source, spans, "local", "\n\n"), syntax::Kind::keyword);
    EXPECT_EQ(kindOf(source, spans, "\"kitty\""), syntax::Kind::string);
    EXPECT_EQ(kindOf(source, spans, "5", "gaps_in"), syntax::Kind::number);
    EXPECT_EQ(kindOf(source, spans, "gaps_in"), syntax::Kind::field);
    EXPECT_EQ(kindOf(source, spans, "bind", "hl."), syntax::Kind::function);
    EXPECT_EQ(kindOf(source, spans, "exec_cmd", "hl.dsp."), syntax::Kind::function);
    EXPECT_EQ(kindOf(source, spans, "true", "mouse = "), syntax::Kind::constant);
    EXPECT_EQ(kindOf(source, spans, "..", "mainMod "), syntax::Kind::operation);
    EXPECT_EQ(kindOf(source, spans, "mainMod", "hl.bind("), std::nullopt);
}

TEST(Syntax, SpansAreInOrderAndApart) {
    const auto source = readFixture();
    const auto spans  = syntax::highlightLua(source);
    ASSERT_FALSE(spans.empty());
    for (std::size_t index = 0; index < spans.size(); ++index) {
        EXPECT_LT(spans[index].begin, spans[index].end);
        EXPECT_LE(spans[index].end, source.size());
        if (index > 0) {
            EXPECT_LE(spans[index - 1].end, spans[index].begin);
        }
    }
}

TEST(Syntax, ColoursWhatItCanOfABrokenSource) {
    // A bracket left open, the kind of mistake made while editing.
    const std::string source = "hl.bind(\"SUPER + Q\", hl.dsp.exec_cmd(\"kitty\")\n-- next\nlocal gaps = 5\n";
    const auto        spans  = syntax::highlightLua(source);
    EXPECT_EQ(kindOf(source, spans, "\"kitty\""), syntax::Kind::string);
    EXPECT_EQ(kindOf(source, spans, "-- next"), syntax::Kind::comment);
    EXPECT_EQ(kindOf(source, spans, "local"), syntax::Kind::keyword);
}

TEST(Syntax, FindsTheFunctionOnALine) {
    const auto source = readFixture();
    // hl.bind("SUPER + F", function () ... end) starts on line 23.
    const auto found = syntax::luaFunctionAt(source, 23);
    ASSERT_TRUE(found);
    const auto text = std::string_view{source}.substr(found->begin, found->end - found->begin);
    EXPECT_TRUE(text.starts_with("function ()"));
    EXPECT_TRUE(text.ends_with("end"));
    EXPECT_NE(text.find("hl.dispatch"), std::string_view::npos);

    EXPECT_FALSE(syntax::luaFunctionAt(source, 16));
    EXPECT_FALSE(syntax::luaFunctionAt(source, 0));
}

TEST(Syntax, FindsANamedFunctionWhole) {
    const std::string source = "local x = 1\nlocal function toggle(name)\n    return name\nend\n";
    const auto        found  = syntax::luaFunctionAt(source, 2);
    ASSERT_TRUE(found);
    EXPECT_EQ(std::string_view{source}.substr(found->begin, found->end - found->begin), "local function toggle(name)\n    return name\nend");
}
