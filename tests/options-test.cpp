// SPDX-License-Identifier: GPL-3.0-or-later
#include "application/options.hpp"
#include "ipc/options.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <fstream>

#include <unistd.h>

TEST(Options, ReadsHyprlandsDescriptions) {
    const auto described = ipc::parseOptionDescriptions(R"([
        {"name": "general:border_size", "description": "size of the border", "default": 1, "current": 1, "min": 0, "max": 20, "map": null},
        {"name": "general:gaps_in", "description": "gaps between windows", "default": "5 5 5 5", "current": "5 5 5 5", "min": null, "max": null},
        {"name": "decoration:dim_strength", "description": "how much", "default": 0.5, "current": 0},
        {"name": "input:follow_mouse", "description": "focus", "default": 1, "current": 1, "min": 0, "max": 3,
         "map": [{"separate": 3}, {"detached": 2}, {"follow": 1}, {"disabled": 0}]},
        {"name": "decoration:blur:enabled", "description": "blur", "default": true, "current": false}
    ])");
    ASSERT_TRUE(described) << described.error();
    ASSERT_EQ(described->size(), 5U);
    EXPECT_EQ((*described)[0].defaultValue, "1");
    EXPECT_EQ((*described)[0].maximum, 20);
    EXPECT_EQ((*described)[1].defaultValue, "5 5 5 5");
    EXPECT_FALSE((*described)[1].minimum);
    EXPECT_EQ((*described)[2].defaultValue, "0.5");
    ASSERT_EQ((*described)[3].choices.size(), 4U);
    EXPECT_EQ((*described)[3].choices.front(), (std::pair<std::string, long>{"disabled", 0}));
    EXPECT_EQ((*described)[4].defaultValue, "true");
    EXPECT_FALSE(ipc::parseOptionDescriptions("not json"));
}

TEST(Options, ReadsABatchOfValuesInOrderEvenPastAReplyThatIsNotJson) {
    const std::vector<std::string> names{"general:gaps_in", "misc:broken", "decoration:active_opacity", "decoration:shadow:offset", "input:kb_layout"};
    const auto                     values = ipc::parseOptionValues("{\"option\": \"general:gaps_in\", \"css\": \"5 5 5 5\", \"set\": true }\n\n\n"
                                                                   "invalid type (internal error)\n\n\n"
                                                                   "{\"option\": \"decoration:active_opacity\", \"float\": 1.000000, \"set\": false }\n\n\n"
                                                                   "{\"option\": \"decoration:shadow:offset\", \"vec2\": [0, 0], \"set\": false }\n\n\n"
                                                                   "{\"option\": \"input:kb_layout\", \"str\": \"us,ara\", \"set\": true }",
                                                                   names);
    ASSERT_EQ(values.size(), 5U);
    EXPECT_EQ(values[0], (ipc::OptionValue{"general:gaps_in", "5 5 5 5", true}));
    EXPECT_EQ(values[1], (ipc::OptionValue{"misc:broken", "", false}));
    EXPECT_EQ(values[2], (ipc::OptionValue{"decoration:active_opacity", "1", false}));
    EXPECT_EQ(values[3].value, "0 0");
    EXPECT_EQ(values[4], (ipc::OptionValue{"input:kb_layout", "us,ara", true}));
    EXPECT_EQ(ipc::numberText(0.2), "0.2");
}

TEST(Options, FindsTheCallThatSetEachOptionAndWhatDiffersFromTheDefault) {
    static std::atomic<int> counter = 0;
    const auto folder = std::filesystem::temp_directory_path() / ("hyprland-studio-options-" + std::to_string(::getpid()) + "-" + std::to_string(counter++));
    std::filesystem::create_directories(folder);
    std::ofstream{folder / "hyprland.lua"} << "hl.config({\n"
                                              "    general = { gaps_in = 5, col = { active_border = { colors = { \"rgba(fabd2fee)\" }, angle = 45 } } },\n"
                                              "})\n"
                                              "hl.config({ decoration = { blur = { enabled = false } } })\n";
    const auto recording = configuration::record(folder / "hyprland.lua");
    std::filesystem::remove_all(folder);
    ASSERT_TRUE(recording) << recording.error();

    std::vector<ipc::OptionDescription> described;
    for (const auto* name : {"general:gaps_in", "general:col.active_border", "decoration:blur:enabled", "decoration:blur:size"})
        described.push_back(ipc::OptionDescription{.name = name, .description = {}, .defaultValue = "true", .minimum = {}, .maximum = {}, .choices = {}});
    described[0].defaultValue = "5 5 5 5";
    const auto options = application::joinOptions(described,
                                                   {{"general:gaps_in", "5 5 5 5", true},
                                                    {"general:col.active_border", "eefabd2f 45deg", true},
                                                    {"decoration:blur:enabled", "false", true},
                                                    {"decoration:blur:size", "8", false}},
                                                   recording);
    ASSERT_EQ(options.rows.size(), 4U);
    ASSERT_TRUE(options.rows[0].where);
    EXPECT_EQ(options.rows[0].where->line, 1);
    EXPECT_FALSE(options.rows[0].changed());
    ASSERT_TRUE(options.rows[1].where);
    EXPECT_TRUE(options.rows[1].changed());
    ASSERT_TRUE(options.rows[2].where);
    EXPECT_EQ(options.rows[2].where->line, 4);
    EXPECT_TRUE(options.rows[2].changed());
    EXPECT_FALSE(options.rows[3].where);
    EXPECT_FALSE(options.rows[3].changed());
}
