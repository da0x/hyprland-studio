// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/idle.hpp"

#include "configuration/hyprlang-edit.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <optional>
#include <ranges>
#include <vector>

namespace desktop {

    namespace {

        constexpr auto lockCommand   = "loginctl lock-session";
        constexpr auto screensOn     = R"(hyprctl dispatch 'hl.dsp.dpms({ action = "on" })')";
        constexpr auto screensOffRun = R"(hyprctl dispatch 'hl.dsp.dpms({ action = "off" })')";
        constexpr auto lockMark      = "# Written by Lantern for Hyprland.";

        std::filesystem::path hyprFile(const foundation::Environment& environment, std::string_view name) {
            if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"); home && std::filesystem::path{*home}.is_absolute())
                return std::filesystem::path{*home} / "hypr" / name;
            return std::filesystem::path{foundation::variable(environment, "HOME").value_or("")} / ".config" / "hypr" / name;
        }

        std::string valueOf(const configuration::HyprlangBlock& block, std::string_view key) {
            for (const auto& [name, value] : block.values)
                if (name == key)
                    return value;
            return {};
        }

        enum class Role { none, dim, lock, screensOff, suspend };

        Role roleOf(const configuration::HyprlangBlock& block) {
            if (block.name != "listener")
                return Role::none;
            const auto run = valueOf(block, "on-timeout");
            if (run.contains("brightnessctl"))
                return Role::dim;
            if (run.contains("lock-session") || run.contains("hyprlock"))
                return Role::lock;
            if (run.contains("dpms"))
                return Role::screensOff;
            if (run.contains("suspend"))
                return Role::suspend;
            return Role::none;
        }

        int minutesOf(const configuration::HyprlangBlock& block) {
            const auto text    = valueOf(block, "timeout");
            int        seconds = 0;
            std::from_chars(text.data(), text.data() + text.size(), seconds);
            return (seconds + 59) / 60;
        }

    }

    std::filesystem::path hypridleConfigFile(const foundation::Environment& environment) {
        return hyprFile(environment, "hypridle.conf");
    }

    std::filesystem::path hyprlockConfigFile(const foundation::Environment& environment) {
        return hyprFile(environment, "hyprlock.conf");
    }

    IdleSettings idleIn(std::string_view config) {
        IdleSettings settings;
        for (const auto& block : configuration::hyprlangBlocks(config)) {
            if (block.name == "general")
                settings.lockBeforeSleep = valueOf(block, "before_sleep_cmd").contains("lock");
            switch (roleOf(block)) {
                case Role::dim: settings.dim = minutesOf(block); break;
                case Role::lock: settings.lock = minutesOf(block); break;
                case Role::screensOff: settings.screensOff = minutesOf(block); break;
                case Role::suspend: settings.suspend = minutesOf(block); break;
                case Role::none: break;
            }
        }
        return settings;
    }

    std::string withIdle(std::string_view config, const IdleSettings& settings) {
        std::string text{config};
        const auto  blocks = configuration::hyprlangBlocks(config);
        // From the end backwards, so that earlier offsets stay right.
        for (const auto& block : blocks | std::views::reverse) {
            if (block.name == "general") {
                std::vector<std::pair<std::string, std::string>> wanted{{"lock_cmd", "pidof hyprlock || hyprlock"}, {"after_sleep_cmd", screensOn}};
                if (settings.lockBeforeSleep)
                    wanted.emplace_back("before_sleep_cmd", lockCommand);
                std::string added;
                for (const auto& [key, value] : wanted)
                    if (valueOf(block, key).empty())
                        added += std::format("    {} = {}\n", key, value);
                // Added at its end first: the lines taken out below come before it.
                text.insert(std::min(block.close, text.size()), added);
                if (!settings.lockBeforeSleep)
                    for (std::size_t index = block.values.size(); index-- > 0;)
                        if (block.values[index].first == "before_sleep_cmd" && block.values[index].second.contains("lock")) {
                            const auto end = text.find('\n', block.valueLines[index]);
                            text.erase(block.valueLines[index], end == std::string::npos ? std::string::npos : end + 1 - block.valueLines[index]);
                        }
                continue;
            }
            if (roleOf(block) == Role::none)
                continue;
            auto begin = block.begin;
            // The blank line that set it apart goes with it.
            if (begin >= 2 && text[begin - 1] == '\n' && text[begin - 2] == '\n')
                --begin;
            text.erase(begin, block.end - begin);
        }
        if (std::ranges::none_of(blocks, [](const auto& block) { return block.name == "general"; })) {
            std::string general = std::format("general {{\n    lock_cmd = pidof hyprlock || hyprlock\n{}    after_sleep_cmd = {}\n}}\n",
                                              settings.lockBeforeSleep ? std::format("    before_sleep_cmd = {}\n", lockCommand) : "", screensOn);
            text = general + (text.empty() ? "" : "\n") + text;
        }

        struct Listener {
            int         minutes;
            std::string body;
        };
        std::vector<Listener> listeners;
        if (settings.dim > 0)
            listeners.push_back({settings.dim, "    on-timeout = brightnessctl -s set 10\n    on-resume = brightnessctl -r\n"});
        if (settings.lock > 0)
            listeners.push_back({settings.lock, std::format("    on-timeout = {}\n", lockCommand)});
        if (settings.screensOff > 0)
            listeners.push_back({settings.screensOff, std::format("    on-timeout = {}\n    on-resume = {}\n", screensOffRun, screensOn)});
        if (settings.suspend > 0)
            listeners.push_back({settings.suspend, "    on-timeout = systemctl suspend\n"});
        std::ranges::stable_sort(listeners, {}, &Listener::minutes);
        for (const auto& listener : listeners) {
            if (!text.empty() && !text.ends_with('\n'))
                text += '\n';
            text += std::format("\nlistener {{\n    timeout = {}\n{}}}\n", listener.minutes * 60, listener.body);
        }
        return text;
    }

    std::string lockScreen(const Palette& palette) {
        const auto rgb = [](std::string_view colour) { return std::format("rgb({})", colour.starts_with('#') ? colour.substr(1) : colour); };
        return std::format(R"({} The {} theme; choosing another rewrites it.

general {{
    hide_cursor = true
}}

background {{
    monitor =
    path = screenshot
    color = {}
    blur_passes = 3
    blur_size = 8
}}

label {{
    monitor =
    text = $TIME
    color = {}
    font_size = 72
    position = 0, 120
    halign = center
    valign = center
}}

input-field {{
    monitor =
    size = 320, 52
    outline_thickness = 2
    outer_color = {}
    inner_color = {}
    font_color = {}
    check_color = {}
    fail_color = {}
    placeholder_text = Password
    fade_on_empty = false
    position = 0, -40
    halign = center
    valign = center
}}
)",
                           lockMark, palette.name, rgb(palette.base), rgb(palette.text), rgb(palette.accent), rgb(palette.surface), rgb(palette.text), rgb(palette.audio),
                           rgb(palette.alert));
    }

    bool lanternLockScreen(std::string_view config) {
        return config.starts_with(lockMark);
    }

}
