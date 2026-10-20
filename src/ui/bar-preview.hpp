// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "desktop/waybar-style.hpp"

#include <hyprtoolkit/element/Element.hpp>

namespace ui {

    // A drawing of Waybar in a style, over a stand-in for the desktop behind it: the
    // workspaces, the clock, audio, network and the tray, in the style's colours and shape.
    // At `scale` 1 it is the size Waybar draws itself.
    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> barPreview(const desktop::WaybarStyle& style, double scale);

    // Its height at `scale`.
    [[nodiscard]] double barPreviewHeight(const desktop::WaybarStyle& style, double scale);

}
