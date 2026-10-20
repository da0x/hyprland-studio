// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/terminal-page.hpp"

#include "application/studio-folder.hpp"
#include "desktop/process.hpp"
#include "desktop/system.hpp"
#include "foundation/environment.hpp"
#include "ui/bar-preview.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"
#include "ui/widgets.hpp"

#include <algorithm>
#include <csignal>
#include <format>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr auto        changeName = "Terminal";
        constexpr std::size_t perRow     = 4;
        constexpr std::size_t perPage    = 2 * perRow;
        constexpr double      cardShare  = 0.24;

    }

    TerminalPage::TerminalPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer) :
        palette{std::move(palette)},
        defer{std::move(defer)},
        diff{this->palette, this->defer,
             {RowList::Column{.width = 24, .monospace = true, .secondary = false, .markup = true, .heading = ""},
              RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true, .heading = ""}},
             20} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Terminal", this->palette));
        root->addChild(paragraph("Colour schemes for kitty, chosen apart from the desktop's: terminal themes are their own family, and many are best dark "
                                 "whatever the desktop is. They come from kitty's own collection, kitty-themes. Nothing is written until you review the change.",
                                 this->palette));
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(body);
        footer = leftRow();
        root->addChild(footer);

        selected = desktop::currentTerminal(foundation::currentEnvironment());
        if (selected == nullptr)
            selected = &desktop::terminalThemes().front();
        lightShown = !selected->dark;
        const auto shown = shownThemes();
        if (const auto at = std::ranges::find(shown, selected); at != shown.end())
            pageStart = static_cast<std::size_t>(at - shown.begin()) / perPage * perPage;
        showOverview();
    }

    void TerminalPage::restyle() {
        if (!reviewing)
            showOverview();
    }

    std::vector<const desktop::TerminalTheme*> TerminalPage::shownThemes() const {
        std::vector<const desktop::TerminalTheme*> shown;
        for (const auto& theme : desktop::terminalThemes())
            if (theme.dark != lightShown)
                shown.push_back(&theme);
        return shown;
    }

    CSharedPointer<Hyprtoolkit::IElement> TerminalPage::sessionOf(const desktop::TerminalTheme* theme, bool large) {
        auto& made = sessions[{theme, large}];
        if (made)
            return made;
        const auto& colour = theme->colours;
        const auto  frame  = Hyprtoolkit::CRectangleBuilder::begin()
                               ->color([c = paletteColour(theme->background)] { return c; })
                               ->rounding(large ? 10 : 6)
                               ->borderColor([c = paletteColour(colour[8])] { return c; })
                               ->borderThickness(1)
                               ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, large ? 200.0 : 82.0}})
                               ->commence();
        const auto lines = Hyprtoolkit::CColumnLayoutBuilder::begin()
                               ->gap(large ? 4 : 1)
                               ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})
                               ->commence();
        lines->setMargin(large ? 16 : 7);
        // A short session, coloured as a shell and the usual tools colour their output.
        const auto in     = [](const std::string& hex, std::string_view text) { return coloured(text, hex); };
        const auto prompt = std::format("{} {} {} ", in(colour[2], "da@studio"), in(colour[4], "~/hyprland-studio"), in(theme->foreground, "$"));
        const std::vector<std::string> session{
            prompt + in(theme->foreground, "ls"),
            std::format("{}  {}  {}  {}  {}", in(colour[4], "build"), in(colour[4], "docs"), in(colour[4], "src"), in(theme->foreground, "README.md"), in(colour[2], "configure")),
            std::format("{} {}", in(colour[3], "warning:"), in(theme->foreground, "unused variable ") + in(colour[5], "'scale'")),
            std::format("{} {}", in(colour[1], "error:"), in(theme->foreground, "expected ';' before ") + in(colour[6], "'}'")),
            in(colour[8], "# 81 tests, 80 passed"),
            prompt + in(theme->cursor, "█"),
        };
        for (std::size_t index = 0; index < session.size(); ++index) {
            if (!large && index == 4)
                continue;
            lines->addChild(Hyprtoolkit::CTextBuilder::begin()
                                ->text(std::format("<span font_family=\"monospace\">{}</span>", session[index]))
                                ->fontSize(CFontSize{CFontSize::HT_FONT_ABSOLUTE, large ? 11.F : 5.F})
                                ->color([c = paletteColour(theme->foreground)] { return c; })
                                ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                                ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                                ->commence());
        }
        frame->addChild(lines);
        made = frame;
        return made;
    }

    void TerminalPage::showOverview() {
        reviewing = false;
        changes.clear();
        body->clearChildren();
        statusShown.clear();
        const auto environment = foundation::currentEnvironment();

        if (!desktop::findProgram("kitty", foundation::variable(environment, "PATH").value_or(""))) {
            body->addChild(paragraph("The studio themes kitty, which is not installed. Install the kitty package, then open this page again.", palette));
            setFooter(footer, {});
            return;
        }

        // Dark or light, and which eight of them.
        const auto shown = shownThemes();
        const auto top   = leftRow();
        top->rebuild()->gap(4)->commence();
        top->addChild(button("Dark", 90, !lightShown, [this] { defer([this] { lightShown = false; pageStart = 0; showOverview(); }); }));
        top->addChild(button("Light", 90, lightShown, [this] { defer([this] { lightShown = true; pageStart = 0; showOverview(); }); }));
        const auto rest = space(1);
        rest->setGrow(true, false);
        top->addChild(rest);
        const auto count = markupLine(std::format("{}–{} of {}", pageStart + 1, std::min(shown.size(), pageStart + perPage), shown.size()), palette);
        count->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        top->addChild(count);
        if (pageStart > 0)
            top->addChild(button("‹ Previous", 110, false, [this] { defer([this] { pageStart -= std::min(pageStart, perPage); showOverview(); }); }));
        if (pageStart + perPage < shown.size())
            top->addChild(button("Next ›", 90, false, [this] { defer([this] { pageStart += perPage; showOverview(); }); }));
        body->addChild(top);

        largePreview = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        body->addChild(largePreview);

        cards.clear();
        for (std::size_t first = pageStart; first < std::min(shown.size(), pageStart + perPage); first += perRow) {
            const auto line = leftRow(0);
            line->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->gap(12)->commence();
            for (std::size_t index = first; index < std::min({shown.size(), first + perRow, pageStart + perPage}); ++index) {
                const auto* theme = shown[index];
                Card        made;
                made.frame = Hyprtoolkit::CRectangleBuilder::begin()
                                 ->color([palette = palette] { return palette->m_colors.alternateBase; })
                                 ->rounding(palette->m_vars.bigRounding)
                                 ->borderColor([] { return Hyprtoolkit::CHyprColor{0, 0, 0, 0}; })
                                 ->borderThickness(2)
                                 ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {cardShare, 1.0}})
                                 ->commence();
                const auto inside = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
                inside->setMargin(8);
                made.holder = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
                made.holder->addChild(sessionOf(theme, false));
                made.name = markupLine(escapeMarkup(theme->name), palette, CFontSize{CFontSize::HT_FONT_SMALL});
                inside->addChild(made.holder);
                inside->addChild(made.name);
                made.frame->addChild(inside);
                made.frame->setReceivesMouse(true);
                made.frame->setMouseButton([this, theme](Hyprtoolkit::Input::eMouseButton pressedButton, bool pressed) {
                    if (pressedButton != Hyprtoolkit::Input::MOUSE_BUTTON_LEFT || !pressed)
                        return;
                    defer([this, theme] {
                        selected = theme;
                        updateSelection();
                    });
                });
                line->addChild(made.frame);
                cards.emplace_back(theme, std::move(made));
            }
            body->addChild(line);
        }
        updateSelection();
    }

    void TerminalPage::updateSelection() {
        largePreview->clearChildren();
        largePreview->addChild(sessionOf(selected, true));
        fillStatus();
        for (auto& [theme, made] : cards) {
            const bool chosen = theme == selected;
            made.frame->rebuild()->borderColor([palette = palette, chosen] { return chosen ? palette->m_colors.accent : Hyprtoolkit::CHyprColor{0, 0, 0, 0}; })->commence();
            auto name = escapeMarkup(theme->name);
            if (chosen)
                name = std::format("<span weight=\"600\">{}</span>", name);
            if (theme == inUse)
                name += std::format("  {}", coloured("· in use", hex(palette->m_colors.accent)));
            made.name->rebuild()->text(std::string{name})->commence();
        }
    }

    void TerminalPage::fillStatus() {
        const auto environment = foundation::currentEnvironment();
        const auto planned     = desktop::planTerminal(*selected, environment);
        const bool undoable    = configuration::latestChange(application::backupsFolder(environment), changeName).has_value();
        inUse                  = desktop::currentTerminal(environment);
        const auto shown = std::format("{}|{}|{}|{}|{}", selected->name, planned ? planned->empty() : false, planned ? "" : planned.error(), undoable, outcome);
        if (shown == statusShown)
            return;
        statusShown = shown;

        // Said in the footer, so that saying it moves nothing above.
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> buttons;
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> away;
        if (undoable)
            away.push_back(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        std::string note;
        if (!planned) {
            note = escapeMarkup(std::format("The studio cannot apply it: {}.", planned.error()));
        } else if (planned->empty()) {
            note = std::format("{}  Your terminal uses {}.", coloured("✓", hex(palette->m_colors.accent)), escapeMarkup(selected->name));
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

    void TerminalPage::showReview() {
        auto planned = desktop::planTerminal(*selected, foundation::currentEnvironment());
        if (!planned) {
            outcome = planned.error();
            return showOverview();
        }
        reviewing = true;
        changes   = std::move(*planned);
        outcome.clear();
        body->clearChildren();
        body->addChild(space(4));
        body->addChild(heading(std::format("Review: {}, for the terminal", selected->name), palette));
        body->addChild(paragraph("Applying will:", palette));
        body->addChild(paragraph("•  Write the changes below. A copy of each file is kept first, so it can be undone.", palette, true));
        body->addChild(paragraph("•  Show the new colours in every open kitty window at once.", palette, true));
        setFooter(footer, {button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }), button("Apply", 110, true, [this] { defer([this] { apply(); }); })});
        diff.setRows(diffRows(changes, *palette));
        body->addChild(diff.element());
    }

    void TerminalPage::apply() {
        if (!reviewing)
            return;
        const auto record =
            configuration::applyChanges(changes, application::backupsFolder(foundation::currentEnvironment()), std::format("{}: {}", changeName, selected->name));
        if (!record) {
            outcome = std::format("Nothing was written: {}.", record.error());
            return showOverview();
        }
        // kitty reads kitty.conf, and the theme it includes, again on SIGUSR1.
        desktop::signalProcesses("kitty", SIGUSR1);
        outcome = std::format("Applied {}.", selected->name);
        showOverview();
    }

    void TerminalPage::undo() {
        const auto latest = configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), changeName);
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone) {
            outcome = std::format("Nothing was undone: {}.", undone.error());
        } else {
            desktop::signalProcesses("kitty", SIGUSR1);
            outcome = "Undone.";
        }
        showOverview();
    }

}
