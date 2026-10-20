// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/colour.hpp"

#include <gtest/gtest.h>

namespace {

    using desktop::parseColour;

    TEST(Colour, ReadsAndWritesHex) {
        EXPECT_EQ(desktop::hexOf(parseColour("#1E1E2E")), "#1e1e2e");
        EXPECT_EQ(desktop::hexOf(parseColour("nonsense")), "#000000");
    }

    TEST(Colour, MeasuresContrastAsWcagDoes) {
        EXPECT_NEAR(desktop::contrast(parseColour("#000000"), parseColour("#ffffff")), 21.0, 1e-9);
        EXPECT_NEAR(desktop::contrast(parseColour("#777777"), parseColour("#ffffff")), 4.48, 0.01);
    }

    TEST(Colour, GoesThroughOklchAndBack) {
        for (const auto* hex : {"#f38ba8", "#a6e3a1", "#1e66f5", "#000000", "#ffffff", "#808080"}) {
            const auto colour = parseColour(hex);
            EXPECT_EQ(desktop::hexOf(desktop::colourOf(desktop::lchOf(colour))), hex);
        }
        // Red is near hue 25, blue near 265.
        EXPECT_NEAR(desktop::lchOf(parseColour("#ff0000")).hue, 29.2, 0.5);
        EXPECT_NEAR(desktop::lchOf(parseColour("#0000ff")).hue, 264.1, 0.5);
    }

    TEST(Colour, ReachesAContrastKeepingTheHue) {
        const auto background = parseColour("#282a36");
        const auto dim        = parseColour("#6272a4");
        const auto raised     = desktop::withContrast(dim, background, 7.0);
        EXPECT_GE(desktop::contrast(raised, background), 7.0);
        EXPECT_LT(desktop::contrast(raised, background), 7.3);
        EXPECT_NEAR(desktop::lchOf(raised).hue, desktop::lchOf(dim).hue, 3.0);

        // On a light background, darker.
        const auto light = parseColour("#fbf1c7");
        const auto dark  = desktop::withContrast(parseColour("#d79921"), light, 7.0);
        EXPECT_GE(desktop::contrast(dark, light), 7.0);
        EXPECT_LT(desktop::lchOf(dark).lightness, desktop::lchOf(parseColour("#d79921")).lightness);
    }

}
