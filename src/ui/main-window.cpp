// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/main-window.hpp"

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/window/Window.hpp>

#include <array>
#include <string_view>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        struct Page {
            std::string_view title;
            std::string_view summary;
        };

        // The pages planned for 0.1. Each says what it will do until it does it.
        constexpr std::array pages{
            Page{"Keybindings", "Every binding Hyprland has, where it comes from, and which ones collide."},
            Page{"Options", "Every option with its description, default and current value."},
            Page{"Live tree", "Monitors, workspaces and windows as they change, from Hyprland's event socket."},
            Page{"Event log", "What Hyprland reports as it happens."},
            Page{"Console", "Read-only requests to Hyprland, and their replies."},
            Page{"Activate", "Try the studio's effects on the running session, and put everything back."},
        };

        constexpr auto notBuiltYet = "Not built yet.";

    }

    int run() {
        const auto backend = Hyprtoolkit::IBackend::create();
        if (!backend)
            return 1;

        const auto palette    = backend->getPalette();
        const auto background = [palette] { return palette->m_colors.background; };
        const auto base       = [palette] { return palette->m_colors.base; };
        const auto text       = [palette] { return palette->m_colors.text; };

        const auto window = Hyprtoolkit::CWindowBuilder::begin()
                                ->appTitle("Hyprland Studio")
                                ->appClass("hyprland-studio")
                                ->preferredSize({960, 600})
                                ->minSize({640, 400})
                                ->resizable(true)
                                ->commence();

        const auto root = Hyprtoolkit::CRectangleBuilder::begin()
                              ->color(background)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                              ->commence();

        const auto body = Hyprtoolkit::CRowLayoutBuilder::begin()
                              ->gap(12)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                              ->commence();
        body->setMargin(16);

        const auto sidebar = Hyprtoolkit::CColumnLayoutBuilder::begin()
                                 ->gap(4)
                                 ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {200, 1}})
                                 ->commence();

        const auto content = Hyprtoolkit::CRectangleBuilder::begin()
                                 ->color(base)
                                 ->rounding(palette->m_vars.bigRounding)
                                 ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                                 ->commence();
        // Growing only adds the space left over, so the base width stays minimal; sized to its
        // text instead, the panel would push past the window's right edge.
        content->setGrow(true, false);

        const auto contentColumn = Hyprtoolkit::CColumnLayoutBuilder::begin()
                                       ->gap(8)
                                       ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                                       ->commence();
        contentColumn->setMargin(20);

        const auto pageTitle   = Hyprtoolkit::CTextBuilder::begin()->text(std::string{pages.front().title})->fontSize(CFontSize{CFontSize::HT_FONT_H1})->color(text)->commence();
        const auto pageSummary = Hyprtoolkit::CTextBuilder::begin()->text(std::string{pages.front().summary})->noEllipsize(true)->color(text)->commence();
        const auto pageStatus  = Hyprtoolkit::CTextBuilder::begin()->text(notBuiltYet)->fontSize(CFontSize{CFontSize::HT_FONT_SMALL})->color(text)->a(0.6F)->commence();

        contentColumn->addChild(pageTitle);
        contentColumn->addChild(pageSummary);
        contentColumn->addChild(pageStatus);
        content->addChild(contentColumn);

        sidebar->addChild(Hyprtoolkit::CTextBuilder::begin()->text("Hyprland Studio")->fontSize(CFontSize{CFontSize::HT_FONT_H2})->color(text)->commence());

        for (const auto& page : pages) {
            sidebar->addChild(Hyprtoolkit::CButtonBuilder::begin()
                                  ->label(std::string{page.title})
                                  ->alignText(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 32}})
                                  ->onMainClick([page, pageTitle, pageSummary](CSharedPointer<Hyprtoolkit::CButtonElement>) {
                                      pageTitle->setText(std::string{page.title});
                                      pageSummary->setText(std::string{page.summary});
                                  })
                                  ->commence());
        }

        body->addChild(sidebar);
        body->addChild(content);
        root->addChild(body);
        window->m_rootElement = root;

        window->m_events.closeRequest.listenStatic([window, backend] {
            window->close();
            backend->destroy();
        });

        window->open();
        backend->enterLoop();
        return 0;
    }

}
