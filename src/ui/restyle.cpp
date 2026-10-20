// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/restyle.hpp"

#include <algorithm>
#include <vector>

namespace ui {

    namespace {

        // The UI runs on one thread, so this needs no lock.
        std::vector<std::function<bool(bool)>>& registered() {
            static std::vector<std::function<bool(bool)>> list;
            return list;
        }

        // Elements come and go as pages are drawn again, so the list is pruned when it has
        // doubled since the last time.
        std::size_t prunedAt = 64;

    }

    void registerRestyle(std::function<bool(bool rebuild)> again) {
        auto& list = registered();
        list.push_back(std::move(again));
        if (list.size() < prunedAt * 2)
            return;
        std::erase_if(list, [](const auto& entry) { return !entry(false); });
        prunedAt = std::max<std::size_t>(64, list.size());
    }

    void restyleAll() {
        auto& list = registered();
        std::erase_if(list, [](const auto& entry) { return !entry(true); });
    }

}
