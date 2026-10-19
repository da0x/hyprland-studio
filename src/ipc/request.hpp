// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ipc/instance.hpp"

#include <expected>
#include <string>

namespace ipc {

    // Every request the studio may send outside the activation layer. Each one only reads,
    // and nothing else can be expressed here: a request that changes Hyprland belongs to
    // the activation layer and does not go through this function.
    enum class Query {
        bindings,
    };

    // Sends one query and waits for the whole JSON reply. Blocks, so never call it on the
    // UI thread.
    [[nodiscard]] std::expected<std::string, std::string> request(const Instance& instance, Query query);

}
