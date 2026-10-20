// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/bar-page.hpp"

#include "application/settings.hpp"
#include "application/studio-folder.hpp"
#include "configuration/diff.hpp"
#include "application/studio-folder.hpp"
#include "configuration/diff.hpp"
#include "desktop/clipboard.hpp"
#include "desktop/waybar.hpp"
#include "ui/bar-preview.hpp"
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

        // A line of markup that wraps, as wide as its column.
        CSharedPointer<Hyprtoolkit::CTextElement> markupLine(const std::string& markup, const CSharedPointer<Hyprtoolkit::CPalette>& palette, CFontSize size = CFontSize{CFontSize::HT_FONT_TEXT},
                                                             float alpha = 0.85F) {
            return Hyprtoolkit::CTextBuilder::begin()
                ->text(std::string{markup})
                ->fontSize(std::move(size))
                ->color([palette] { return palette->m_colors.text; })
                ->a(alpha)
                ->noEllipsize(true)
                ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                ->commence();
        }

        CSharedPointer<Hyprtoolkit::IElement> space(double height) {
            return Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, height}})->commence();
        }

        // A path under the home folder with ~ for the home folder.
        std::string shortPath(const std::filesystem::path& path) {
            const auto* home = std::getenv("HOME"); // NOLINT(concurrency-mt-unsafe): read once, on the UI thread
            if (home == nullptr)
                return path.string();
            const auto relative = path.lexically_relative(home);
            if (relative.empty() || relative.native().starts_with(".."))
                return path.string();
            return (std::filesystem::path{"~"} / relative).string();
        }

        // A command to type, in the monospace font on a panel of its own.
        CSharedPointer<Hyprtoolkit::IElement> commandPanel(const std::string& command, const CSharedPointer<Hyprtoolkit::CPalette>& palette) {
            const auto panel = Hyprtoolkit::CRectangleBuilder::begin()
                                   ->color([palette] { return palette->m_colors.alternateBase; })
                                   ->rounding(palette->m_vars.smallRounding)
                                   ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 40.0}})
                                   ->commence();
            // Takes the row's width left over by the Copy button.
            panel->setGrow(true, false);
            const auto text = Hyprtoolkit::CTextBuilder::begin()
                                  ->text(std::format("<span font_family=\"{}\">{}</span>", escapeMarkup(palette->m_vars.fontFamilyMonospace), escapeMarkup(command)))
                                  ->color([palette] { return palette->m_colors.text; })
                                  ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                                  ->commence();
            text->setMargin(12);
            panel->addChild(text);
            return panel;
        }

        // The command's panel, with a button that copies it when wl-copy is installed.
        CSharedPointer<Hyprtoolkit::IElement> commandRow(const std::string& command, const std::optional<std::filesystem::path>& copier,
                                                         const CSharedPointer<Hyprtoolkit::CPalette>& palette, const std::function<void(std::function<void()>)>& defer) {
            const auto row = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 40.0}})->commence();
            row->addChild(commandPanel(command, palette));
            if (!copier)
                return row;
            row->addChild(Hyprtoolkit::CButtonBuilder::begin()
                              ->label("Copy")
                              ->noBorder(true)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {90.0, 40.0}})
                              ->onMainClick([command, program = *copier, defer](CSharedPointer<Hyprtoolkit::CButtonElement> button) {
                                  const auto copied = desktop::copyText(program, command);
                                  defer([copied, weak = Hyprutils::Memory::CWeakPointer<Hyprtoolkit::CButtonElement>{button}] {
                                      if (const auto shown = weak.lock())
                                          shown->rebuild()->label(copied ? "Copied" : "Not copied")->commence();
                                  });
                              })
                              ->commence());
            return row;
        }

        CSharedPointer<Hyprtoolkit::IElement> requirementBlock(const desktop::Requirement& requirement, const desktop::Distribution& distribution,
                                                               const std::optional<std::filesystem::path>& copier, const CSharedPointer<Hyprtoolkit::CPalette>& palette,
                                                               const std::function<void(std::function<void()>)>& defer) {
            const bool dark  = darkTheme(*palette);
            const auto green = dark ? std::string{"#a6e3a1"} : std::string{"#2f7d32"};
            const auto red   = dark ? std::string{"#f38ba8"} : std::string{"#c62828"};

            const auto block = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
            const auto mark  = requirement.met() ? coloured("✓", green) : coloured("✗", red);
            const auto state = requirement.met() ? escapeMarkup(*requirement.found) : coloured("not installed", red);
            block->addChild(markupLine(std::format("{}  <span weight=\"600\">{}</span>   {}", mark, escapeMarkup(requirement.name), state), palette));
            block->addChild(paragraph(requirement.purpose, palette, true));
            block->addChild(markupLine(coloured(escapeMarkup(requirement.upstream), hex(palette->m_colors.linkText)), palette, CFontSize{CFontSize::HT_FONT_SMALL}));
            if (requirement.met())
                return block;

            block->addChild(space(4));
            if (const auto command = desktop::installCommand(requirement, distribution); !command.empty()) {
                block->addChild(paragraph("Run this in a terminal, then press Check again:", palette));
                block->addChild(commandRow(command, copier, palette, defer));
                if (!copier)
                    block->addChild(paragraph("A Copy button appears here when wl-clipboard is installed; the studio copies through its wl-copy.", palette, true));
            } else {
                const auto where = distribution.name.empty() ? std::string{"your distribution"} : distribution.name;
                block->addChild(paragraph(std::format("Install it from {}'s packages, then press Check again. On Arch Linux the package is {}.", where, requirement.package), palette));
            }
            return block;
        }

        CSharedPointer<Hyprtoolkit::CButtonElement> button(std::string label, double width, bool accent, std::function<void()> clicked) {
            return Hyprtoolkit::CButtonBuilder::begin()
                ->label(std::move(label))
                ->noBorder(true)
                ->accent(accent)
                ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {width, 34.0}})
                ->onMainClick([clicked = std::move(clicked)](CSharedPointer<Hyprtoolkit::CButtonElement>) { clicked(); })
                ->commence();
        }

        // A full-width row, so what is in it starts at the left edge rather than the column's middle.
        CSharedPointer<Hyprtoolkit::CRowLayoutElement> leftRow(double height = 34.0) {
            return Hyprtoolkit::CRowLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, height}})->commence();
        }

        CSharedPointer<Hyprtoolkit::IElement> heading(const std::string& text, const CSharedPointer<Hyprtoolkit::CPalette>& palette) {
            return markupLine(std::format("<span weight=\"600\">{}</span>", escapeMarkup(text)), palette, CFontSize{CFontSize::HT_FONT_H3}, 1.0F);
        }

        // A diff as list rows: a header per file, then its changes with a sign and a colour.
        std::vector<RowList::Row> diffRows(const std::vector<configuration::FileChange>& changes, const Hyprtoolkit::CPalette& palette) {
            using Kind                        = configuration::DiffLine::Kind;
            const bool                dark    = darkTheme(palette);
            const auto                added   = dark ? std::string{"#a6e3a1"} : std::string{"#2f7d32"};
            const auto                removed = dark ? std::string{"#f38ba8"} : std::string{"#c62828"};
            const auto                faded   = dark ? std::string{"#7d8799"} : std::string{"#6a737d"};
            std::vector<RowList::Row> rows;
            for (const auto& change : changes) {
                if (!change.changes())
                    continue;
                const auto what = change.current ? "changed" : "a new file";
                rows.push_back({{"", std::format("<b>{}</b>   {}", escapeMarkup(shortPath(change.file)), what)}, true});
                for (const auto& line : configuration::changesOnly(configuration::diffLines(change.current.value_or(""), change.proposed))) {
                    if (line.kind == Kind::same && line.line == 0) {
                        rows.push_back({{"", coloured("…", faded)}, false});
                        continue;
                    }
                    const auto& colour = line.kind == Kind::added ? added : line.kind == Kind::removed ? removed : faded;
                    const auto* sign   = line.kind == Kind::added ? "+" : line.kind == Kind::removed ? "−" : "";
                    const auto  text   = escapeMarkup(line.text.empty() ? std::string{" "} : line.text);
                    rows.push_back({{coloured(sign, colour), line.kind == Kind::same ? text : coloured(text, colour)}, false});
                }
            }
            return rows;
        }

        // Three cards to a row, with the gaps between them.
        constexpr double cardsPerRow = 3;
        constexpr double cardShare   = 0.32;

        const desktop::ShapeInfo& shapeInfo(desktop::Shape shape) {
            for (const auto& info : desktop::waybarShapes())
                if (info.shape == shape)
                    return info;
            return desktop::waybarShapes().front();
        }

        std::string recordName(const std::filesystem::path& record) {
            return shortPath(record);
        }

    }

    BarPage::BarPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop) :
        palette{std::move(palette)},
        defer{std::move(defer)},
        loop{std::move(loop)},
        diff{this->palette, this->defer,
             {RowList::Column{.width = 24, .monospace = true, .secondary = false, .markup = true, .heading = ""},
              RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true, .heading = ""}},
             20} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Bar", this->palette));
        root->addChild(paragraph("The studio has no bar of its own. It recommends Waybar, the bar most Hyprland desktops use, and helps you set it up. "
                                 "It never installs anything: when something is missing, it shows the command for you to run.",
                                 this->palette));

        // On and Off, for Waybar as it runs now. Turning it off does not change what starts at login.
        switchArea  = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        switchState = Hyprtoolkit::CTextBuilder::begin()
                          ->text(" ")
                          ->color([palette = this->palette] { return palette->m_colors.text; })
                          ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                          ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                          ->commence();
        switchState->setGrow(true, false);
        onButton  = button("On", 70, false, [this] { this->defer([this] { if (waybar) waybar->turnOn(); }); });
        offButton = button("Off", 70, false, [this] { this->defer([this] { if (waybar) waybar->turnOff(); }); });

        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(switchArea);
        root->addChild(body);

        const auto environment = foundation::currentEnvironment();
        settings               = application::loadSettings(application::studioFolder(environment));
        selected               = desktop::currentWaybarStyle(environment).value_or(desktop::WaybarStyle{desktop::Shape::islands, &desktop::waybarPalettes().front()});
        check();
    }

    void BarPage::close() {
        waybar.reset();
    }

    void BarPage::check() {
        const auto environment = foundation::currentEnvironment();
        const auto program     = desktop::findProgram("waybar", foundation::variable(environment, "PATH").value_or(""));
        if (program != waybarProgram) {
            waybarProgram = program;
            waybar.reset();
            if (program)
                waybar.emplace(*program, loop, [this] { programChanged(); });
        } else if (waybar) {
            waybar->refresh();
        }
        updateSwitch();
        if (!reviewing)
            showOverview();
    }

    void BarPage::programChanged() {
        // A change was just written and Waybar restarted with it: keep it if Waybar runs,
        // put the files back if it does not.
        if (checking && waybar && !waybar->busy()) {
            const auto record = *checking;
            checking.reset();
            if (waybar->running()) {
                outcome = "Written, and Waybar restarted with it.";
            } else if (const auto undone = configuration::undoChange(record); undone) {
                outcome = "Waybar did not start with the new files, so they were put back as they were.";
                waybar->turnOn();
            } else {
                outcome = std::format("Waybar did not start with the new files, and they could not be put back: {}", undone.error());
            }
            defer([this] { if (!reviewing) showOverview(); });
        }
        defer([this] { updateSwitch(); });
    }

    void BarPage::updateSwitch() {
        switchArea->clearChildren();
        if (!waybar)
            return;
        std::string state;
        if (waybar->busy())
            state = waybar->running() ? "stopping…" : "starting…";
        else
            state = waybar->running() ? "running" : "not running";
        switchState->rebuild()->text(std::format("<span weight=\"600\">Waybar</span>   {}", state))->commence();
        onButton->rebuild()->accent(waybar->running())->commence();
        offButton->rebuild()->accent(!waybar->running())->commence();

        const auto row = leftRow();
        row->addChild(switchState);
        row->addChild(onButton);
        row->addChild(offButton);
        switchArea->addChild(row);
        if (waybar->error())
            switchArea->addChild(paragraph(*waybar->error(), palette, true));
    }

    CSharedPointer<Hyprtoolkit::IElement> BarPage::card(const CSharedPointer<Hyprtoolkit::IElement>& preview, const std::string& name, bool chosen,
                                                       std::function<void()> chosenNow) {
        const auto made = Hyprtoolkit::CRectangleBuilder::begin()
                              ->color([palette = palette] { return palette->m_colors.alternateBase; })
                              ->rounding(palette->m_vars.bigRounding)
                              ->borderColor([palette = palette, chosen] { return chosen ? palette->m_colors.accent : Hyprtoolkit::CHyprColor{0, 0, 0, 0}; })
                              ->borderThickness(2)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {cardShare, 1.0}})
                              ->commence();
        const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        inside->setMargin(10);
        inside->addChild(preview);
        inside->addChild(markupLine(chosen ? std::format("<span weight=\"600\">{}</span>", escapeMarkup(name)) : escapeMarkup(name), palette));
        made->addChild(inside);
        made->setReceivesMouse(true);
        made->setMouseButton([this, chosenNow = std::move(chosenNow)](Hyprtoolkit::Input::eMouseButton button, bool pressed) {
            if (button == Hyprtoolkit::Input::MOUSE_BUTTON_LEFT && pressed)
                defer(chosenNow);
        });
        return made;
    }

    CSharedPointer<Hyprtoolkit::IElement> BarPage::styleSection() {
        const auto section = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        section->addChild(heading("Style", palette));
        const auto environment = foundation::currentEnvironment();

        // The chosen style at Waybar's own size, then what applying it would do.
        section->addChild(barPreview(selected, 1.0));
        const auto actions = leftRow();
        const auto planned = desktop::planWaybarStyle(selected, environment);
        if (!planned) {
            section->addChild(paragraph(std::format("The studio cannot write it: {}.", planned.error()), palette));
        } else if (std::ranges::none_of(*planned, [](const auto& change) { return change.changes(); })) {
            actions->addChild(markupLine(std::format("{}  Your Waybar uses {}, {}.", coloured("✓", hex(palette->m_colors.accent)), escapeMarkup(selected.palette->name),
                                                     escapeMarkup(std::string{shapeInfo(selected.shape).name})),
                                         palette));
        } else {
            actions->addChild(button("Review changes", 160, true, [this] { defer([this] { showReview(selected); }); }));
        }
        if (const auto latest = configuration::latestChange(application::backupsFolder(environment)))
            actions->addChild(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        section->addChild(actions);
        if (!outcome.empty())
            section->addChild(paragraph(outcome, palette));

        constexpr double cardScale = 0.42;
        section->addChild(space(4));
        section->addChild(markupLine("<span weight=\"600\">Shape</span>", palette));
        const auto shapes = leftRow(0);
        shapes->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
        for (const auto& info : desktop::waybarShapes()) {
            const desktop::WaybarStyle style{info.shape, selected.palette};
            shapes->addChild(card(barPreview(style, cardScale), std::string{info.name}, info.shape == selected.shape, [this, style] {
                selected = style;
                showOverview();
            }));
        }
        section->addChild(shapes);

        section->addChild(markupLine("<span weight=\"600\">Colours</span>", palette));
        const auto all     = desktop::waybarPalettes();
        constexpr auto perRow = static_cast<std::size_t>(cardsPerRow);
        for (std::size_t first = 0; first < all.size(); first += perRow) {
            const auto colours = leftRow(0);
            colours->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
            for (std::size_t index = first; index < std::min(all.size(), first + perRow); ++index) {
                const desktop::WaybarStyle style{selected.shape, &all[index]};
                colours->addChild(card(barPreview(style, cardScale), std::string{all[index].name}, &all[index] == selected.palette, [this, style] {
                    selected = style;
                    showOverview();
                }));
            }
            section->addChild(colours);
        }
        return section;
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
                        if (const auto saved = application::saveSettings(settings, application::studioFolder(foundation::currentEnvironment())); !saved)
                            outcome = std::format("The studio could not save its settings: {}.", saved.error());
                        showOverview();
                    });
                }));
            body->addChild(actions);
            body->addChild(space(8));
        }
        body->addChild(styleSection());
    }

    void BarPage::showReview(const desktop::WaybarStyle& style) {
        auto planned = desktop::planWaybarStyle(style, foundation::currentEnvironment());
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
        const bool off = waybar && !waybar->running();
        body->addChild(paragraph(std::format("Applying writes these files. A copy of each file it replaces is kept first, so it can be undone. {}",
                                             off ? "Waybar is off, so it will not be restarted." : "Waybar then restarts to read them."),
                                 palette, true));
        const auto actions = leftRow();
        actions->addChild(button("Apply", 110, true, [this] { defer([this] { apply(); }); }));
        actions->addChild(button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }));
        body->addChild(actions);
        diff.setRows(diffRows(changes, *palette));
        body->addChild(diff.element());
    }

    void BarPage::apply() {
        if (!reviewing)
            return;
        const auto record = configuration::applyChanges(changes, application::backupsFolder(foundation::currentEnvironment()), std::format("Waybar style: {}, {}", reviewing->palette->name, shapeInfo(reviewing->shape).name));
        if (!record) {
            outcome = std::format("Nothing was written: {}.", record.error());
        } else if (waybar && waybar->running()) {
            outcome  = "Written. Waybar is restarting…";
            checking = *record;
            waybar->restart();
        } else {
            outcome = "Written. Waybar is off, so it was not restarted; it reads the new files when it starts.";
        }
        showOverview();
    }

    void BarPage::undo() {
        const auto latest = configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()));
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone) {
            outcome = std::format("Nothing was undone: {}.", undone.error());
        } else if (waybar && waybar->running()) {
            outcome = "Undone, and Waybar is restarting to read the files as they were.";
            waybar->restart();
        } else {
            outcome = "Undone.";
        }
        showOverview();
    }

}
