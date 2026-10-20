// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/session.hpp"
#include "configuration/file-change.hpp"
#include "desktop/window-look.hpp"
#include "ui/choice-cards.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ui {

    // How windows look and are handled: corners, gaps, border, and resizing by the border.
    // Written to the Hyprland config only after the user reviews the change.
    class WindowsPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        WindowsPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // Draws the page again, after the studio's colours or text size changed.
        void restyle();

      private:
        void showOverview();
        void showReview();
        void apply();
        void undo();
        // After a choice: the picture of the desktop and the footer.
        void fillPreview();
        void fillStatus();

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        application::EventLoop                                               loop;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> body;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement>    footer;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> preview;
        ChoiceCards                                                          cards;
        RowList                                                              diff;

        // What the config sets now, and what is chosen; nothing when it cannot be read.
        std::optional<desktop::WindowLook>                                   current;
        desktop::WindowLook                                                  wanted;
        bool                                                                 reviewing = false;
        std::vector<configuration::FileChange>                               changes;
        std::string                                                          outcome;
        std::string                                                          statusShown;
    };

}
