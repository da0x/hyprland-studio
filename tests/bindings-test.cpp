// SPDX-License-Identifier: GPL-3.0-or-later
#include "ipc/bindings.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

namespace {

    std::string fixture(const char* name) {
        std::ifstream     file{std::string{FIXTURES} + "/" + name};
        std::stringstream text;
        text << file.rdbuf();
        return text.str();
    }

}

TEST(Bindings, ReadsHyprlandsReplyUnderLua) {
    const auto bindings = ipc::parseBindings(fixture("binds-lua.json"));
    ASSERT_TRUE(bindings) << bindings.error();
    ASSERT_EQ(bindings->size(), 3U);

    const auto& first = bindings->at(0);
    EXPECT_EQ(first.modifiers.with(first.key), "SUPER + Q");
    EXPECT_TRUE(first.fromLua());
    EXPECT_TRUE(first.description.empty());

    const auto& volume = bindings->at(1);
    EXPECT_EQ(volume.modifiers.with(volume.key), "XF86AudioRaiseVolume");
    EXPECT_EQ(volume.description, "Raise the volume");
    EXPECT_TRUE(volume.locked);
    EXPECT_TRUE(volume.repeat);

    const auto& drag = bindings->at(2);
    EXPECT_EQ(drag.modifiers.with(drag.key), "SUPER + SHIFT + mouse:272");
    EXPECT_EQ(drag.submap, "resize");
    EXPECT_TRUE(drag.mouse);
}

TEST(Bindings, ToleratesFieldsOfAnUnexpectedType) {
    const auto bindings = ipc::parseBindings(R"([{"modmask": "64", "key": 5, "locked": "true"}])");
    ASSERT_TRUE(bindings) << bindings.error();
    ASSERT_EQ(bindings->size(), 1U);
    EXPECT_TRUE(bindings->front().modifiers.empty());
    EXPECT_TRUE(bindings->front().key.empty());
    EXPECT_FALSE(bindings->front().locked);
}

TEST(Bindings, RejectsWhatIsNotAListOfObjects) {
    EXPECT_FALSE(ipc::parseBindings("not json"));
    EXPECT_FALSE(ipc::parseBindings(R"({"key": "Q"})"));
    EXPECT_FALSE(ipc::parseBindings(R"([1, 2])"));
    EXPECT_TRUE(ipc::parseBindings("[]"));
}
