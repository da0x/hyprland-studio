// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <functional>

namespace ui {

    // The bar at the top of the screen. The studio ships no bar of its own: it checks what
    // Waybar needs, and for anything missing shows the command that installs it, for the
    // user to run.
    class BarPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        BarPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // Looks again, as after the user installed something.
        void check();

      private:
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> checks;
    };

}
