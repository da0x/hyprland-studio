// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/session.hpp"
#include "configuration/file-change.hpp"
#include "desktop/theme.hpp"
#include "desktop/waybar-style.hpp"
#include "ui/program-switch.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <expected>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ui {

    // One palette for the whole desktop: the windows' borders, Waybar, and the apps built on
    // hyprtoolkit, the studio among them. Written only after the user reviews the change.
    class ThemePage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        ThemePage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // Stops watching Waybar. Called before the event loop goes away.
        void close();

        // What to redraw once the studio's palette has new colours.
        void onRecoloured(std::function<void()> recoloured) { this->recoloured = std::move(recoloured); }

      private:
        void showOverview();
        void showReview();
        void apply();
        void undo();
        // After a click on a palette: changes the preview, the borders and the footer in place.
        void updateSelection();
        // Asks dunst and the open kitty windows to read their files again.
        void reloadApps();
        void fillStatus();
        // A drawing of the desktop in a palette, made once and kept: large or for a card.
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> desktopOf(const desktop::Palette* shown, bool large);
        // The same for a terminal session in a palette's terminal colours.
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> terminalOf(const desktop::Palette* shown, bool large);
        // The drawing for the part shown: the desktop's or the terminal's.
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> previewOf(const desktop::Palette* shown, bool large);
        // The palette chosen for the part shown.
        [[nodiscard]] const desktop::Palette*&                                chosen();
        // The part's file changes for the palette chosen.
        [[nodiscard]] std::expected<std::vector<configuration::FileChange>, std::string> plan();
        void                                                                   showPart(bool terminal);

        struct Card {
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement>    frame;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> holder;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>         name;
        };

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        application::EventLoop                                               loop;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> body;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement>    footer;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> largePreview;
        std::string                                                          statusShown;
        std::vector<std::pair<const desktop::Palette*, Card>>                cards;
        std::map<std::pair<const desktop::Palette*, bool>, Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>> desktops;
        std::map<std::pair<const desktop::Palette*, bool>, Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>> terminals;
        // The page has two parts, the desktop and the terminal, each with its own palette:
        // terminal programs mostly print for a dark background, whatever the desktop's.
        bool                                                                 terminalShown = false;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CButtonElement>       desktopButton;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CButtonElement>       terminalButton;
        RowList                                                              diff;
        // Waybar, to restart it after its files change. Its switch is on the Bar page.
        ProgramSwitch                                                        waybar;

        const desktop::Palette*                                              selected = nullptr;
        const desktop::Palette*                                              selectedTerminal = nullptr;
        // The palette the desktop uses now, marked on its card.
        const desktop::Palette*                                              inUse    = nullptr;
        bool                                                                 reviewing = false;
        std::vector<configuration::FileChange>                               changes;
        std::vector<desktop::GtkSetting>                                     gtk;
        std::string                                                          outcome;
        std::function<void()>                                                recoloured;
    };

}
