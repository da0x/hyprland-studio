// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "desktop/waybar-style.hpp"

#include <hyprtoolkit/element/Element.hpp>
#include <hyprtoolkit/palette/Color.hpp>

#include <string_view>

namespace ui {

    // A drawing of Waybar in a style, over a stand-in for the desktop behind it: the
    // workspaces, the clock, audio, network and the tray, in the style's colours and shape.
    // At `scale` 1 it is the size Waybar draws itself. Without `backdrop` the desktop
    // behind is left to whatever the bar is drawn on.
    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> barPreview(const desktop::WaybarStyle& style, double scale, bool backdrop = true);

    // A palette's #rrggbb as a colour.
    [[nodiscard]] Hyprtoolkit::CHyprColor paletteColour(std::string_view hex, float alpha = 1.F);

    // Its height at `scale`.
    [[nodiscard]] double barPreviewHeight(const desktop::WaybarStyle& style, double scale);

}
