// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/session.hpp"
#include "application/settings.hpp"
#include "configuration/file-change.hpp"
#include "desktop/waybar-style.hpp"
#include "ui/program-switch.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ui {

    // The bar at the top of the screen. The studio ships no bar of its own: it checks what
    // Waybar needs and shows the command that installs anything missing, turns Waybar on
    // and off, and offers styles for it, written only after the user reviews the change.
    class BarPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        BarPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // Looks again, as after the user installed something.
        void check();

        // Stops watching Waybar. Called before the event loop goes away.
        void close();

      private:
        void showOverview();
        void showReview(const desktop::WaybarStyle& style);
        void apply();
        void undo();
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> styleSection();
        // A choice drawn as a preview with its name; the chosen one has the accent around it.
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> card(const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>& preview, const std::string& name,
                                                                                    bool chosen, std::function<void()> chosenNow);

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        application::EventLoop                                               loop;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> body;
        RowList                                                              diff;

        ProgramSwitch                                                        waybar;

        application::Settings                                                settings;
        // The style shown in the big preview: Waybar's own at first, if it is one of the studio's.
        desktop::WaybarStyle                                                 selected;
        // The style under review and the files it would write.
        std::optional<desktop::WaybarStyle>                                  reviewing;
        std::vector<configuration::FileChange>                               changes;
        // What happened last, shown under the style.
        std::string                                                          outcome;
    };

}
