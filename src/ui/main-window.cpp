// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/main-window.hpp"

#include "application/session.hpp"
#include "ui/bar-page.hpp"
#include "ui/keybindings-page.hpp"
#include "ui/texts.hpp"

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/window/Window.hpp>
#include <hyprutils/cli/Logger.hpp>

#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

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

        CSharedPointer<Hyprtoolkit::IElement> plannedPage(const Page& page, const CSharedPointer<Hyprtoolkit::CPalette>& palette) {
            const auto column = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();
            column->setMargin(28);
            column->addChild(pageTitle(std::string{page.title}, palette));
            column->addChild(paragraph(std::string{page.summary}, palette));
            column->addChild(paragraph("Not built yet.", palette, true));
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

        // The sidebar's buttons, each with its page; the open page's button is in the accent.
        std::vector<std::pair<CSharedPointer<Hyprtoolkit::CButtonElement>, CSharedPointer<Hyprtoolkit::IElement>>> pages;
        const auto show = [content, &pages](const CSharedPointer<Hyprtoolkit::IElement>& page) {
            content->clearChildren();
            content->addChild(page);
            for (const auto& [button, shown] : pages)
                button->rebuild()->accent(shown == page)->commence();
        };

        const auto defer = [backend](std::function<void()> function) { backend->addIdle(function); };
        const application::EventLoop loop{
            .watch   = [backend](int descriptor, std::function<void()> readable) { backend->addFd(descriptor, std::move(readable)); },
            .unwatch = [backend](int descriptor) { backend->removeFd(descriptor); },
            .after =
                [backend](std::chrono::milliseconds delay, std::function<void()> callback) {
                    backend->addTimer(delay, [callback = std::move(callback)](auto, void*) { callback(); }, nullptr);
                },
        };
        KeybindingsPage keybindings{palette, defer};
        BarPage         bar{palette, defer, loop};

        const auto addPageButton = [&sidebar, &show, &pages](std::string_view title, CSharedPointer<Hyprtoolkit::IElement> page) {
            const auto button = Hyprtoolkit::CButtonBuilder::begin()
                                    ->label(std::string{title})
                                    ->noBorder(true)
                                    ->noBg(true)
                                    ->alignText(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                                    ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 34}})
                                    ->onMainClick([&show, page](CSharedPointer<Hyprtoolkit::CButtonElement>) { show(page); })
                                    ->commence();
            sidebar->addChild(button);
            pages.emplace_back(button, std::move(page));
        };

        sidebar->addChild(Hyprtoolkit::CTextBuilder::begin()
                              ->text("<span weight=\"600\">Hyprland Studio</span>")
                              ->fontSize(CFontSize{CFontSize::HT_FONT_H2})
                              ->color([palette] { return palette->m_colors.text; })
                              ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                              ->commence());
        sidebar->addChild(Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 12}})->commence());
        addPageButton("Keybindings", keybindings.element());
        addPageButton("Bar", bar.element());
        for (const auto& page : plannedPages)
            addPageButton(page.title, plannedPage(page, palette));
        show(keybindings.element());

        body->addChild(sidebar);
        body->addChild(content);
        root->addChild(body);
        window->m_rootElement = root;

        std::optional<application::Session> session;
        session.emplace(
            loop,
            application::Listeners{
                .connection = [&keybindings](application::Connection connection, const std::string& reason) { keybindings.showConnection(connection, reason); },
                .keybindings = [&keybindings](const auto& result) { keybindings.showKeybindings(result); },
            });

        // The session and the Bar page go first: they unwatch descriptors, which needs a live event loop.
        window->m_events.closeRequest.listenStatic([&session, &bar, window, backend] {
            session.reset();
            bar.close();
            window->close();
            backend->destroy();
        });

        window->open();
        session->start();
        backend->enterLoop();
        return 0;
    }

}
