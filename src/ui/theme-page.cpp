// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/theme-page.hpp"

#include "application/config-check.hpp"
#include "application/lantern-folder.hpp"
#include "configuration/main-config.hpp"
#include "desktop/process.hpp"
#include "desktop/system.hpp"
#include "desktop/theme.hpp"
#include "desktop/window-look.hpp"
#include "ui/bar-preview.hpp"
#include "ui/desktop-preview.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"
#include "ui/widgets.hpp"

#include <hyprtoolkit/element/Null.hpp>

#include <algorithm>
#include <csignal>
#include <fstream>
#include <format>
#include <numbers>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr auto   changeName = "Theme";
        constexpr double cardShare  = 0.32;

    }

    ThemePage::ThemePage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop) :
        palette{std::move(palette)},
        defer{std::move(defer)},
        loop{std::move(loop)},
        diff{this->palette, this->defer,
             {RowList::Column{.width = 24, .monospace = true, .secondary = false, .markup = true, .heading = ""},
              RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true, .heading = ""}},
             20},
        waybar{this->palette, this->defer, this->loop, "waybar", "Waybar", nullptr} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Theme", this->palette));
        root->addChild(paragraph("One palette for the desktop: the borders of your windows, Waybar, the launcher, notifications, GTK apps and the apps built "
                                 "on hyprtoolkit, Lantern among them. The terminal has themes of its own, on the Terminal page. Nothing is written until "
                                 "you review the change.",
                                 this->palette));
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        // A page laid out taller than the window comes out wrong, so the large preview takes
        // only the height the cards leave, measured after each layout pass.
        body->setRepositioned([this] { this->defer([this] { fitPreview(); }); });
        root->addChild(body);
        footer = leftRow();
        root->addChild(footer);

        const auto config = configuration::mainConfig(configuration::lanternLaunch());
        selected          = config ? desktop::currentTheme(*config) : nullptr;
        if (selected == nullptr)
            if (const auto style = desktop::currentBarStyle(foundation::currentEnvironment(), desktop::runningBarGeometry()))
                selected = style->palette;
        if (selected == nullptr)
            selected = &desktop::waybarPalettes().front();
        showOverview();
    }

    void ThemePage::close() {
        waybar.close();
    }

    CSharedPointer<Hyprtoolkit::IElement> ThemePage::desktopOf(const desktop::Palette* shown, bool large) {
        auto& made = desktops[{shown, large}];
        if (made)
            return made;
        // The large one is drawn smaller when the page has less height to give it.
        const double scale  = large ? previewHeight / 250 : 0.42;
        const double height = large ? previewHeight : 105;
        // The windows as the config sets them, so this and the Windows page show the same desktop.
        const auto bar    = desktop::currentBarStyle(foundation::currentEnvironment(), desktop::runningBarGeometry()).value_or(desktop::WaybarStyle{desktop::Shape::islands, shown});
        const auto config = configuration::mainConfig(configuration::lanternLaunch());
        const auto look   = (config ? desktop::currentWindowLook(*config) : std::nullopt).value_or(desktop::WindowLook{});
        made              = desktopPreview(*shown, look, bar, scale, height);
        return made;
    }

    void ThemePage::showOverview() {
        reviewing = false;
        changes.clear();
        body->clearChildren();

        largePreview = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        statusShown.clear();
        body->addChild(largePreview);

        cards.clear();
        cardRows.clear();
        const auto all = desktop::waybarPalettes();
        for (std::size_t first = 0; first < all.size(); first += 3) {
            const auto line = leftRow(0);
            line->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
            for (std::size_t index = first; index < std::min(all.size(), first + 3); ++index) {
                Card made;
                made.frame = Hyprtoolkit::CRectangleBuilder::begin()
                                 ->color([palette = palette] { return palette->m_colors.alternateBase; })
                                 ->rounding(palette->m_vars.bigRounding)
                                 ->borderColor([] { return Hyprtoolkit::CHyprColor{0, 0, 0, 0}; })
                                 ->borderThickness(2)
                                 ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {cardShare, 1.0}})
                                 ->commence();
                const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
                inside->setMargin(10);
                made.holder = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
                made.holder->addChild(desktopOf(&all[index], false));
                made.name = markupLine(escapeMarkup(all[index].name), palette);
                inside->addChild(made.holder);
                inside->addChild(made.name);
                made.frame->addChild(inside);
                made.frame->setReceivesMouse(true);
                made.frame->setMouseButton([this, chosen = &all[index]](Hyprtoolkit::Input::eMouseButton button, bool pressed) {
                    if (button != Hyprtoolkit::Input::MOUSE_BUTTON_LEFT || !pressed)
                        return;
                    defer([this, chosen] {
                        selected = chosen;
                        updateSelection();
                    });
                });
                line->addChild(made.frame);
                cards.emplace_back(&all[index], std::move(made));
            }
            body->addChild(line);
            cardRows.push_back(line);
        }
        updateSelection();
    }

    void ThemePage::updateSelection() {
        largePreview->clearChildren();
        if (previewHeight > 0)
            largePreview->addChild(desktopOf(selected, true));
        fillStatus();
        for (auto& [shown, made] : cards) {
            const bool chosen = shown == selected;
            made.frame->rebuild()->borderColor([palette = palette, chosen] { return chosen ? palette->m_colors.accent : Hyprtoolkit::CHyprColor{0, 0, 0, 0}; })->commence();
            auto name = escapeMarkup(shown->name);
            if (chosen)
                name = std::format("<span weight=\"600\">{}</span>", name);
            if (shown == inUse)
                name += std::format("   {}", coloured("· in use", hex(palette->m_colors.accent)));
            made.name->rebuild()->text(std::string{name})->commence();
        }
    }

    void ThemePage::fillStatus() {
        const auto environment = foundation::currentEnvironment();
        const auto config      = configuration::mainConfig(configuration::lanternLaunch());
        const auto bar         = desktop::currentBarStyle(environment, desktop::runningBarGeometry()).value_or(desktop::WaybarStyle{desktop::Shape::islands, selected});
        const auto planned     = config ? desktop::planTheme(*selected, bar, *config, environment)
                                        : std::expected<std::vector<configuration::FileChange>, std::string>{std::unexpect, config.error()};
        const bool undoable    = configuration::latestChange(application::backupsFolder(environment), changeName).has_value();
        const bool gtkSame     = desktop::gtkChanges(*selected, environment).empty();
        inUse                  = config ? desktop::currentTheme(*config) : nullptr;
        const auto shown = std::format("{}|{}|{}|{}|{}|{}", selected->name, planned ? planned->empty() : false, gtkSame, planned ? "" : planned.error(), undoable, outcome);
        if (shown == statusShown)
            return;
        statusShown = shown;

        // Everything said here sits in the footer, so that saying it moves nothing above.
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> buttons;
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> away;
        if (undoable)
            away.push_back(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        std::string note;
        if (!planned) {
            note = escapeMarkup(std::format("Lantern cannot apply it: {}.", planned.error()));
        } else if (planned->empty() && gtkSame) {
            note = std::format("{}  Your desktop uses {}.", coloured("✓", hex(palette->m_colors.accent)), escapeMarkup(selected->name));
        } else {
            buttons.push_back(button("Review changes", 160, true, [this] { defer([this] { showReview(); }); }));
            note = escapeMarkup(outcome);
        }
        CSharedPointer<Hyprtoolkit::IElement> noteText;
        if (!note.empty()) {
            const auto made = markupLine(note, palette);
            made->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
            noteText = made;
        }
        setFooter(footer, buttons, noteText, away);
    }

    void ThemePage::showReview() {
        const auto environment = foundation::currentEnvironment();
        const auto config      = configuration::mainConfig(configuration::lanternLaunch());
        const auto bar         = desktop::currentBarStyle(environment, desktop::runningBarGeometry()).value_or(desktop::WaybarStyle{desktop::Shape::islands, selected});
        auto       planned     = config ? desktop::planTheme(*selected, bar, *config, environment)
                                        : std::expected<std::vector<configuration::FileChange>, std::string>{std::unexpect, config.error()};
        if (!planned) {
            outcome = planned.error();
            return showOverview();
        }
        reviewing = true;
        changes   = std::move(*planned);
        outcome.clear();
        body->clearChildren();
        body->addChild(space(4));
        body->addChild(heading(std::format("Review: {}", selected->name), palette));
        gtk = desktop::gtkChanges(*selected, environment);
        body->addChild(paragraph("Applying will:", palette));
        std::vector<std::string> steps;
        if (!changes.empty())
            steps.emplace_back("Write the changes below. A copy of each file is kept first, so it can be undone.");
        for (const auto& setting : gtk)
            steps.push_back(std::format("Set GTK's {} from {} to {}, for GTK apps.", setting.key, setting.current, setting.wanted));
        steps.emplace_back("Show the new colours at once: Hyprland's borders, Lantern, Waybar, the launcher and dunst's notifications.");
        if (std::ranges::any_of(changes, [](const auto& change) { return change.file.filename() == "gtk.css"; }))
            steps.emplace_back("Give GTK apps, such as the volume control and the network settings, the palette too: every one opened from now on, "
                               "and the rest once they are opened again.");
        for (const auto& step : steps)
            body->addChild(paragraph(std::format("•  {}", step), palette, true));
        setFooter(footer, {button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }), button("Apply", 110, true, [this] { defer([this] { apply(); }); })});
        if (!changes.empty()) {
            diff.setRows(diffRows(changes, *palette));
            body->addChild(diff.element());
        }
    }

    void ThemePage::apply() {
        if (!reviewing)
            return;
        const auto environment = foundation::currentEnvironment();
        const auto record      = configuration::applyChanges(changes, application::backupsFolder(environment), std::format("{}: {}", changeName, selected->name));
        if (!record) {
            outcome = std::format("Nothing was written: {}.", record.error());
            return showOverview();
        }
        // GTK's settings live in dconf, not in a file: their old values go beside the change's
        // record, for Undo, before they are set.
        desktop::setGsettings(gtk, *record, environment);
        reloadApps();

        const auto changed = [this](auto test) { return std::ranges::any_of(changes, test); };
        const bool toolkit = changed([](const auto& change) { return change.file.filename() == "hyprtoolkit.conf"; });
        if (toolkit) {
            // Lantern's own colours, set before hyprtoolkit sees the file change and
            // redraws every element with them.
            auto& colours          = palette->m_colors;
            colours.background     = paletteColour(selected->base);
            colours.base           = paletteColour(selected->surface);
            colours.alternateBase  = paletteColour(selected->raised);
            colours.text           = paletteColour(selected->text);
            colours.brightText     = paletteColour(selected->text);
            colours.linkText       = paletteColour(selected->audio);
            colours.accent         = paletteColour(selected->accent);
            colours.accentSecondary = paletteColour(selected->network);
            // Every element is made again in the new colours.
            if (recoloured)
                defer(recoloured);
        }
        const bool bar     = changed([](const auto& change) { return change.file.parent_path().filename() == "waybar"; });
        const bool config  = changed([](const auto& change) { return change.file.extension() == ".lua"; });
        const auto hyprctl = desktop::findProgram("hyprctl", foundation::variable(environment, "PATH").value_or(""));

        // Waybar comes back and Hyprland checks the config while Lantern keeps running;
        // a problem with either is reported here.
        auto waiting  = std::make_shared<int>((bar ? 1 : 0) + (config && hyprctl ? 1 : 0));
        auto problem  = std::make_shared<std::optional<std::string>>();
        auto finished = [this, waiting, problem] {
            if (--*waiting > 0)
                return;
            if (*problem) {
                outcome = **problem;
                if (!reviewing)
                    showOverview();
            }
        };
        outcome = std::format("Applied {}.", selected->name);
        if (bar)
            waybar.restartToRead(*record, [problem, finished](std::string what) {
                if (what.contains("put back"))
                    *problem = what;
                finished();
            });
        if (config && hyprctl)
            application::checkHyprlandConfig(loop, *hyprctl, *record, [problem, finished](std::optional<std::string> what) {
                if (what)
                    *problem = what;
                finished();
            });
        if (*waiting == 0) {
            ++*waiting;
            finished();
        }
        showOverview();
    }

    void ThemePage::undo() {
        const auto environment = foundation::currentEnvironment();
        const auto latest      = configuration::latestChange(application::backupsFolder(environment), changeName);
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone) {
            outcome = std::format("Nothing was undone: {}.", undone.error());
        } else {
            // GTK's settings as they were before the change.
            desktop::restoreGsettings(*latest, environment);
            reloadApps();
            outcome = "Undone.";
            waybar.restart();
        }
        showOverview();
    }

    void ThemePage::fitPreview() {
        if (reviewing || cardRows.empty())
            return;
        constexpr double full    = 250;
        // Less than this and the drawing's contents no longer fit in it.
        constexpr double least   = 80;
        constexpr double between = 10;
        double           cards   = 0;
        for (const auto& row : cardRows)
            cards += row->size().y + between;
        auto fitting = std::min(full, body->size().y - cards - between);
        if (fitting < least)
            fitting = 0;
        // Small differences are left, so a pass that changes nothing ends the fitting.
        if (std::abs(fitting - previewHeight) < 4)
            return;
        previewHeight = fitting;
        for (auto at = desktops.begin(); at != desktops.end();)
            at = at->first.second ? desktops.erase(at) : std::next(at);
        updateSelection();
    }

    void ThemePage::restyle() {
        desktops.clear();
        if (!reviewing)
            showOverview();
    }

    void ThemePage::reloadApps() {
        const auto environment = foundation::currentEnvironment();
        const auto path        = foundation::variable(environment, "PATH").value_or("");
        // dunst reads its files again on request.
        if (!desktop::findProcesses("dunst").empty())
            if (const auto dunstctl = desktop::findProgram("dunstctl", path))
                (void)desktop::readCommand(*dunstctl, {"reload"});
    }

}
