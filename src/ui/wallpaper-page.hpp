// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "application/session.hpp"
#include "application/settings.hpp"
#include "configuration/file-change.hpp"
#include "desktop/wallpaper.hpp"
#include "ui/program-switch.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <cstddef>
#include <filesystem>
#include <functional>
#include <map>
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

        // The page is no longer shown: a wallpaper tried on the desktop and not kept is
        // replaced by the one hyprpaper.conf sets. When the studio is `closing` and the file
        // sets none, it is left: restarting hyprpaper would not finish.
        void leave(bool closing = false);

      private:
        void                                                                   showOverview();
        void                                                                   showReview();
        void                                                                   apply(bool turnOn);
        // Shows the chosen wallpaper on the desktop through hyprpaper, writing nothing.
        void                                                                   tryOnDesktop();
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
        // The desktop shows the chosen wallpaper, which hyprpaper.conf does not set yet.
        bool                                                                 tried     = false;
        std::optional<std::filesystem::path>                                 hyprctl;
        // Images already loaded, kept so that choosing again does not load them anew.
        std::map<std::filesystem::path, Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CImageElement>> thumbnails;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CImageElement>        preview;
        std::filesystem::path                                                previewImage;
        std::vector<configuration::FileChange>                               changes;
        std::string                                                          outcome;
    };

}
