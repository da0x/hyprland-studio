// SPDX-License-Identifier: GPL-3.0-or-later
#include "ipc/events.hpp"

#include <gtest/gtest.h>

using ipc::Event;

TEST(EventSplitter, SplitsWholeLines) {
    ipc::EventSplitter splitter;
    EXPECT_EQ(splitter.feed("workspacev2>>2,2\nconfigreloaded>>\n"), (std::vector<Event>{{"workspacev2", "2,2"}, {"configreloaded", ""}}));
}

TEST(EventSplitter, KeepsALineCutInTwoUntilTheRestArrives) {
    ipc::EventSplitter splitter;
    EXPECT_TRUE(splitter.feed("openwindow>>5a1,1,kitty,a title, with").empty());
    EXPECT_EQ(splitter.feed(" a comma\nactivewindowv2>>5a1\n"),
              (std::vector<Event>{{"openwindow", "5a1,1,kitty,a title, with a comma"}, {"activewindowv2", "5a1"}}));
}

TEST(EventSplitter, SkipsLinesWithoutASeparator) {
    ipc::EventSplitter splitter;
    EXPECT_EQ(splitter.feed("garbage\nbell>>\n"), (std::vector<Event>{{"bell", ""}}));
}
