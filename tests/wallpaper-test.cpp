// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/wallpaper.hpp"

#include <gtest/gtest.h>

#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

namespace {

    using desktop::FitMode;
    using desktop::Wallpaper;

    TEST(Wallpaper, ChangesOnlyTheFallbackBlocksValues) {
        const std::string config = "# my wallpapers\n"
                                   "splash = false\n"
                                   "\n"
                                   "wallpaper {\n"
                                   "  monitor = DP-1\n"
                                   "  path = ~/left.png   # the left one\n"
                                   "}\n"
                                   "\n"
                                   "wallpaper {\n"
                                   "  monitor =\n"
                                   "  path    =  ~/old.png\n"
                                   "  fit_mode = cover\n"
                                   "}\n";
        const auto changed = desktop::withFallbackWallpaper(config, Wallpaper{"/home/me/new.jpg", FitMode::contain});
        ASSERT_TRUE(changed.has_value());
        EXPECT_EQ(*changed, "# my wallpapers\n"
                            "splash = false\n"
                            "\n"
                            "wallpaper {\n"
                            "  monitor = DP-1\n"
                            "  path = ~/left.png   # the left one\n"
                            "}\n"
                            "\n"
                            "wallpaper {\n"
                            "  monitor =\n"
                            "  path    =  /home/me/new.jpg\n"
                            "  fit_mode = contain\n"
                            "}\n");
        EXPECT_EQ(desktop::monitorsWithOwnWallpaper(config), std::vector<std::string>{"DP-1"});
        EXPECT_EQ(desktop::fallbackWallpaper(config, "/home/me"), (Wallpaper{"/home/me/old.png", FitMode::cover}));
        EXPECT_EQ(desktop::fallbackWallpaper(*changed, "/home/me"), (Wallpaper{"/home/me/new.jpg", FitMode::contain}));
    }

    TEST(Wallpaper, AddsAMissingFitModeWithTheBlocksIndentation) {
        const auto changed = desktop::withFallbackWallpaper("wallpaper {\n\tpath = /a.png\n}\n", Wallpaper{"/b.png", FitMode::tile});
        ASSERT_TRUE(changed.has_value());
        EXPECT_EQ(*changed, "wallpaper {\n\tpath = /b.png\n\tfit_mode = tile\n}\n");
    }

    TEST(Wallpaper, AddsABlockWhenThereIsNone) {
        EXPECT_EQ(desktop::withFallbackWallpaper("", Wallpaper{"/a.png", FitMode::cover}), "wallpaper {\n    monitor =\n    path = /a.png\n    fit_mode = cover\n}\n");
        EXPECT_EQ(desktop::withFallbackWallpaper("splash = false", Wallpaper{"/a.png", FitMode::fill}),
                  "splash = false\n\nwallpaper {\n    monitor =\n    path = /a.png\n    fit_mode = fill\n}\n");
    }

    TEST(Wallpaper, RefusesTheFormatBeforeHyprpaper08) {
        EXPECT_FALSE(desktop::withFallbackWallpaper("preload = /a.png\nwallpaper = ,/a.png\n", Wallpaper{"/b.png", FitMode::cover}).has_value());
    }

    TEST(Wallpaper, FindsImagesInTheUsualFolders) {
        static std::atomic<int> counter = 0;
        const auto root = std::filesystem::temp_directory_path() / ("hyprland-studio-wallpaper-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
        const auto make = [&root](const std::string& relative) {
            std::filesystem::create_directories((root / relative).parent_path());
            std::ofstream{root / relative} << "x";
        };
        make("home/Pictures/Wallpapers/b.png");
        make("home/Pictures/Wallpapers/forest/a.JPG");
        make("home/Pictures/notes.txt");
        make("home/Pictures/photo.webp");
        make("share/hypr/wall0.png");
        make("share/hypr/lockdead.png");
        make("share/backgrounds/sway/blue.png");

        const std::array system{root / "share" / "hypr", root / "share" / "backgrounds"};
        const auto       found = desktop::findWallpapers({{"HOME", (root / "home").string()}}, system);
        const std::vector<std::filesystem::path> expected{
            root / "home/Pictures/Wallpapers/b.png", root / "home/Pictures/Wallpapers/forest/a.JPG", root / "home/Pictures/photo.webp",
            root / "share/hypr/wall0.png",           root / "share/backgrounds/sway/blue.png",
        };
        EXPECT_EQ(found, expected);
        std::filesystem::remove_all(root);
    }

    TEST(Wallpaper, AsksHyprctlInItsOwnWords) {
        EXPECT_EQ(desktop::showWallpaperArguments(Wallpaper{"/a b/c.png", FitMode::fill}), (std::vector<std::string>{"hyprpaper", "wallpaper", ", /a b/c.png, stretch"}));
        EXPECT_FALSE(desktop::showWallpaperArguments(Wallpaper{"/a,b.png", FitMode::cover}).has_value());
    }

    TEST(Wallpaper, AsksAwwwToFadeIn) {
        EXPECT_EQ(desktop::awwwArguments(Wallpaper{"/a.png", FitMode::contain}),
                  (std::vector<std::string>{"img", "/a.png", "--resize", "fit", "--transition-type", "fade"}));
        EXPECT_FALSE(desktop::awwwArguments(Wallpaper{"/a.png", FitMode::tile}).has_value());
    }

    TEST(Wallpaper, ReadsWhatAwwwShows) {
        const auto shown = R"({"": [{"name": "DP-10", "width": 3840, "height": 2160, "scale": 1.0, "displaying": {"image": "/usr/share/hypr/wall2.png"}}]})";
        EXPECT_EQ(desktop::awwwImage(shown), std::filesystem::path{"/usr/share/hypr/wall2.png"});
        EXPECT_FALSE(desktop::awwwImage(R"({"": [{"name": "DP-10", "displaying": {"color": "#000000"}}]})").has_value());
        EXPECT_FALSE(desktop::awwwImage("not json").has_value());
    }

}
