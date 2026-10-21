// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/console-page.hpp"

#include "ipc/request.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"
#include "ui/widgets.hpp"

#include <hyprtoolkit/element/RowLayout.hpp>

#include <algorithm>
#include <format>
#include <ranges>
#include <sstream>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr double rowHeight     = 28;
        constexpr double nameWidth     = 160;
        constexpr double requestsWidth = 480;
        constexpr double typedWidth    = 420;

    }

    ConsolePage::ConsolePage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer) :
        palette{std::move(palette)},
        defer{defer},
        requests{this->palette, defer,
                 {RowList::Column{.width = nameWidth, .monospace = true, .secondary = false, .markup = false, .heading = "Request"},
                  RowList::Column{.width = 0, .monospace = false, .secondary = true, .markup = false, .heading = "What it tells"}},
                 rowHeight},
        reply{this->palette, defer, {RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = false, .heading = ""}}, 24} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Console", this->palette));
        root->addChild(paragraph("Ask Hyprland what it knows. Pick a request from the list or type it as you would for hyprctl. The console sends only requests "
                                 "that read, so nothing here changes your desktop.",
                                 this->palette));

        const auto toolbar = leftRow();
        typed              = Hyprtoolkit::CTextboxBuilder::begin()
                    ->placeholder("monitors, or getoption general:gaps_in")
                    ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {scaled(typedWidth), scaled(34)}})
                    ->commence();
        toolbar->addChild(typed);
        toolbar->addChild(space(1));
        toolbar->addChild(button("Send", 100, true, [this] { this->defer([this] { send(std::string{typed->currentText()}); }); }));
        root->addChild(toolbar);

        status = paragraph(" ", this->palette, true);
        root->addChild(status);

        const auto body = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(16)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        const auto left = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {scaled(requestsWidth), 1.0}})->commence();
        left->addChild(requests.element());
        const auto right = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        // Takes what the list leaves, like the studio's panel beside its sidebar.
        right->setGrow(true, false);
        right->addChild(reply.element());
        body->addChild(left);
        body->addChild(right);
        root->addChild(body);

        requests.setClicked([this](std::size_t index) {
            const auto all = ipc::consoleRequests();
            if (index >= all.size())
                return;
            // getoption needs an option's name, so it is filled in for editing, not sent.
            const std::string name = all[index].name == "getoption" ? "getoption general:gaps_in" : std::string{all[index].name};
            this->defer([this, name] {
                typed->setText(name);
                send(name);
            });
        });

        showRequests();
        showReply();
    }

    void ConsolePage::onAsk(Ask ask) {
        this->ask = std::move(ask);
    }

    void ConsolePage::restyle() {
        showRequests();
        showReply();
    }

    void ConsolePage::send(std::string text) {
        asked = text;
        if (const auto command = ipc::consoleCommand(text); !command || !ask) {
            answer = std::unexpected{command ? std::string{"Not connected to Hyprland yet."} : command.error()};
            showReply();
            return;
        }
        status->setText(escapeMarkup(std::format("Asking Hyprland: {}", text)));
        const auto number = ++sent;
        ask(std::move(text), [this, number](const std::expected<std::string, std::string>& came) {
            if (number != sent)
                return;
            answer = came ? std::expected<std::string, std::string>{ipc::indentedReply(*came)} : std::unexpected{std::format("No reply: {}.", came.error())};
            defer([this] { showReply(); });
        });
    }

    void ConsolePage::showRequests() {
        std::vector<RowList::Row> rows;
        for (const auto& request : ipc::consoleRequests())
            rows.push_back(RowList::Row{.cells = {std::string{request.name}, std::string{request.tells}}, .marked = false});
        requests.setRows(std::move(rows));
    }

    void ConsolePage::showReply() {
        std::vector<RowList::Row> rows;
        if (asked.empty()) {
            status->setText(escapeMarkup("Nothing asked yet."));
        } else if (!answer) {
            status->setText(escapeMarkup(answer.error()));
        } else {
            std::istringstream lines{*answer};
            for (std::string line; std::getline(lines, line);)
                rows.push_back(RowList::Row{.cells = {line}, .marked = false});
            status->setText(escapeMarkup(std::format("Hyprland's reply to {}: {} {}.", asked, rows.size(), rows.size() == 1 ? "line" : "lines")));
        }
        reply.setRows(std::move(rows));
    }

}
