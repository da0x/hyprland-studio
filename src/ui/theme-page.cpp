// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/theme-page.hpp"

#include "application/config-check.hpp"
#include "application/studio-folder.hpp"
#include "configuration/main-config.hpp"
#include "desktop/process.hpp"
#include "desktop/system.hpp"
#include "desktop/theme.hpp"
#include "ui/bar-preview.hpp"
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

        CSharedPointer<Hyprtoolkit::IElement> gap(double width, double height) {
            return Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {width, height}})->commence();
        }

        // A bar of a colour, standing for a line of text or a button in a window.
        CSharedPointer<Hyprtoolkit::IElement> stroke(std::string_view hex, double share, double height, double rounding) {
            return Hyprtoolkit::CRectangleBuilder::begin()
                ->color([c = paletteColour(hex)] { return c; })
                ->rounding(static_cast<int>(rounding))
                ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {share, height}})
                ->commence();
        }

        // A window as the theme draws it: hyprtoolkit's colours inside, Hyprland's border
        // around, a gradient when it has the focus.
        CSharedPointer<Hyprtoolkit::IElement> window(const desktop::Palette& shown, bool focused, double share, double height, double scale) {
            auto builder = Hyprtoolkit::CRectangleBuilder::begin()
                               ->color([c = paletteColour(shown.base)] { return c; })
                               ->rounding(static_cast<int>(10 * scale))
                               ->borderThickness(std::max(1, static_cast<int>(2 * scale)))
                               ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {share, height}});
            if (focused)
                builder->borderGradient([accent = paletteColour(shown.accent, 0.93F), audio = paletteColour(shown.audio, 0.93F)] {
                    Hyprtoolkit::CGradientValueData gradient;
                    gradient.m_vColors = {accent, audio};
                    gradient.m_fAngle  = std::numbers::pi_v<float> / 4;
                    return gradient;
                });
            else
                builder->borderColor([c = paletteColour(shown.raised, 0.67F)] { return c; });
            const auto made = builder->commence();

            const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()
                                    ->gap(static_cast<std::size_t>(8 * scale))
                                    ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})
                                    ->commence();
            inside->setMargin(static_cast<int>(14 * scale));
            if (focused) {
                // An app: a title, a panel with lines, and the accent button.
                inside->addChild(stroke(shown.text, 0.45, 10 * scale, 3 * scale));
                const auto panel = Hyprtoolkit::CRectangleBuilder::begin()
                                       ->color([c = paletteColour(shown.surface)] { return c; })
                                       ->rounding(static_cast<int>(6 * scale))
                                       ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 56 * scale}})
                                       ->commence();
                const auto lines = Hyprtoolkit::CColumnLayoutBuilder::begin()
                                       ->gap(static_cast<std::size_t>(8 * scale))
                                       ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})
                                       ->commence();
                lines->setMargin(static_cast<int>(10 * scale));
                lines->addChild(stroke(shown.subtle, 0.8, 6 * scale, 3 * scale));
                lines->addChild(stroke(shown.subtle, 0.6, 6 * scale, 3 * scale));
                lines->addChild(stroke(shown.audio, 0.4, 6 * scale, 3 * scale));
                panel->addChild(lines);
                inside->addChild(panel);
                inside->addChild(stroke(shown.accent, 0.3, 16 * scale, 5 * scale));
            } else {
                // A terminal: lines of output in a few colours.
                inside->addChild(stroke(shown.network, 0.5, 6 * scale, 3 * scale));
                inside->addChild(stroke(shown.subtle, 0.8, 6 * scale, 3 * scale));
                inside->addChild(stroke(shown.subtle, 0.65, 6 * scale, 3 * scale));
                inside->addChild(stroke(shown.alert, 0.35, 6 * scale, 3 * scale));
                inside->addChild(stroke(shown.subtle, 0.7, 6 * scale, 3 * scale));
            }
            made->addChild(inside);
            return made;
        }

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
                                 "on hyprtoolkit, this studio among them. The terminal has themes of its own, on the Terminal page. Nothing is written until "
                                 "you review the change.",
                                 this->palette));
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(body);
        footer = leftRow();
        root->addChild(footer);

        const auto config = configuration::mainConfig(configuration::studioLaunch());
        selected          = config ? desktop::currentTheme(*config) : nullptr;
        if (selected == nullptr)
            if (const auto style = desktop::currentWaybarStyle(foundation::currentEnvironment()))
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
        const double scale  = large ? 1.0 : 0.42;
        const double height = large ? 250 : 105;
        const auto   shape  = desktop::currentWaybarStyle(foundation::currentEnvironment()).value_or(desktop::WaybarStyle{desktop::Shape::islands, shown}).shape;

        const auto backdrop = Hyprtoolkit::CRectangleBuilder::begin()
                                  ->color([c = shown->dark ? paletteColour("#3b4252") : paletteColour("#b6bccb")] { return c; })
                                  ->rounding(static_cast<int>(8 * scale))
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, height}})
                                  ->commence();
        const auto stack = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        stack->addChild(barPreview(desktop::WaybarStyle{shape, shown}, scale * 0.8));
        const auto windows = Hyprtoolkit::CRowLayoutBuilder::begin()
                                 ->gap(static_cast<std::size_t>(14 * scale))
                                 ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, height - barPreviewHeight({shape, shown}, scale * 0.8) - 12 * scale}})
                                 ->commence();
        const double windowHeight = height - barPreviewHeight({shape, shown}, scale * 0.8) - 22 * scale;
        windows->addChild(gap(16 * scale, 1));
        windows->addChild(window(*shown, true, 0.56, windowHeight, scale));
        windows->addChild(window(*shown, false, 0.36, windowHeight, scale));
        stack->addChild(windows);
        backdrop->addChild(stack);
        made = backdrop;
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
        }
        updateSelection();
    }

    void ThemePage::updateSelection() {
        largePreview->clearChildren();
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
        const auto config      = configuration::mainConfig(configuration::studioLaunch());
        const auto shape       = desktop::currentWaybarStyle(environment).value_or(desktop::WaybarStyle{desktop::Shape::islands, selected}).shape;
        const auto planned     = config ? desktop::planTheme(*selected, shape, *config, environment)
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
            note = escapeMarkup(std::format("The studio cannot apply it: {}.", planned.error()));
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
        const auto config      = configuration::mainConfig(configuration::studioLaunch());
        const auto shape       = desktop::currentWaybarStyle(environment).value_or(desktop::WaybarStyle{desktop::Shape::islands, selected}).shape;
        auto       planned     = config ? desktop::planTheme(*selected, shape, *config, environment)
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
        steps.emplace_back("Show the new colours at once: Hyprland's borders, this studio, Waybar, the launcher and dunst's notifications.");
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
            // The studio's own colours, set before hyprtoolkit sees the file change and
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

        // Waybar comes back and Hyprland checks the config while the studio keeps running;
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

    void ThemePage::reloadApps() {
        const auto environment = foundation::currentEnvironment();
        const auto path        = foundation::variable(environment, "PATH").value_or("");
        // dunst reads its files again on request.
        if (!desktop::findProcesses("dunst").empty())
            if (const auto dunstctl = desktop::findProgram("dunstctl", path))
                (void)desktop::readCommand(*dunstctl, {"reload"});
    }

}
