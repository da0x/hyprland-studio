// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/system.hpp"
#include "desktop/waybar.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <unistd.h>

namespace {

    // A folder of its own for one test, removed afterwards.
    class Folder {
      public:
        Folder() {
            static std::atomic<int> counter = 0;
            root = std::filesystem::temp_directory_path() / ("hyprland-studio-desktop-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
            std::filesystem::create_directories(root);
        }
        ~Folder() { std::filesystem::remove_all(root); }
        Folder(const Folder&)            = delete;
        Folder& operator=(const Folder&) = delete;

        // Creates the file and returns its path.
        std::filesystem::path add(const std::filesystem::path& relative) const {
            const auto file = root / relative;
            std::filesystem::create_directories(file.parent_path());
            std::ofstream{file} << "{}\n";
            return file;
        }

        std::filesystem::path root;
    };

    TEST(Desktop, FindsAnExecutableOnThePath) {
        const Folder folder;
        const auto   plain   = folder.add("first/waybar");
        const auto   program = folder.add("second/waybar");
        std::filesystem::permissions(program, std::filesystem::perms::owner_exec, std::filesystem::perm_options::add);

        // The first folder's file is not executable, so the second one's is found.
        const auto path  = (folder.root / "first").string() + "::" + (folder.root / "second").string();
        const auto found = desktop::findProgram("waybar", path);
        ASSERT_TRUE(found.has_value());
        EXPECT_EQ(*found, program);
        EXPECT_FALSE(desktop::findProgram("waybar", (folder.root / "first").string()).has_value());
        EXPECT_NE(plain, program);
    }

    TEST(Desktop, ReadsOsRelease) {
        const auto arch = desktop::parseOsRelease("NAME=\"Arch Linux\"\nPRETTY_NAME=\"Arch Linux\"\nID=arch\n# a comment\n");
        EXPECT_EQ(arch.name, "Arch Linux");
        EXPECT_TRUE(arch.arch());

        const auto derived = desktop::parseOsRelease("NAME='EndeavourOS'\nID=endeavouros\nID_LIKE=\"arch\"\n");
        EXPECT_EQ(derived.name, "EndeavourOS");
        EXPECT_TRUE(derived.arch());

        const auto other = desktop::parseOsRelease("NAME=Fedora\nID=fedora\n");
        EXPECT_FALSE(other.arch());
    }

    TEST(Desktop, NamesTheInstallCommandOnlyWhereItKnowsThePackages) {
        const desktop::Requirement font{.name = "Icon font", .purpose = "", .upstream = "", .package = "ttf-nerd-fonts-symbols", .found = std::nullopt};
        EXPECT_EQ(desktop::installCommand(font, desktop::parseOsRelease("ID=arch\n")), "sudo pacman -S ttf-nerd-fonts-symbols");
        EXPECT_EQ(desktop::installCommand(font, desktop::parseOsRelease("ID=fedora\n")), "");
    }

    TEST(Desktop, DoesNotFindAFontThatDoesNotExist) {
        EXPECT_FALSE(desktop::hasFontFamily("No Such Font Family For Hyprland Studio"));
    }

    TEST(Desktop, FindsWaybarsConfigInWaybarsOrder) {
        const Folder folder;
        const auto   system = folder.root / "etc";
        const auto   example = folder.add("etc/waybar/config.jsonc");
        foundation::Environment environment{{"HOME", (folder.root / "home").string()}};
        EXPECT_EQ(desktop::waybarConfig(environment, system), example);

        const auto user = folder.add("home/.config/waybar/config.jsonc");
        EXPECT_EQ(desktop::waybarConfig(environment, system), user);

        // In one folder, config comes before config.jsonc.
        const auto plain = folder.add("home/.config/waybar/config");
        EXPECT_EQ(desktop::waybarConfig(environment, system), plain);

        // $XDG_CONFIG_HOME comes before ~/.config, and $WAYBAR_CONFIG_DIR before both.
        const auto xdg = folder.add("xdg/waybar/config.jsonc");
        environment["XDG_CONFIG_HOME"] = (folder.root / "xdg").string();
        EXPECT_EQ(desktop::waybarConfig(environment, system), xdg);
        const auto chosen = folder.add("chosen/config.jsonc");
        environment["WAYBAR_CONFIG_DIR"] = (folder.root / "chosen").string();
        EXPECT_EQ(desktop::waybarConfig(environment, system), chosen);
    }

    TEST(Desktop, FindsNoWaybarConfigWhereThereIsNone) {
        const Folder folder;
        EXPECT_FALSE(desktop::waybarConfig({{"HOME", folder.root.string()}}, folder.root / "etc").has_value());
    }

}
