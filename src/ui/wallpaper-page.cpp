// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/wallpaper-page.hpp"

#include "application/studio-folder.hpp"
#include "configuration/main-config.hpp"
#include "desktop/clipboard.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"
#include "ui/widgets.hpp"

#include <hyprtoolkit/element/Image.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <sstream>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr std::size_t perRow       = 4;
        constexpr double      cardShare    = 0.24;
        constexpr double      thumbHeight  = 110;
        constexpr double      previewWidth = 480;
        // A 16:9 screen.
        constexpr double      previewHeight = previewWidth * 9 / 16;
        constexpr auto        changeName    = "Wallpaper";

        Hyprtoolkit::eImageFitMode imageFit(desktop::FitMode fit) {
            switch (fit) {
                case desktop::FitMode::cover: return Hyprtoolkit::IMAGE_FIT_MODE_COVER;
                case desktop::FitMode::contain: return Hyprtoolkit::IMAGE_FIT_MODE_CONTAIN;
                case desktop::FitMode::fill: return Hyprtoolkit::IMAGE_FIT_MODE_STRETCH;
                case desktop::FitMode::tile: return Hyprtoolkit::IMAGE_FIT_MODE_TILE;
            }
            return Hyprtoolkit::IMAGE_FIT_MODE_COVER;
        }

        std::string fitName(desktop::FitMode fit) {
            for (const auto& info : desktop::fitModes())
                if (info.mode == fit)
                    return std::string{info.name};
            return {};
        }

        // Whether the config Hyprland reads starts hyprpaper at login, judged by whether it
        // names it at all.
        std::optional<configuration::MainConfig> configWithoutHyprpaper() {
            const auto config = configuration::mainConfig(configuration::studioLaunch());
            if (!config)
                return std::nullopt;
            std::ifstream     stream{config->file};
            std::stringstream text;
            text << stream.rdbuf();
            if (text.str().contains("hyprpaper"))
                return std::nullopt;
            return *config;
        }

        const std::array systemFolders{std::filesystem::path{"/usr/share/hypr"}, std::filesystem::path{"/usr/share/backgrounds"}};

    }

    WallpaperPage::WallpaperPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop) :
        palette{std::move(palette)},
        defer{std::move(defer)},
        loop{std::move(loop)},
        diff{this->palette, this->defer,
             {RowList::Column{.width = 24, .monospace = true, .secondary = false, .markup = true, .heading = ""},
              RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true, .heading = ""}},
             20},
        hyprpaper{this->palette, this->defer, this->loop, "hyprpaper", "hyprpaper", nullptr} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Wallpaper", this->palette));
        root->addChild(paragraph("The studio draws no wallpaper of its own. It uses hyprpaper, Hyprland's own wallpaper program, and shows the images it finds "
                                 "for you to choose from. It never installs anything: when something is missing, it shows the command for you to run.",
                                 this->palette));
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(hyprpaper.element());
        root->addChild(body);

        settings = application::loadSettings(application::studioFolder(foundation::currentEnvironment()));
        check();
    }

    void WallpaperPage::close() {
        hyprpaper.close();
    }

    void WallpaperPage::check() {
        const auto environment = foundation::currentEnvironment();
        hyprpaper.refresh();
        wallpapers = desktop::findWallpapers(environment, systemFolders);
        const auto current = desktop::currentWallpaper(environment);
        if (current && !std::ranges::contains(wallpapers, current->image))
            wallpapers.insert(wallpapers.begin(), current->image);
        if (!selected)
            selected = current ? current : wallpapers.empty() ? std::nullopt : std::optional{desktop::Wallpaper{wallpapers.front(), desktop::FitMode::cover}};
        pageStart = std::min(pageStart, wallpapers.empty() ? 0 : wallpapers.size() - 1);
        if (!reviewing)
            showOverview();
    }

    CSharedPointer<Hyprtoolkit::IElement> WallpaperPage::chooser() {
        const auto environment = foundation::currentEnvironment();
        const auto whole       = leftRow(previewHeight);
        whole->rebuild()->gap(20)->commence();

        // The chosen image as the screen would show it.
        const auto frame = Hyprtoolkit::CRectangleBuilder::begin()
                               ->color([] { return Hyprtoolkit::CHyprColor{0, 0, 0, 1}; })
                               ->rounding(palette->m_vars.smallRounding)
                               ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {previewWidth, previewHeight}})
                               ->commence();
        frame->addChild(Hyprtoolkit::CImageBuilder::begin()
                            ->path(selected->image.string())
                            ->fitMode(imageFit(selected->fit))
                            ->rounding(palette->m_vars.smallRounding)
                            ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})
                            ->commence());
        whole->addChild(frame);

        const auto side = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        side->setGrow(true, false);
        side->addChild(markupLine(std::format("<span weight=\"600\">{}</span>", escapeMarkup(selected->image.filename().string())), palette));
        side->addChild(paragraph(shortPath(selected->image.parent_path()), palette, true));
        side->addChild(space(6));
        side->addChild(markupLine("<span weight=\"600\">Fit</span>", palette));
        const auto fits = leftRow();
        fits->rebuild()->gap(4)->commence();
        for (const auto& info : desktop::fitModes())
            fits->addChild(button(std::string{info.name}, 84, info.mode == selected->fit, [this, mode = info.mode] {
                defer([this, mode] {
                    selected->fit = mode;
                    showOverview();
                });
            }));
        side->addChild(fits);
        side->addChild(space(6));

        const auto actions = leftRow();
        const auto planned = desktop::planWallpaper(*selected, environment);
        if (!planned)
            side->addChild(paragraph(std::format("The studio cannot write it: {}.", planned.error()), palette));
        else if (!planned->changes())
            actions->addChild(markupLine(std::format("{}  hyprpaper.conf sets this wallpaper.", coloured("✓", hex(palette->m_colors.accent))), palette));
        else
            actions->addChild(button("Review changes", 160, true, [this] { defer([this] { showReview(); }); }));
        if (configuration::latestChange(application::backupsFolder(environment), changeName))
            actions->addChild(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        side->addChild(actions);
        if (!outcome.empty())
            side->addChild(paragraph(outcome, palette));

        if (planned && planned->current) {
            if (const auto own = desktop::monitorsWithOwnWallpaper(*planned->current); !own.empty()) {
                std::string names;
                for (const auto& monitor : own)
                    names += (names.empty() ? "" : ", ") + monitor;
                side->addChild(paragraph(std::format("hyprpaper.conf gives {} a wallpaper of its own, which this does not change.", names), palette, true));
            }
        }
        whole->addChild(side);
        return whole;
    }

    CSharedPointer<Hyprtoolkit::IElement> WallpaperPage::thumbnail(const std::filesystem::path& image) {
        const bool chosen = selected && selected->image == image;
        const auto card   = Hyprtoolkit::CRectangleBuilder::begin()
                              ->color([palette = palette] { return palette->m_colors.alternateBase; })
                              ->rounding(palette->m_vars.bigRounding)
                              ->borderColor([palette = palette, chosen] { return chosen ? palette->m_colors.accent : Hyprtoolkit::CHyprColor{0, 0, 0, 0}; })
                              ->borderThickness(2)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {cardShare, 1.0}})
                              ->commence();
        const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        inside->setMargin(8);
        inside->addChild(Hyprtoolkit::CImageBuilder::begin()
                             ->path(image.string())
                             ->fitMode(Hyprtoolkit::IMAGE_FIT_MODE_COVER)
                             ->rounding(palette->m_vars.smallRounding)
                             ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, thumbHeight}})
                             ->commence());
        inside->addChild(markupLine(escapeMarkup(image.filename().string()), palette, CFontSize{CFontSize::HT_FONT_SMALL}, chosen ? 1.F : 0.7F));
        card->addChild(inside);
        card->setReceivesMouse(true);
        card->setMouseButton([this, image](Hyprtoolkit::Input::eMouseButton button, bool pressed) {
            if (button != Hyprtoolkit::Input::MOUSE_BUTTON_LEFT || !pressed)
                return;
            defer([this, image] {
                selected = desktop::Wallpaper{image, selected ? selected->fit : desktop::FitMode::cover};
                outcome.clear();
                showOverview();
            });
        });
        return card;
    }

    CSharedPointer<Hyprtoolkit::IElement> WallpaperPage::grid() {
        const auto section = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        const auto top     = leftRow();
        const auto title   = markupLine(std::format("<span weight=\"600\">Wallpapers</span>   {}–{} of {}", pageStart + 1, std::min(wallpapers.size(), pageStart + pageSize), wallpapers.size()),
                                        palette);
        title->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        title->setGrow(true, false);
        top->addChild(title);
        if (pageStart > 0)
            top->addChild(button("‹ Previous", 110, false, [this] { defer([this] { pageStart -= std::min(pageStart, pageSize); showOverview(); }); }));
        if (pageStart + pageSize < wallpapers.size())
            top->addChild(button("Next ›", 110, false, [this] { defer([this] { pageStart += pageSize; showOverview(); }); }));
        section->addChild(top);

        for (std::size_t first = pageStart; first < std::min(wallpapers.size(), pageStart + pageSize); first += perRow) {
            const auto line = leftRow(0);
            line->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
            for (std::size_t index = first; index < std::min({wallpapers.size(), first + perRow, pageStart + pageSize}); ++index)
                line->addChild(thumbnail(wallpapers[index]));
            section->addChild(line);
        }
        return section;
    }

    void WallpaperPage::showOverview() {
        reviewing = false;
        changes.clear();
        body->clearChildren();

        const auto environment  = foundation::currentEnvironment();
        const auto distribution = desktop::currentDistribution();
        const auto copier       = desktop::clipboardProgram(foundation::variable(environment, "PATH").value_or(""));
        const auto requirements = desktop::wallpaperRequirements(environment);
        const bool allMet       = std::ranges::all_of(requirements, [](const auto& requirement) { return requirement.met(); });

        const bool checklist = !allMet || !settings.wallpaperRequirementsDismissed;
        pageSize             = checklist ? perRow : 2 * perRow;
        pageStart            = pageStart / pageSize * pageSize;
        if (checklist) {
            for (const auto& requirement : requirements)
                body->addChild(requirementBlock(requirement, distribution, copier, palette, defer));
            const auto actions = leftRow();
            actions->addChild(button("Check again", 140, false, [this] { defer([this] { check(); }); }));
            if (allMet)
                actions->addChild(button("Dismiss", 110, false, [this] {
                    defer([this] {
                        settings.wallpaperRequirementsDismissed = true;
                        if (const auto saved = application::saveSettings(settings, application::studioFolder(foundation::currentEnvironment())); !saved)
                            outcome = std::format("The studio could not save its settings: {}.", saved.error());
                        showOverview();
                    });
                }));
            body->addChild(actions);
            body->addChild(space(6));
        }

        // hyprpaper runs only if something starts it; the studio adds nothing to start at
        // login on its own, so it shows the line to add.
        if (allMet) {
            if (const auto config = configWithoutHyprpaper()) {
                body->addChild(paragraph(std::format("Nothing starts hyprpaper when you log in. To start it, add this line to {}:", shortPath(config->file)), palette));
                body->addChild(commandRow(config->lua() ? "hl.exec_cmd(\"hyprpaper\")" : "exec-once = hyprpaper", copier, palette, defer));
                body->addChild(space(6));
            }
        }

        if (!selected) {
            body->addChild(paragraph("No images were found in ~/Pictures/Wallpapers, ~/Pictures, ~/.local/share/backgrounds, /usr/share/hypr or /usr/share/backgrounds. "
                                     "Put some in ~/Pictures/Wallpapers and press Check again.",
                                     palette));
            return;
        }
        body->addChild(chooser());
        body->addChild(space(6));
        body->addChild(grid());
    }

    void WallpaperPage::showReview() {
        auto planned = desktop::planWallpaper(*selected, foundation::currentEnvironment());
        if (!planned) {
            outcome = planned.error();
            return showOverview();
        }
        reviewing = true;
        changes   = {std::move(*planned)};
        outcome.clear();
        body->clearChildren();

        body->addChild(space(4));
        body->addChild(heading(std::format("Review: {}, {}", selected->image.filename().string(), fitName(selected->fit)), palette));
        body->addChild(paragraph(std::format("Applying writes this file. A copy of it is kept first, so it can be undone. {}",
                                             hyprpaper.running() ? "hyprpaper then restarts to read it." : "hyprpaper is off, so it will not be restarted."),
                                 palette, true));
        const auto actions = leftRow();
        actions->addChild(button("Apply", 110, true, [this] { defer([this] { apply(); }); }));
        actions->addChild(button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }));
        body->addChild(actions);
        diff.setRows(diffRows(changes, *palette));
        body->addChild(diff.element());
    }

    void WallpaperPage::apply() {
        if (!reviewing)
            return;
        const auto record = configuration::applyChanges(changes, application::backupsFolder(foundation::currentEnvironment()),
                                                        std::format("{}: {}, {}", changeName, selected->image.string(), fitName(selected->fit)));
        if (!record) {
            outcome = std::format("Nothing was written: {}.", record.error());
        } else {
            outcome = "Written. hyprpaper is restarting…";
            hyprpaper.restartToRead(*record, [this](std::string what) {
                outcome = std::move(what);
                if (!reviewing)
                    showOverview();
            });
        }
        showOverview();
    }

    void WallpaperPage::undo() {
        const auto latest = configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), changeName);
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone) {
            outcome = std::format("Nothing was undone: {}.", undone.error());
        } else {
            selected = desktop::currentWallpaper(foundation::currentEnvironment()).value_or(*selected);
            if (hyprpaper.running()) {
                outcome = "Undone, and hyprpaper is restarting to read the file as it was.";
                hyprpaper.restart();
            } else {
                outcome = "Undone.";
            }
        }
        showOverview();
    }

}
