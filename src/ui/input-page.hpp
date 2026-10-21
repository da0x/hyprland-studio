// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/session.hpp"
#include "configuration/file-change.hpp"
#include "desktop/input.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <expected>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ui {

    // The keyboard's layouts and the keys that switch them, what Caps Lock does, how held
    // keys repeat, and the mouse's speed and buttons. Written to the Hyprland config only
    // after the user reviews the change.
    class InputPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        InputPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // Draws the page again, after Lantern's colours or text size changed.
        void restyle();

      private:
        void showOverview();
        void showReview();
        void apply();
        void undo();
        void fillKeyboard(const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column);
        void fillMouse(const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column);
        void fillStatus();
        [[nodiscard]] std::expected<std::optional<configuration::FileChange>, std::string> plan() const;
        void reread();

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        application::EventLoop                                               loop;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> body;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement>    footer;
        RowList                                                              diff;

        // The layouts xkb knows, by name.
        std::vector<desktop::KnownLayout>                                    known;
        // What the config sets now, and what is chosen; nothing when it cannot be read.
        std::optional<desktop::InputSettings>                                current;
        desktop::InputSettings                                               wanted;
        bool                                                                 reviewing = false;
        configuration::FileChange                                            change;
        std::string                                                          outcome;
    };

}
