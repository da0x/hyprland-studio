// SPDX-License-Identifier: GPL-3.0-or-later
#include "application/keybindings.hpp"

#include <algorithm>
#include <cctype>
#include <format>

namespace application {

    namespace {

        bool sameKey(std::string_view left, std::string_view right) {
            return std::ranges::equal(left, right, [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); });
        }

        bool sameKeys(const ipc::Binding& live, const configuration::Binding& recorded) {
            return live.modifiers == recorded.modifiers && sameKey(live.key, recorded.key) && live.submap == recorded.submap;
        }

    }

    Keybindings joinKeybindings(std::vector<ipc::Binding> live, const std::expected<configuration::Recording, std::string>& recording) {
        Keybindings result;

        std::vector<configuration::Binding> recorded;
        if (!recording) {
            result.configurationProblem = recording.error();
        } else {
            auto read = configuration::bindings(*recording);
            recorded  = std::move(read.bindings);
            if (recording->failure)
                result.configurationProblem = std::format("reading the config stopped at {}: {}",
                                                          recording->failure->stack.empty() ? std::string{"its start"} : recording->failure->stack.front().brief(),
                                                          recording->failure->message);
            else if (!read.unreadable.empty())
                result.configurationProblem = std::format("{} binding(s) could not be read, the first at {}", read.unreadable.size(), read.unreadable.front());
        }

        std::vector<bool> taken(recorded.size(), false);
        for (auto& binding : live) {
            KeybindingRow row{.live = std::move(binding), .recorded = std::nullopt};
            for (std::size_t index = 0; index < recorded.size(); ++index) {
                if (taken[index] || !sameKeys(row.live, recorded[index]))
                    continue;
                taken[index] = true;
                row.recorded = recorded[index];
                break;
            }
            result.rows.push_back(std::move(row));
        }

        for (std::size_t index = 0; index < recorded.size(); ++index)
            if (!taken[index])
                result.inactive.push_back(std::move(recorded[index]));
        return result;
    }

}
