// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>

#include <functional>

namespace ui {

    // hyprtoolkit takes an element's colour when it is made or rebuilt, so after the
    // studio's palette changes, every element made in the old colours is rebuilt. The
    // places that make long-lived elements register them here.

    // `again` says whether its element is still there, and when asked to, rebuilds it.
    void registerRestyle(std::function<bool(bool rebuild)> again);

    template <typename Element>
    void restyled(const Hyprutils::Memory::CSharedPointer<Element>& element) {
        registerRestyle([weak = Hyprutils::Memory::CWeakPointer<Element>{element}](bool rebuild) {
            const auto alive = weak.lock();
            if (!alive)
                return false;
            if (rebuild)
                alive->rebuild()->commence();
            return true;
        });
    }

    // Rebuilds every registered element that is still alive.
    void restyleAll();

}
