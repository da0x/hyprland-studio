// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/session.hpp"
#include "desktop/accessibility.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace ui {

    // Contrast, colour vision, text size, cursor size and motion, for the whole desktop:
    // dconf's settings, Hyprland's config, hyprtoolkit's, and every terminal theme. Written
    // only after the user reviews the change.
    class AccessibilityPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        AccessibilityPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // What to do once settings were applied or undone, given what is now in use: show
        // the terminal's themes in that version, and the studio's text at that size.
        void onApplied(std::function<void(const desktop::Accessibility&)> applied) { this->applied = std::move(applied); }

        // Draws the page again, after the studio's colours changed.
        void restyle();

        // What is in use: the desktop's settings, with the studio's own record of the
        // contrast and colour vision for the terminal laid over them.
        [[nodiscard]] static desktop::Accessibility inUse();

      private:
        void showOverview();
        void showReview();
        void apply();
        void undo();
        // After a choice: marks the cards, redraws the pictures that depend on it, and fills
        // the footer, in place.
        void updateChoices();
        void fillStatus();
        // A card's picture, made once for each key and kept, so that showing it again does
        // not flicker while its text is drawn. A key belongs to one card, since an element
        // has one parent.
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> picture(const std::string& key,
                                                                                       const std::function<Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>()>& make);

        // A choice drawn as a card: a picture of what it does, and its name.
        struct Option {
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRectangleElement>    frame  = nullptr;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> holder = nullptr;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>         name   = nullptr;
            std::string                                                          label;
            std::function<bool(const desktop::Accessibility&)>                   chosen;
            std::function<void(desktop::Accessibility&)>                         select;
            // The picture for what is wanted now.
            std::function<Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>()> draw;
        };

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        application::EventLoop                                               loop;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> body;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement>    footer;
        RowList                                                              diff;

        std::vector<Option>                                                  options;
        std::map<std::string, Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>> pictures;

        desktop::Accessibility                                               wanted;
        desktop::Accessibility                                               current;
        bool                                                                 reviewing = false;
        desktop::AccessibilityPlan                                           plan;
        std::string                                                          outcome;
        std::string                                                          statusShown;
        std::function<void(const desktop::Accessibility&)>                   applied;
    };

}
