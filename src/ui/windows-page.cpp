// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/windows-page.hpp"

#include "application/config-check.hpp"
#include "application/studio-folder.hpp"
#include "configuration/main-config.hpp"
#include "desktop/system.hpp"
#include "desktop/theme.hpp"
#include "ui/bar-preview.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"
#include "ui/widgets.hpp"

#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Text.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr auto   changeName    = "Windows";
        constexpr double previewHeight = 230;
        // The preview draws the desktop at a little over half its size.
        constexpr double previewScale  = 0.6;
        // Waybar's height, as the preview draws it.
        constexpr double barHeight     = 22;

        // The colours windows are drawn in: the desktop's theme, if the studio set one, or
        // the studio's own.
        struct Colours {
            std::string backdrop;
            std::string window;
            std::string content;
            std::string active;
            std::string inactive;
        };

        Colours coloursFor(const Hyprtoolkit::CPalette& studio) {
            if (const auto config = configuration::mainConfig(configuration::studioLaunch()))
                if (const auto* theme = desktop::currentTheme(*config))
                    return {theme->dark ? "#3b4252" : "#b6bccb", std::string{theme->base}, std::string{theme->raised}, std::string{theme->accent}, std::string{theme->raised}};
            return {darkTheme(studio) ? "#3b4252" : "#b6bccb", hex(studio.m_colors.base), hex(studio.m_colors.alternateBase), hex(studio.m_colors.accent),
                    hex(studio.m_colors.alternateBase)};
        }

        CSharedPointer<Hyprtoolkit::CRectangleElement> block(const std::string& colour, CDynamicSize size, double rounding = 0, const std::string& border = {}, int thickness = 0) {
            auto builder = Hyprtoolkit::CRectangleBuilder::begin()
                               ->color([c = paletteColour(colour)] { return c; })
                               ->rounding(static_cast<int>(std::lround(rounding)))
                               ->size(std::move(size));
            if (thickness > 0)
                builder->borderColor([c = paletteColour(border)] { return c; })->borderThickness(thickness);
            return builder->commence();
        }

        CDynamicSize fixed(double width, double height) {
            return CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {width, height}};
        }

        CDynamicSize filling() {
            return CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}};
        }

        // A card's picture: what is drawn, centred up and down; a column centres it across.
        CSharedPointer<Hyprtoolkit::IElement> cardPicture(const std::string& background, const CSharedPointer<Hyprtoolkit::IElement>& inside, double used) {
            const auto frame  = block(background, CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, ChoiceCards::defaultPictureHeight}}, 6);
            const auto column = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(filling())->commence();
            column->addChild(space(std::max(0.0, (ChoiceCards::defaultPictureHeight - used) / 2)));
            column->addChild(inside);
            frame->addChild(column);
            return frame;
        }

        // Empty space `width` wide, for a row.
        CSharedPointer<Hyprtoolkit::IElement> across(double width) {
            return Hyprtoolkit::CNullBuilder::begin()->size(fixed(width, 1))->commence();
        }

        // A border's width as drawn small: never thinner than a pixel unless there is none.
        int drawnBorder(int border, double scale) {
            return border == 0 ? 0 : std::max(1, static_cast<int>(std::lround(border * scale)));
        }

    }

    WindowsPage::WindowsPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop) :
        palette{std::move(palette)},
        defer{std::move(defer)},
        loop{std::move(loop)},
        cards{this->palette, this->defer, [this] { fillPreview(); }},
        diff{this->palette, this->defer,
             {RowList::Column{.width = 24, .monospace = true, .secondary = false, .markup = true, .heading = ""},
              RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true, .heading = ""}},
             20},
        waybar{this->palette, this->defer, this->loop, "waybar", "Waybar", nullptr} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(filling())->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Windows", this->palette));
        root->addChild(paragraph("How windows look and are handled: their corners, the gaps around them, their border, and whether dragging that border resizes "
                                 "them. Holding Super and dragging with the right button resizes a window either way. Nothing is written until you review the change.",
                                 this->palette));
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(body);
        footer = leftRow();
        root->addChild(footer);
        if (const auto config = configuration::mainConfig(configuration::studioLaunch()))
            current = desktop::currentWindowLook(*config);
        wanted = current.value_or(desktop::WindowLook{});
        showOverview();
    }

    void WindowsPage::close() {
        waybar.close();
    }

    std::expected<std::vector<configuration::FileChange>, std::string> WindowsPage::plan() const {
        const auto config = configuration::mainConfig(configuration::studioLaunch());
        if (!config)
            return std::unexpected{config.error()};
        const auto hyprland = desktop::planWindowLook(wanted, *config);
        if (!hyprland)
            return std::unexpected{hyprland.error()};
        auto planned = desktop::planBarFollowing(wanted, current, foundation::currentEnvironment());
        if (!planned)
            return std::unexpected{planned.error()};
        if (*hyprland)
            planned->insert(planned->begin(), **hyprland);
        return planned;
    }

    void WindowsPage::restyle() {
        cards.forget();
        if (!reviewing)
            showOverview();
    }

    void WindowsPage::showOverview() {
        reviewing = false;
        body->clearChildren();
        cards.clear();
        statusShown.clear();

        preview = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        body->addChild(preview);
        body->addChild(space(6));

        // Settings two to a row, so each pair lines up: a row centres what is shorter.
        std::vector<CSharedPointer<Hyprtoolkit::CColumnLayoutElement>> sides;
        for (int row = 0; row < 3; ++row) {
            const auto line = leftRow(0);
            line->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(24)->commence();
            for (int side = 0; side < 2; ++side) {
                sides.push_back(Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {0.49, 1.0}})->commence());
                line->addChild(sides.back());
            }
            body->addChild(line);
        }
        // Row by row: corners and the gap between windows, the border and the screen's
        // edge, then resizing.
        const auto& corners = sides[0];
        const auto& between = sides[1];
        const auto& border  = sides[2];
        const auto& edge    = sides[3];
        const auto& resizing = sides[4];

        const auto colours = coloursFor(*palette);
        // Presets, and the value the config has now when it is none of them, shown as
        // `shown` times the value: the distance it makes on screen.
        const auto choicesOf = [this](int desktop::WindowLook::*member, std::vector<std::pair<std::string, int>> presets, const std::function<CSharedPointer<Hyprtoolkit::IElement>(int)>& draw,
                                      const std::string& kind, int shown = 1) {
            if (current && std::ranges::none_of(presets, [&](const auto& preset) { return preset.second == (*current).*member; }))
                presets.emplace_back(std::format("Yours · {} px", (*current).*member * shown), (*current).*member);
            std::vector<ChoiceCards::Choice> made;
            for (const auto& [label, value] : presets)
                made.push_back(ChoiceCards::Choice{
                    .label  = label,
                    .chosen = [this, member, value] { return wanted.*member == value; },
                    .select = [this, member, value] { wanted.*member = value; },
                    .draw   = [this, draw, value, kind] { return cards.picture(std::format("{} {} {}", kind, value, wanted.rounding), [&] { return draw(value); }); },
                });
            return made;
        };

        cards.addSetting(corners, "Corners", "how round each window is",
                         choicesOf(&desktop::WindowLook::rounding, {{"Square", 0}, {"Slight", 4}, {"Round", 10}, {"Very round", 18}},
                                   [colours](int radius) {
                                       return cardPicture(colours.backdrop, block(colours.window, fixed(70, 38), radius * 0.8, colours.active, 2), 38);
                                   },
                                   "corners"));
        cards.addSetting(border, "Border", "the coloured line around each window",
                         choicesOf(&desktop::WindowLook::borderSize, {{"None", 0}, {"Thin", 1}, {"Normal", 2}, {"Thick", 4}},
                                   // Drawn with the corners chosen, so the two read as one window.
                                   [this, colours](int border) {
                                       return cardPicture(colours.backdrop, block(colours.window, fixed(70, 38), wanted.rounding * 0.8, colours.active, border), 38);
                                   },
                                   "border"));
        std::vector<ChoiceCards::Choice> resize;
        for (const auto& [label, on] : {std::pair{"Off", false}, std::pair{"On", true}})
            resize.push_back(ChoiceCards::Choice{
                .label  = label,
                .chosen = [this, on] { return wanted.resizeOnBorder == on; },
                .select = [this, on] { wanted.resizeOnBorder = on; },
                .draw =
                    [this, on, colours] {
                        return cards.picture(std::format("resize {} {}", on, wanted.rounding), [&] {
                            // On, a window whose highlighted edge is being dragged out to the
                            // outline of the size it will be; off, a window and nothing more.
                            const auto radius  = wanted.rounding * 0.8;
                            const auto outline = block(colours.backdrop, fixed(80, 38), radius, colours.content, on ? 1 : 0);
                            const auto row     = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(3)->size(filling())->commence();
                            row->addChild(block(colours.window, fixed(46, 38), radius, on ? colours.active : colours.inactive, on ? 3 : 1));
                            if (on) {
                                const auto arrow = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
                                arrow->addChild(space(8));
                                arrow->addChild(Hyprtoolkit::CTextBuilder::begin()
                                                    ->text("→")
                                                    ->fontSize(CFontSize{CFontSize::HT_FONT_ABSOLUTE, 14.F})
                                                    ->color([c = paletteColour(colours.active)] { return c; })
                                                    ->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                                                    ->commence());
                                row->addChild(arrow);
                            }
                            outline->addChild(row);
                            return cardPicture(colours.backdrop, outline, 38);
                        });
                    },
            });
        cards.addSetting(resizing, "Resize from the border", "drag a window's edge to resize it", std::move(resize));

        // Hyprland keeps its inner gap on each side of a window, so two windows are twice it
        // apart. The choices are named and shown by that distance, in the same steps as the
        // screen's edge, so the same choice in both is the same gap.
        cards.addSetting(between, "Between windows", "the distance between two windows",
                         choicesOf(&desktop::WindowLook::gapsIn, {{"None", 0}, {"Small · 10 px", 5}, {"Normal · 20 px", 10}, {"Wide · 40 px", 20}},
                                   [colours](int gap) {
                                       // Two plain windows, and the gap between them drawn as a line as
                                       // thick as it is; with none they meet at a hairline.
                                       const auto row = Hyprtoolkit::CRowLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 38.0}})->commence();
                                       row->addChild(block(colours.window, fixed(36, 38), 3));
                                       if (gap == 0)
                                           row->addChild(block(colours.content, fixed(1, 38)));
                                       else
                                           row->addChild(block(colours.active, fixed(std::max(2.0, std::round(gap * 2 * 0.3)), 38), 1));
                                       row->addChild(block(colours.window, fixed(36, 38), 3));
                                       return cardPicture(colours.backdrop, row, 38);
                                   },
                                   "between", 2));
        cards.addSetting(edge, "Screen edge", "the gap between windows and the edges of the screen",
                         choicesOf(&desktop::WindowLook::gapsOut, {{"None", 0}, {"Small · 10 px", 10}, {"Normal · 20 px", 20}, {"Wide · 40 px", 40}},
                                   [colours](int gap) {
                                       // A monitor on its stand, the gap between its edge and the window on
                                       // it highlighted.
                                       const auto bezel = block(colours.window, fixed(84, 42), 4);
                                       const auto glass = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(filling())->commence();
                                       glass->setMargin(3);
                                       const auto edgeGap = block(colours.active, filling(), 1);
                                       const auto inside  = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(filling())->commence();
                                       inside->setMargin(gap * 0.3);
                                       inside->addChild(block(colours.content, filling(), 2));
                                       edgeGap->addChild(inside);
                                       glass->addChild(edgeGap);
                                       bezel->addChild(glass);
                                       const auto monitor = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
                                       monitor->addChild(bezel);
                                       monitor->addChild(block(colours.window, fixed(10, 4)));
                                       monitor->addChild(block(colours.window, fixed(30, 3), 1));
                                       return cardPicture(colours.backdrop, monitor, 49);
                                   },
                                   "edge"));

        cards.update();
        fillPreview();
    }

    void WindowsPage::fillPreview() {
        preview->clearChildren();
        const auto colours = coloursFor(*palette);
        const auto scale   = previewScale;
        const auto radius  = wanted.rounding * scale;
        const auto border  = drawnBorder(wanted.borderSize, scale);
        const auto between = std::lround(wanted.gapsIn * 2 * scale);

        const auto edge    = wanted.gapsOut * scale;

        // The screen: Waybar at the top, half the outer gap from it and the whole gap from
        // the sides, with its corners following the windows', as the bar is set to; then
        // windows the outer gap from the bar and the screen's edges.
        const auto screen = block(colours.backdrop, CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, previewHeight}}, 8);
        const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(filling())->commence();
        inside->addChild(space(edge / 2));
        const auto barLine = Hyprtoolkit::CRowLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, barHeight}})->commence();
        barLine->addChild(across(edge));
        const auto bar = block(colours.window, CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}}, std::min(radius, barHeight / 2));
        bar->setGrow(true, false);
        const auto pieces = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(3)->size(filling())->commence();
        pieces->setMargin(5);
        const auto pill = [&](const std::string& colour, double width) {
            return block(colour, CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {width, 1.0}}, std::min(std::round(radius * 0.7), 4.0));
        };
        // Workspaces on the left, the clock in the middle, the status on the right.
        pieces->addChild(pill(colours.active, 14));
        for (int workspace = 0; workspace < 3; ++workspace)
            pieces->addChild(pill(colours.content, 14));
        const auto springs = [&] {
            const auto made = across(1);
            made->setGrow(true, false);
            pieces->addChild(made);
        };
        springs();
        pieces->addChild(pill(colours.content, 50));
        springs();
        pieces->addChild(pill(colours.content, 22));
        pieces->addChild(pill(colours.content, 22));
        bar->addChild(pieces);
        barLine->addChild(bar);
        barLine->addChild(across(edge));
        inside->addChild(barLine);
        inside->addChild(space(edge));
        const auto windowLine = Hyprtoolkit::CRowLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        windowLine->setGrow(false, true);
        windowLine->addChild(across(edge));
        // One window on the left, focused, and two stacked on the right, as Hyprland tiles three.
        const auto row = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(static_cast<std::size_t>(between))->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        const auto window = [&](const std::string& edge, CDynamicSize size) {
            const auto made  = block(colours.window, std::move(size), radius, edge, border);
            const auto lines = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(filling())->commence();
            lines->setMargin(14);
            for (const double share : {0.5, 0.8, 0.65})
                lines->addChild(block(colours.content, CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {share, 5.0}}, 2));
            made->addChild(lines);
            return made;
        };
        const auto focused = window(colours.active, CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}});
        // Takes the width the stack on the right leaves.
        focused->setGrow(true, false);
        row->addChild(focused);
        const auto stack = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(static_cast<std::size_t>(between))->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.42, 1.0}})->commence();
        const auto top = window(colours.inactive, CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}});
        top->setGrow(false, true);
        stack->addChild(top);
        stack->addChild(window(colours.inactive, CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 0.5}}));
        row->addChild(stack);
        row->setGrow(true, false);
        windowLine->addChild(row);
        windowLine->addChild(across(edge));
        inside->addChild(windowLine);
        inside->addChild(space(edge));
        screen->addChild(inside);
        preview->addChild(screen);
        fillStatus();
    }

    void WindowsPage::fillStatus() {
        const auto environment = foundation::currentEnvironment();
        const auto planned     = plan();
        const bool undoable    = configuration::latestChange(application::backupsFolder(environment), changeName).has_value();
        const bool same        = planned && planned->empty();
        const auto shown       = std::format("{}|{}|{}|{}|{}|{}|{}|{}|{}", wanted.rounding, wanted.gapsIn, wanted.gapsOut, wanted.borderSize, wanted.resizeOnBorder, same,
                                             planned ? "" : planned.error(), undoable, outcome);
        if (shown == statusShown)
            return;
        statusShown = shown;

        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> buttons;
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> away;
        if (undoable)
            away.push_back(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        std::string note;
        if (!planned) {
            note = escapeMarkup(std::format("The studio cannot apply it: {}.", planned.error()));
        } else if (same) {
            note = std::format("{}  {}", coloured("✓", hex(palette->m_colors.accent)), escapeMarkup(outcome.empty() ? "Your windows look like this." : outcome));
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

    void WindowsPage::showReview() {
        auto planned = plan();
        if (!planned || planned->empty()) {
            outcome = planned ? "" : planned.error();
            return showOverview();
        }
        reviewing = true;
        changes   = std::move(*planned);
        outcome.clear();
        body->clearChildren();
        body->addChild(space(4));
        body->addChild(heading("Review: windows", palette));
        const bool bar = std::ranges::any_of(changes, [](const auto& change) { return change.file.parent_path().filename() == "waybar"; });
        body->addChild(paragraph(std::format("Applying writes the changes below, after keeping a copy of each file so it can be undone. Hyprland reads its config at "
                                             "once; if it reports a problem with it, the studio puts the config back.{}",
                                             bar ? " Waybar's corners and its distance from the edges follow the windows', so its files change too, and it restarts to "
                                                   "read them while it is on."
                                                 : ""),
                                 palette, true));
        setFooter(footer, {button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }), button("Apply", 110, true, [this] { defer([this] { apply(); }); })});
        diff.setRows(diffRows(changes, *palette));
        body->addChild(diff.element());
    }

    void WindowsPage::apply() {
        if (!reviewing)
            return;
        const auto environment = foundation::currentEnvironment();
        const auto record      = configuration::applyChanges(changes, application::backupsFolder(environment), changeName);
        if (!record) {
            outcome = std::format("Nothing was written: {}.", record.error());
            return showOverview();
        }
        outcome = "Applied.";
        if (std::ranges::any_of(changes, [](const auto& change) { return change.file.parent_path().filename() == "waybar"; }))
            waybar.restartToRead(*record, [this](std::string what) {
                if (!what.contains("put back"))
                    return;
                outcome = std::move(what);
                if (!reviewing)
                    showOverview();
            });
        if (const auto hyprctl = desktop::findProgram("hyprctl", foundation::variable(environment, "PATH").value_or("")))
            application::checkHyprlandConfig(loop, *hyprctl, *record, [this](std::optional<std::string> problem) {
                if (!problem)
                    return;
                outcome = *problem;
                if (const auto config = configuration::mainConfig(configuration::studioLaunch()))
                    current = desktop::currentWindowLook(*config);
                if (!reviewing)
                    showOverview();
            });
        if (const auto config = configuration::mainConfig(configuration::studioLaunch()))
            current = desktop::currentWindowLook(*config);
        showOverview();
    }

    void WindowsPage::undo() {
        const auto latest = configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), changeName);
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone)
            outcome = std::format("Nothing was undone: {}.", undone.error());
        else
            outcome = "Undone.";
        if (const auto config = configuration::mainConfig(configuration::studioLaunch()))
            current = desktop::currentWindowLook(*config);
        wanted = current.value_or(wanted);
        showOverview();
    }

}
