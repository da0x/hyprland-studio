// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/main-window.hpp"

#include "application/session.hpp"
#include "ui/keybindings-page.hpp"

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprutils/cli/Logger.hpp>

#include <array>
#include <optional>
#include <string_view>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;
        using Hyprutils::Memory::makeShared;

        struct Page {
            std::string_view title;
            std::string_view summary;
        };

        // The pages planned for 0.1 that are not built yet. Each says what it will do until it does it.
        constexpr std::array plannedPages{
            Page{"Options", "Every option with its description, default and current value."},
            Page{"Live tree", "Monitors, workspaces and windows as they change, from Hyprland's event socket."},
            Page{"Event log", "What Hyprland reports as it happens."},
            Page{"Console", "Read-only requests to Hyprland, and their replies."},
            Page{"Activate", "Try the studio's effects on the running session, and put everything back."},
        };

        CSharedPointer<Hyprtoolkit::IElement> plannedPage(const Page& page, const Hyprtoolkit::colorFn& text) {
            const auto column = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
            column->setMargin(20);
            column->addChild(Hyprtoolkit::CTextBuilder::begin()->text(std::string{page.title})->fontSize(CFontSize{CFontSize::HT_FONT_H1})->color(Hyprtoolkit::colorFn{text})->commence());
            column->addChild(Hyprtoolkit::CTextBuilder::begin()->text(std::string{page.summary})->noEllipsize(true)->color(Hyprtoolkit::colorFn{text})->commence());
            column->addChild(Hyprtoolkit::CTextBuilder::begin()->text("Not built yet.")->fontSize(CFontSize{CFontSize::HT_FONT_SMALL})->color(Hyprtoolkit::colorFn{text})->a(0.6F)->commence());
            return column;
        }

    }

    int run(const Options& options) {
        // hyprtoolkit logs every step at debug level. Only warnings and errors, unless asked.
        Hyprutils::CLI::CLogger logger;
        logger.setLogLevel(options.verbose ? Hyprutils::CLI::LOG_DEBUG : Hyprutils::CLI::LOG_WARN);
        Hyprtoolkit::IBackend::SBackendCreationData creation;
        creation.pLogConnection = makeShared<Hyprutils::CLI::CLoggerConnection>(logger);

        const auto backend = Hyprtoolkit::IBackend::createWithData(creation);
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

        const auto show = [content](const CSharedPointer<Hyprtoolkit::IElement>& page) {
            content->clearChildren();
            content->addChild(page);
        };

        KeybindingsPage keybindings{palette, [backend](std::function<void()> function) { backend->addIdle(function); }};
        show(keybindings.element());

        const auto addPageButton = [&sidebar, &show](std::string_view title, CSharedPointer<Hyprtoolkit::IElement> page) {
            sidebar->addChild(Hyprtoolkit::CButtonBuilder::begin()
                                  ->label(std::string{title})
                                  ->alignText(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 32}})
                                  ->onMainClick([show, page = std::move(page)](CSharedPointer<Hyprtoolkit::CButtonElement>) { show(page); })
                                  ->commence());
        };

        sidebar->addChild(Hyprtoolkit::CTextBuilder::begin()->text("Hyprland Studio")->fontSize(CFontSize{CFontSize::HT_FONT_H2})->color(text)->commence());
        addPageButton("Keybindings", keybindings.element());
        for (const auto& page : plannedPages)
            addPageButton(page.title, plannedPage(page, text));

        body->addChild(sidebar);
        body->addChild(content);
        root->addChild(body);
        window->m_rootElement = root;

        std::optional<application::Session> session;
        session.emplace(
            application::EventLoop{
                .watch   = [backend](int descriptor, std::function<void()> readable) { backend->addFd(descriptor, std::move(readable)); },
                .unwatch = [backend](int descriptor) { backend->removeFd(descriptor); },
                .after =
                    [backend](std::chrono::milliseconds delay, std::function<void()> callback) {
                        backend->addTimer(delay, [callback = std::move(callback)](auto, void*) { callback(); }, nullptr);
                    },
            },
            application::Listeners{
                .connection = [&keybindings](application::Connection connection, const std::string& reason) { keybindings.showConnection(connection, reason); },
                .keybindings = [&keybindings](const auto& result) { keybindings.showKeybindings(result); },
            });

        // The session goes first: it unwatches its sockets, which needs a live event loop.
        window->m_events.closeRequest.listenStatic([&session, window, backend] {
            session.reset();
            window->close();
            backend->destroy();
        });

        window->open();
        session->start();
        backend->enterLoop();
        return 0;
    }

}
