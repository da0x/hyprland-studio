// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/session.hpp"
#include "application/settings.hpp"
#include "configuration/file-change.hpp"
#include "desktop/wallpaper.hpp"
#include "ui/program-switch.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ui {

    // The wallpaper. The studio draws none of its own: it uses hyprpaper, Hyprland's own
    // wallpaper program, shows the images it finds to choose from, and writes the choice
    // to hyprpaper.conf only after the user reviews the change.
    class WallpaperPage {
      public:
        // `defer` runs a function after the current layout pass; see RowList.
        WallpaperPage(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        // Looks again, as after the user installed something or added images.
        void check();

        // Stops watching hyprpaper. Called before the event loop goes away.
        void close();

      private:
        void                                                                   showOverview();
        void                                                                   showReview();
        void                                                                   apply();
        void                                                                   undo();
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> chooser();
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> grid();
        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> thumbnail(const std::filesystem::path& image);

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>             palette;
        std::function<void(std::function<void()>)>                           defer;
        application::EventLoop                                               loop;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> body;
        RowList                                                              diff;
        ProgramSwitch                                                        hyprpaper;
        application::Settings                                                settings;

        std::vector<std::filesystem::path>                                   wallpapers;
        // The first image on the grid's page, and how many a page shows: one row while the
        // checklist takes room, two after.
        std::size_t                                                          pageStart = 0;
        std::size_t                                                          pageSize  = 8;
        std::optional<desktop::Wallpaper>                                    selected;
        bool                                                                 reviewing = false;
        std::vector<configuration::FileChange>                               changes;
        std::string                                                          outcome;
    };

}
