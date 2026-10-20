// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "desktop/terminal-theme.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace ui {

    // The terminal's colours, chosen apart from the desktop's from terminal themes of their
    // own, and written to kitty only after the user reviews the change.
    class TerminalPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        TerminalPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // Draws the page again, after the studio's colours changed.
        void restyle();

      private:
        void showOverview();
        void showReview();
        void apply();
        void undo();
        void updateSelection();
        void fillStatus();
        // The themes shown: the dark ones or the light ones.
        [[nodiscard]] std::vector<const desktop::TerminalTheme*>               shownThemes() const;
        // A short terminal session in a theme, made once and kept: large or for a card.
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> sessionOf(const desktop::TerminalTheme* theme, bool large);

        struct Card {
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement>    frame;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> holder;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>         name;
        };

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> body;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement>    footer;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> largePreview;
        std::vector<std::pair<const desktop::TerminalTheme*, Card>>          cards;
        std::map<std::pair<const desktop::TerminalTheme*, bool>, Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>> sessions;
        RowList                                                              diff;

        const desktop::TerminalTheme*                                        selected = nullptr;
        const desktop::TerminalTheme*                                        inUse    = nullptr;
        bool                                                                 lightShown = false;
        std::size_t                                                          pageStart  = 0;
        bool                                                                 reviewing  = false;
        std::vector<configuration::FileChange>                               changes;
        std::string                                                          outcome;
        std::string                                                          statusShown;
    };

}
