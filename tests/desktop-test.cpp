// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/clipboard.hpp"
#include "desktop/process.hpp"
#include "desktop/system.hpp"
#include "desktop/waybar-style.hpp"
#include "desktop/waybar.hpp"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>
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

    TEST(Desktop, CopiesThroughTheClipboardProgram) {
        const Folder folder;
        // A stand-in for wl-copy that writes what it was given to a file.
        const auto received = folder.root / "received";
        const auto program  = folder.root / "bin" / "wl-copy";
        std::filesystem::create_directories(program.parent_path());
        std::ofstream{program} << "#!/bin/sh\n[ \"$1\" = -- ] || exit 2\nprintf '%s' \"$2\" > '" << received.string() << "'\n";
        std::filesystem::permissions(program, std::filesystem::perms::owner_all, std::filesystem::perm_options::add);

        const auto found = desktop::clipboardProgram((folder.root / "bin").string());
        ASSERT_TRUE(found.has_value());
        ASSERT_TRUE(desktop::copyText(*found, "sudo pacman -S ttf-nerd-fonts-symbols").has_value());
        std::stringstream text;
        text << std::ifstream{received}.rdbuf();
        EXPECT_EQ(text.str(), "sudo pacman -S ttf-nerd-fonts-symbols");
    }

    TEST(Desktop, ReportsAClipboardProgramThatFails) {
        const Folder folder;
        const auto   program = folder.root / "wl-copy";
        std::ofstream{program} << "#!/bin/sh\nexit 1\n";
        std::filesystem::permissions(program, std::filesystem::perms::owner_all, std::filesystem::perm_options::add);
        EXPECT_FALSE(desktop::copyText(program, "text").has_value());
    }

    TEST(Desktop, FindsTheUsersLiveProcessesByName) {
        const Folder folder;
        const auto   process = [&folder](const std::string& id, const std::string& name, char state) {
            std::filesystem::create_directories(folder.root / id);
            std::ofstream{folder.root / id / "comm"} << name << "\n";
            std::ofstream{folder.root / id / "stat"} << id << " (" << name << ") " << state << " 1 2 3\n";
        };
        process("120", "waybar", 'S');
        process("7", "waybar", 'R');
        process("300", "waybar", 'Z');
        process("44", "kitty", 'S');
        std::filesystem::create_directories(folder.root / "self");

        EXPECT_EQ(desktop::findProcesses("waybar", folder.root), (std::vector<pid_t>{7, 120}));
    }

    TEST(Desktop, StartsAProgramDetached) {
        const Folder folder;
        const auto   started = folder.root / "started";
        const auto   program = folder.root / "program";
        std::ofstream{program} << "#!/bin/sh\necho $PPID > '" << started.string() << "'\n";
        std::filesystem::permissions(program, std::filesystem::perms::owner_all, std::filesystem::perm_options::add);

        ASSERT_TRUE(desktop::startDetached(program).has_value());
        for (int wait = 0; wait < 100 && !std::filesystem::exists(started); ++wait)
            ::usleep(20'000);
        ASSERT_TRUE(std::filesystem::exists(started));
        // Not the studio's child.
        std::string parent;
        std::ifstream{started} >> parent;
        EXPECT_NE(parent, std::to_string(::getpid()));
    }

    TEST(Desktop, PlansTheWaybarStyleBesideTheConfigWaybarReads) {
        const Folder folder;
        const auto   system = folder.root / "etc";
        folder.add("etc/waybar/config.jsonc");
        const foundation::Environment environment{{"HOME", (folder.root / "home").string()}};
        const desktop::WaybarStyle     style{desktop::Shape::islands, &desktop::waybarPalettes().front()};

        // Only the system's example: the files go to ~/.config/waybar, both new.
        const auto fresh = desktop::planWaybarStyle(style, environment, system);
        ASSERT_TRUE(fresh.has_value()) << fresh.error();
        ASSERT_EQ(fresh->size(), 2U);
        EXPECT_EQ((*fresh)[0].file, folder.root / "home/.config/waybar/config.jsonc");
        EXPECT_FALSE((*fresh)[0].current.has_value());
        EXPECT_EQ((*fresh)[1].file, folder.root / "home/.config/waybar/style.css");
        EXPECT_TRUE((*fresh)[0].proposed.contains("hyprland/workspaces"));

        // A config called config in ~/waybar is kept under its own name.
        const auto mine = folder.add("home/waybar/config");
        const auto kept = desktop::planWaybarStyle(style, environment, system);
        ASSERT_TRUE(kept.has_value());
        EXPECT_EQ((*kept)[0].file, mine);
        EXPECT_EQ((*kept)[0].current, std::optional<std::string>{"{}\n"});
    }

    TEST(Desktop, MakesAValidWaybarConfigForEveryStyle) {
        for (const auto& shape : desktop::waybarShapes()) {
            for (const auto& palette : desktop::waybarPalettes()) {
                const desktop::WaybarStyle style{shape.shape, &palette};
                const auto                 config = nlohmann::json::parse(desktop::waybarConfigText(style), nullptr, false);
                ASSERT_FALSE(config.is_discarded()) << palette.name << ", " << shape.name;
                EXPECT_EQ(config["modules-left"][0], "hyprland/workspaces");
                const auto css = desktop::waybarStyleText(style);
                EXPECT_TRUE(css.contains(std::string{"@define-color accent    "} + std::string{palette.accent})) << palette.name;
                EXPECT_EQ(std::ranges::count(css, '{'), std::ranges::count(css, '}')) << palette.name << ", " << shape.name;
            }
        }
    }

    TEST(Desktop, RecognisesTheStyleWaybarUses) {
        const Folder                  folder;
        const auto                    system = folder.root / "etc";
        const foundation::Environment environment{{"HOME", (folder.root / "home").string()}};
        EXPECT_FALSE(desktop::currentWaybarStyle(environment, system).has_value());

        const desktop::WaybarStyle style{desktop::Shape::floating, &desktop::waybarPalettes()[3]};
        const auto                 planned = desktop::planWaybarStyle(style, environment, system);
        ASSERT_TRUE(planned.has_value());
        ASSERT_TRUE(configuration::applyChanges(*planned, folder.root / "backups", "test").has_value());
        EXPECT_EQ(desktop::currentWaybarStyle(environment, system), style);
    }

    TEST(Waybar, FloatsWhatTheBarOpensOnce) {
        const auto config = std::string{"hl.config({})\n"};
        const auto ruled  = desktop::withPopupRules(config);
        EXPECT_TRUE(ruled.starts_with(config));
        EXPECT_TRUE(ruled.contains("name  = \"waybar-volume\""));
        EXPECT_TRUE(ruled.contains("match = { class = \"^(org.pulseaudio.pavucontrol)$\" }"));
        EXPECT_TRUE(ruled.contains("name  = \"waybar-network\""));
        EXPECT_TRUE(ruled.contains("    float = true,\n"));
        // Rules already there are left alone.
        EXPECT_EQ(desktop::withPopupRules(ruled), ruled);
    }

    TEST(Process, StartsACommandAndSaysWhichProcessItBecame) {
        const auto started = desktop::startCommand("sleep 30");
        ASSERT_TRUE(started.has_value()) << started.error();
        // The shell has become the program, so the process is sleep itself.
        std::optional<std::string> name;
        for (int tries = 0; tries < 100 && name != "sleep"; ++tries) {
            name = desktop::processName(*started);
            ::usleep(10000);
        }
        EXPECT_EQ(name, std::optional<std::string>{"sleep"});
        EXPECT_TRUE(desktop::stopProcess(*started).has_value());
    }

}
