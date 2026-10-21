// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/bar-page.hpp"

#include "application/config-check.hpp"
#include "application/settings.hpp"
#include "configuration/main-config.hpp"
#include "application/lantern-folder.hpp"
#include "configuration/diff.hpp"
#include "application/lantern-folder.hpp"
#include "configuration/diff.hpp"
#include "desktop/clipboard.hpp"
#include "desktop/waybar.hpp"
#include "desktop/window-look.hpp"
#include "ui/bar-preview.hpp"
#include "ui/widgets.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>

#include <algorithm>
#include <filesystem>
#include <format>
#include <optional>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        // Three cards to a row, with the gaps between them.
        constexpr double cardsPerRow = 3;
        constexpr double cardShare   = 0.32;

        const desktop::ShapeInfo& shapeInfo(desktop::Shape shape) {
            for (const auto& info : desktop::waybarShapes())
                if (info.shape == shape)
                    return info;
            return desktop::waybarShapes().front();
        }

    }

    BarPage::BarPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop) :
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
        root->addChild(pageTitle("Bar", this->palette));
        root->addChild(paragraph("Lantern has no bar of its own. It recommends Waybar, the bar most Hyprland desktops use, and helps you set it up. "
                                 "It never installs anything: when something is missing, it shows the command for you to run.",
                                 this->palette));

        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(waybar.element());
        root->addChild(body);
        footer = leftRow();
        root->addChild(footer);

        const auto environment = foundation::currentEnvironment();
        settings               = application::loadSettings(application::lanternFolder(environment));
        selected               = desktop::currentBarStyle(environment, desktop::runningBarGeometry())
                                     .value_or(desktop::WaybarStyle{desktop::Shape::islands, &desktop::waybarPalettes().front(), desktop::runningBarGeometry()});
        // The thin line is no longer offered: a bar that has it is shown with the inactive
        // windows' border, ready to review.
        if (selected.border == desktop::Border::thin)
            selected.border = desktop::Border::inactive;
        check();
    }

    void BarPage::close() {
        waybar.close();
    }

    void BarPage::restyle() {
        if (!reviewing)
            showOverview();
    }

    void BarPage::check() {
        waybar.refresh();
        if (!reviewing)
            showOverview();
    }

    BarPage::Card BarPage::card(const std::string& name, std::function<void()> chosenNow, bool nameFirst) {
        Card made;
        made.title = name;
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
        made.name   = markupLine(escapeMarkup(name), palette);
        const CSharedPointer<Hyprtoolkit::IElement> title = made.name;
        const CSharedPointer<Hyprtoolkit::IElement> held = made.holder;
        inside->addChild(nameFirst ? title : held);
        inside->addChild(nameFirst ? held : title);
        made.frame->addChild(inside);
        made.frame->setReceivesMouse(true);
        made.frame->setMouseButton([this, chosenNow = std::move(chosenNow)](Hyprtoolkit::Input::eMouseButton button, bool pressed) {
            if (button == Hyprtoolkit::Input::MOUSE_BUTTON_LEFT && pressed)
                defer(chosenNow);
        });
        return made;
    }

    CSharedPointer<Hyprtoolkit::IElement> BarPage::previewOf(int kind, const desktop::WaybarStyle& style) {
        constexpr double cardScale = 0.42;
        // The cards show the bar with only the standard widgets, as more do not fit at their
        // size; the large one shows them all.
        auto shown = style;
        if (kind != 0)
            shown.widgets = desktop::standardWidgets();
        auto& made = previews[{kind, shown.shape, shown.palette, shown.border, shown.widgets}];
        if (!made)
            made = barPreview(shown, kind == 0 ? 1.0 : cardScale);
        return made;
    }

    CSharedPointer<Hyprtoolkit::IElement> BarPage::styleSection() {
        const auto section = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();

        // The chosen style at Waybar's own size; what applying it would do is in the footer.
        largePreview = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        styleStatusShown.clear();
        section->addChild(largePreview);

        // The look of the bar, and what is on it, each on a tab of its own.
        const auto tabs = leftRow();
        tabs->rebuild()->gap(4)->commence();
        tabButtons.clear();
        for (const bool widgetsTab : {false, true}) {
            auto made = button(widgetsTab ? "Widgets" : "Style", 100, widgetsTab == showingWidgets, [this, widgetsTab] {
                defer([this, widgetsTab] {
                    showingWidgets = widgetsTab;
                    fillChoices();
                    updateStyle();
                });
            });
            tabs->addChild(made);
            tabButtons.push_back(std::move(made));
        }
        section->addChild(space(4));
        section->addChild(tabs);
        choices = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        section->addChild(choices);
        fillChoices();
        updateStyle();
        return section;
    }

    void BarPage::fillChoices() {
        choices->clearChildren();
        shapeCards.clear();
        paletteCards.clear();
        borderCards.clear();
        widgetCards.clear();
        widgetColumns = {};
        dragging.reset();
        target.reset();
        drop.reset();
        for (std::size_t index = 0; index < tabButtons.size(); ++index)
            tabButtons[index]->rebuild()->accent((index == 1) == showingWidgets)->commence();
        if (!showingWidgets) {
            choices->addChild(markupLine("<span weight=\"600\">Shape</span>", palette));
            const auto shapes = leftRow(0);
            shapes->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
            shapeCards.clear();
            for (const auto& info : desktop::waybarShapes()) {
                auto made = card(std::string{info.name}, [this, shape = info.shape] {
                    selected.shape = shape;
                    updateStyle();
                });
                shapes->addChild(made.frame);
                shapeCards.emplace_back(info.shape, std::move(made));
            }
            choices->addChild(shapes);

            choices->addChild(markupLine("<span weight=\"600\">Colours</span>", palette));
            const auto     all    = desktop::waybarPalettes();
            constexpr auto perRow = static_cast<std::size_t>(cardsPerRow);
            paletteCards.clear();
            for (std::size_t first = 0; first < all.size(); first += perRow) {
                const auto colours = leftRow(0);
                colours->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
                for (std::size_t index = first; index < std::min(all.size(), first + perRow); ++index) {
                    auto made = card(std::string{all[index].name}, [this, chosen = &all[index]] {
                        selected.palette = chosen;
                        updateStyle();
                    });
                    colours->addChild(made.frame);
                    paletteCards.emplace_back(&all[index], std::move(made));
                }
                choices->addChild(colours);
            }

            choices->addChild(markupLine("<span weight=\"600\">Border</span>", palette));
            const auto borders = leftRow(0);
            borders->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
            for (const auto& [border, name] : {std::pair{desktop::Border::inactive, "Dim"}, std::pair{desktop::Border::active, "Lit"}}) {
                auto made = card(name, [this, border] {
                    selected.border = border;
                    updateStyle();
                });
                borders->addChild(made.frame);
                borderCards.emplace_back(border, std::move(made));
            }
            choices->addChild(borders);
            return;
        }

        // A column for each side of the bar and one for what is off it. A card is dragged to
        // another column, or onto a card to go before it; a click takes it off the bar, or puts
        // it back on its usual side.
        std::error_code error;
        bool            battery = false;
        for (const auto& supply : std::filesystem::directory_iterator{"/sys/class/power_supply", error})
            battery = battery || supply.path().filename().string().starts_with("BAT");
        const auto table = leftRow(0);
        table->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(10)->commence();
        constexpr std::array<std::string_view, 4> titles{"Left", "Center", "Right", "Off the bar"};
        for (std::size_t column = 0; column < titles.size(); ++column) {
            const std::optional<desktop::Side> side = column < 3 ? std::optional{static_cast<desktop::Side>(column)} : std::nullopt;
            const auto area = Hyprtoolkit::CRectangleBuilder::begin()
                                  ->color([palette = palette] { return palette->m_colors.alternateBase; })
                                  ->rounding(palette->m_vars.bigRounding)
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {0.24, scaled(480.0)}})
                                  ->commence();
            const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
            inside->setMargin(8);
            inside->addChild(markupLine(std::format("<span weight=\"600\">{}</span>", titles[column]), palette));
            widgetColumns[column] = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
            inside->addChild(widgetColumns[column]);
            area->addChild(inside);
            // Dropped on the column itself: at its end.
            area->setReceivesMouse(true);
            area->setMouseButton([this, side](Hyprtoolkit::Input::eMouseButton pressed, bool down) {
                if (pressed != Hyprtoolkit::Input::MOUSE_BUTTON_LEFT || down || !dragging)
                    return;
                if (!drop)
                    drop = Drop{.side = side, .before = std::nullopt, .onCard = false};
                defer([this] { dropWidget(); });
            });
            table->addChild(area);
        }
        for (const auto& info : desktop::waybarWidgets()) {
            // What it shows, and what it would show nothing without on this computer.
            std::string summary{info.summary};
            if (info.widget == desktop::Widget::bluetooth && !std::filesystem::exists("/usr/lib/bluetooth/bluetoothd"))
                summary = "Needs bluez, which is not installed.";
            if (info.widget == desktop::Widget::battery && !battery)
                summary = "This computer reports no battery.";
            const auto card = Hyprtoolkit::CRectangleBuilder::begin()
                                  ->color([palette = palette] { return palette->m_colors.base; })
                                  ->rounding(palette->m_vars.smallRounding)
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, scaled(34.0)}})
                                  ->commence();
            const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
            inside->setMargin(7);
            inside->addChild(markupLine(escapeMarkup(std::string{info.name}), palette));
            card->addChild(inside);
            card->setTooltip(std::move(summary));
            card->setReceivesMouse(true);
            card->setMouseButton([this, widget = info.widget](Hyprtoolkit::Input::eMouseButton pressed, bool down) {
                if (pressed != Hyprtoolkit::Input::MOUSE_BUTTON_LEFT)
                    return;
                if (down) {
                    dragging = widget;
                    drop.reset();
                    defer([this] { colourWidgetCards(); });
                    return;
                }
                if (!dragging)
                    return;
                drop = Drop{.side = desktop::sideOf(selected.widgets, widget), .before = widget, .onCard = true};
                defer([this] { dropWidget(); });
            });
            card->setMouseEnter([this, widget = info.widget](const Hyprutils::Math::Vector2D&) {
                if (dragging && dragging != widget) {
                    target = widget;
                    defer([this] { colourWidgetCards(); });
                }
            });
            card->setMouseLeave([this, widget = info.widget] {
                if (target == widget) {
                    target.reset();
                    defer([this] { colourWidgetCards(); });
                }
            });
            widgetCards[info.widget] = card;
        }
        choices->addChild(table);
        arrangeWidgets();
    }

    void BarPage::arrangeWidgets() {
        for (const auto& column : widgetColumns)
            if (column)
                column->clearChildren();
        if (widgetCards.empty())
            return;
        for (std::size_t side = 0; side < 3; ++side)
            for (const auto widget : selected.widgets[side])
                widgetColumns[side]->addChild(widgetCards[widget]);
        for (const auto& info : desktop::waybarWidgets())
            if (!desktop::sideOf(selected.widgets, info.widget))
                widgetColumns[3]->addChild(widgetCards[info.widget]);
        colourWidgetCards();
    }

    void BarPage::colourWidgetCards() {
        // The card being dragged, and the one it would go before, in the accent.
        for (const auto& [widget, card] : widgetCards) {
            const bool marked = widget == dragging || widget == target;
            card->rebuild()
                ->color([palette = palette, marked, held = widget == dragging] {
                    auto colour = marked ? palette->m_colors.accent : palette->m_colors.base;
                    if (marked)
                        colour.a = held ? 0.45F : 0.25F;
                    return colour;
                })
                ->commence();
        }
    }

    void BarPage::dropWidget() {
        if (!drop || !dragging)
            return;
        const auto moved = *dragging;
        const auto where = *drop;
        dragging.reset();
        target.reset();
        drop.reset();
        if (where.onCard && where.before == moved) {
            // A click: off the bar, or back on its usual side.
            if (desktop::sideOf(selected.widgets, moved))
                desktop::removeWidget(selected.widgets, moved);
            else
                desktop::placeWidget(selected.widgets, moved, std::ranges::find(desktop::waybarWidgets(), moved, &desktop::WidgetInfo::widget)->side);
        } else if (!where.side) {
            desktop::removeWidget(selected.widgets, moved);
        } else {
            desktop::placeWidget(selected.widgets, moved, *where.side, where.onCard ? where.before : std::nullopt);
        }
        updateStyle();
    }

    void BarPage::updateStyle() {
        largePreview->clearChildren();
        largePreview->addChild(previewOf(0, selected));
        const auto mark = [this](Card& made, bool chosen, const desktop::WaybarStyle& shown, int kind) {
            made.holder->clearChildren();
            made.holder->addChild(previewOf(kind, shown));
            made.frame->rebuild()->borderColor([palette = palette, chosen] { return chosen ? palette->m_colors.accent : Hyprtoolkit::CHyprColor{0, 0, 0, 0}; })->commence();
            made.name->rebuild()->text(chosen ? std::format("<span weight=\"600\">{}</span>", escapeMarkup(made.title)) : escapeMarkup(made.title))->commence();
        };
        for (auto& [shape, made] : shapeCards)
            mark(made, shape == selected.shape, desktop::WaybarStyle{shape, selected.palette, selected.geometry, selected.border, selected.widgets}, 1);
        for (auto& [shown, made] : paletteCards)
            mark(made, shown == selected.palette, desktop::WaybarStyle{selected.shape, shown, selected.geometry, selected.border, selected.widgets}, 2);
        for (auto& [border, made] : borderCards)
            mark(made, border == selected.border, desktop::WaybarStyle{selected.shape, selected.palette, selected.geometry, border, selected.widgets}, 3);
        arrangeWidgets();
        fillStyleStatus();
    }

    std::expected<std::vector<configuration::FileChange>, std::string> BarPage::plan(const desktop::WaybarStyle& style) {
        auto planned = desktop::planWaybarStyle(style, foundation::currentEnvironment());
        if (!planned)
            return planned;
        std::erase_if(*planned, [](const auto& change) { return !change.changes(); });
        if (const auto config = configuration::mainConfig(configuration::lanternLaunch())) {
            auto rules = desktop::planPopupRules(*config);
            if (!rules)
                return std::unexpected{rules.error()};
            if (*rules)
                planned->push_back(std::move(**rules));
        }
        return planned;
    }

    void BarPage::fillStyleStatus() {
        // The windows' geometry, which the bar follows, may have changed on the Windows page.
        selected.geometry = desktop::runningBarGeometry();
        const auto environment = foundation::currentEnvironment();
        const auto planned     = plan(selected);
        const bool undoable    = configuration::latestChange(application::backupsFolder(environment), "Waybar").has_value();
        const bool same        = planned && std::ranges::none_of(*planned, [](const auto& change) { return change.changes(); });
        const auto shown       = std::format("{}|{}|{}|{}|{}", planned ? "" : planned.error(), same ? selected.palette->name : "", same ? shapeInfo(selected.shape).name : "", undoable,
                                             outcome);
        if (shown == styleStatusShown)
            return;
        styleStatusShown = shown;

        // Everything said here sits in the footer, so that saying it moves nothing above.
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> buttons;
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> away;
        if (undoable)
            away.push_back(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        std::string note;
        if (!planned) {
            note = escapeMarkup(std::format("Lantern cannot write it: {}.", planned.error()));
        } else if (same) {
            note = std::format("{}  Your Waybar uses {}, {}.", coloured("✓", hex(palette->m_colors.accent)), escapeMarkup(selected.palette->name),
                               escapeMarkup(std::string{shapeInfo(selected.shape).name}));
        } else {
            buttons.push_back(button("Review changes", 160, true, [this] { defer([this] { showReview(selected); }); }));
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

    void BarPage::showOverview() {
        reviewing.reset();
        changes.clear();
        body->clearChildren();

        const auto environment  = foundation::currentEnvironment();
        const auto distribution = desktop::currentDistribution();
        const auto copier       = desktop::clipboardProgram(foundation::variable(environment, "PATH").value_or(""));
        const auto requirements = desktop::waybarRequirements(environment);
        const bool allMet       = std::ranges::all_of(requirements, [](const auto& requirement) { return requirement.met(); });

        // Once everything is installed and the user dismissed the list, it stays hidden,
        // until something goes missing again.
        if (!allMet || !settings.barRequirementsDismissed) {
            const auto checks = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(18)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
            for (const auto& requirement : requirements)
                checks->addChild(requirementBlock(requirement, distribution, copier, palette, defer));

            const auto config = desktop::waybarConfig(environment);
            std::string reads;
            if (!config)
                reads = "Waybar has no config to read, so it will not start.";
            else if (config->string().starts_with("/etc/"))
                reads = std::format("Waybar reads its example config, {}. It was written for Sway, so it shows none of Hyprland's workspaces.", config->string());
            else
                reads = std::format("Waybar reads {}.", shortPath(*config));
            checks->addChild(paragraph(reads, palette));

            body->addChild(space(4));
            body->addChild(checks);
            const auto actions = leftRow();
            actions->addChild(button("Check again", 140, false, [this] { defer([this] { check(); }); }));
            if (allMet)
                actions->addChild(button("Dismiss", 110, false, [this] {
                    defer([this] {
                        settings.barRequirementsDismissed = true;
                        if (const auto saved = application::saveSettings(settings, application::lanternFolder(foundation::currentEnvironment())); !saved)
                            outcome = std::format("Lantern could not save its settings: {}.", saved.error());
                        showOverview();
                    });
                }));
            body->addChild(actions);
            body->addChild(space(8));
        }
        body->addChild(styleSection());
    }

    void BarPage::showReview(const desktop::WaybarStyle& style) {
        auto planned = plan(style);
        if (!planned) {
            outcome = planned.error();
            return showOverview();
        }
        reviewing = style;
        changes   = std::move(*planned);
        outcome.clear();
        body->clearChildren();

        body->addChild(space(4));
        body->addChild(heading(std::format("Review: {}, {}", style.palette->name, shapeInfo(style.shape).name), palette));
        const bool off = !waybar.running();
        const bool rules = std::ranges::any_of(changes, [](const auto& change) { return change.file.extension() == ".lua"; });
        body->addChild(paragraph(std::format("Applying writes these files. A copy of each file it replaces is kept first, so it can be undone. {}{}",
                                             off ? "Waybar is off, so it will not be restarted." : "Waybar then restarts to read them.",
                                             rules ? " The Hyprland config gets rules that float the volume control and the network settings the bar opens, under its right end." : ""),
                                 palette, true));
        if (desktop::sideOf(style.widgets, desktop::Widget::weather))
            body->addChild(paragraph("The weather widget asks the website wttr.in for the temperature every half hour. The site sees your computer's address, "
                                     "which tells it roughly where you are.",
                                     palette, true));
        setFooter(footer, {button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }), button("Apply", 110, true, [this] { defer([this] { apply(); }); })});
        diff.setRows(diffRows(changes, *palette));
        body->addChild(diff.element());
    }

    void BarPage::apply() {
        if (!reviewing)
            return;
        const auto record = configuration::applyChanges(changes, application::backupsFolder(foundation::currentEnvironment()), std::format("Waybar style: {}, {}", reviewing->palette->name, shapeInfo(reviewing->shape).name));
        if (!record) {
            outcome = std::format("Nothing was written: {}.", record.error());
        } else {
            outcome = "Written. Waybar is restarting…";
            waybar.restartToRead(*record, [this](std::string what) {
                outcome = std::move(what);
                if (!reviewing)
                    showOverview();
            });
            // Hyprland reads the rules by itself; a mistake in them is undone.
            const auto hyprctl = desktop::findProgram("hyprctl", foundation::variable(foundation::currentEnvironment(), "PATH").value_or(""));
            if (hyprctl && std::ranges::any_of(changes, [](const auto& change) { return change.file.extension() == ".lua"; }))
                application::checkHyprlandConfig(loop, *hyprctl, *record, [this](std::optional<std::string> problem) {
                    if (!problem)
                        return;
                    outcome = *problem;
                    if (!reviewing)
                        showOverview();
                });
        }
        showOverview();
    }

    void BarPage::undo() {
        const auto latest = configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), "Waybar");
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone) {
            outcome = std::format("Nothing was undone: {}.", undone.error());
        } else if (waybar.running()) {
            outcome = "Undone, and Waybar is restarting to read the files as they were.";
            waybar.restart();
        } else {
            outcome = "Undone.";
        }
        showOverview();
    }

}
