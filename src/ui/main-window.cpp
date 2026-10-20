// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/main-window.hpp"

#include "application/session.hpp"
#include "ui/bar-page.hpp"
#include "ui/keybindings-page.hpp"
#include "ui/restyle.hpp"
#include "ui/terminal-page.hpp"
#include "ui/accessibility-page.hpp"
#include "ui/widgets.hpp"
#include "ui/theme-page.hpp"
#include "ui/wallpaper-page.hpp"
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

#include <algorithm>
#include <array>
#include <functional>
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
            Page{"Activate", "Try the studio's effects on the running session, and put everything back."},
            Page{"Live tree", "Monitors, workspaces and windows as they change, from Hyprland's event socket."},
            Page{"Event log", "What Hyprland reports as it happens."},
            Page{"Console", "Read-only requests to Hyprland, and their replies."},
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

    namespace {

        // The sidebar grows with the text, but by less, so its longest name still fits and
        // the page keeps most of the width.
        double sidebarWidth() {
            constexpr double designed = 200;
            return designed + (scaled(designed) - designed) * 0.6;
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
        // The studio's colours: hyprtoolkit's, from hyprtoolkit.conf or its defaults, until
        // the Theme page sets new ones here and every element is made again in them.
        const auto background = [palette] { return palette->m_colors.background; };
        const auto base       = [palette] { return palette->m_colors.base; };

        const auto window = Hyprtoolkit::CWindowBuilder::begin()
                                ->appTitle("Hyprland Studio")
                                ->appClass("hyprland-studio")
                                // Larger with larger text, so a page still fits.
                                ->preferredSize({scaled(960), scaled(600)})
                                ->minSize({scaled(640), scaled(400)})
                                ->resizable(true)
                                ->commence();

        const auto root = Hyprtoolkit::CRectangleBuilder::begin()
                              ->color(background)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                              ->commence();

        restyled(root);

        const auto body = Hyprtoolkit::CRowLayoutBuilder::begin()
                              ->gap(12)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                              ->commence();
        body->setMargin(16);

        const auto sidebar = Hyprtoolkit::CColumnLayoutBuilder::begin()
                                 ->gap(4)
                                 ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {sidebarWidth(), 1.0}})
                                 ->commence();

        const auto content = Hyprtoolkit::CRectangleBuilder::begin()
                                 ->color(base)
                                 ->rounding(palette->m_vars.bigRounding)
                                 ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                                 ->commence();
        // Growing only adds the space left over, so the base width stays minimal; sized to its
        // text instead, the panel would push past the window's right edge.
        content->setGrow(true, false);
        restyled(content);

        // The sidebar's buttons, each with its page; the open page's button is in the accent.
        std::vector<std::pair<CSharedPointer<Hyprtoolkit::CButtonElement>, CSharedPointer<Hyprtoolkit::IElement>>> pages;
        std::vector<std::pair<std::string, CSharedPointer<Hyprtoolkit::IElement>>>                                titles;
        // Told which page opens, for a page that has to tidy up when it is left.
        std::function<void(const CSharedPointer<Hyprtoolkit::IElement>&)> opening;
        const auto show = [content, &pages, &opening](const CSharedPointer<Hyprtoolkit::IElement>& page) {
            if (opening)
                opening(page);
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
        WallpaperPage   wallpaper{palette, defer, loop};
        ThemePage       theme{palette, defer, loop};
        TerminalPage    terminal{palette, defer};
        AccessibilityPage accessibility{palette, defer, loop};
        // A wallpaper tried on the desktop and not kept goes back when its page is left.
        opening = [&wallpaper](const CSharedPointer<Hyprtoolkit::IElement>& page) {
            if (page != wallpaper.element())
                wallpaper.leave();
        };

        const auto addPageButton = [&sidebar, &show, &pages, &titles](std::string_view title, CSharedPointer<Hyprtoolkit::IElement> page) {
            titles.emplace_back(std::string{title}, page);
            const auto button = Hyprtoolkit::CButtonBuilder::begin()
                                    ->label(std::string{title})
                                    ->noBorder(true)
                                    ->noBg(true)
                                    ->alignText(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                                    ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, scaled(34)}})
                                    ->onMainClick([&show, page](CSharedPointer<Hyprtoolkit::CButtonElement>) { show(page); })
                                    ->commence();
            restyled(button);
            sidebar->addChild(button);
            pages.emplace_back(button, std::move(page));
        };

        const auto studioName = Hyprtoolkit::CTextBuilder::begin()
                              ->text("<span weight=\"600\">Hyprland Studio</span>")
                              ->fontSize(CFontSize{CFontSize::HT_FONT_H2})
                              ->color([palette] { return palette->m_colors.text; })
                              ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                              ->commence();
        restyled(studioName, CFontSize{CFontSize::HT_FONT_H2});
        // The sidebar and its buttons grow with the text, so the pages' names still fit.
        const auto resizeSidebar = [sidebar, &pages] {
            sidebar->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {sidebarWidth(), 1.0}})->commence();
            for (const auto& [button, page] : pages)
                button->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, scaled(34)}})->commence();
        };
        sidebar->addChild(studioName);
        sidebar->addChild(Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 12}})->commence());
        // Most used first: the look of the desktop, then the terminal, then what is read
        // more than changed.
        addPageButton("Theme", theme.element());
        addPageButton("Wallpaper", wallpaper.element());
        addPageButton("Bar", bar.element());
        addPageButton("Terminal", terminal.element());
        addPageButton("Accessibility", accessibility.element());
        addPageButton("Keyboard shortcuts", keybindings.element());
        for (const auto& page : plannedPages)
            addPageButton(page.title, plannedPage(page, palette));
        const auto first = std::ranges::find(titles, options.page, &std::pair<std::string, CSharedPointer<Hyprtoolkit::IElement>>::first);
        show(first != titles.end() ? first->second : theme.element());

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

        // The session and the setup pages go first: they unwatch descriptors, which needs a live event loop.
        const auto quit = [&session, &bar, &wallpaper, &theme, window, backend] {
            session.reset();
            bar.close();
            wallpaper.leave(true);
            wallpaper.close();
            theme.close();
            window->close();
            backend->destroy();
        };
        window->m_events.closeRequest.listenStatic(quit);
        // Every element made in the old colours is made again: the long-lived ones in place,
        // and the pages' contents, some with colours baked into their text, from their data.
        const auto redraw = [&keybindings, &bar, &wallpaper, &theme, &terminal, &accessibility](bool resized) {
            // The pages first, so the elements they let go of are not drawn again in between;
            // what they make is already right, so only what was there before is rebuilt.
            // The Theme page draws itself again after its own change; a new text size is another's.
            const auto before = restyleCount();
            if (resized)
                theme.restyle();
            keybindings.restyle();
            terminal.restyle();
            bar.restyle();
            wallpaper.restyle();
            accessibility.restyle();
            restyleAll(resized, before);
        };
        theme.onRecoloured([redraw] { redraw(false); });
        // The terminal's themes follow the contrast and colour vision; the studio's text, the size.
        accessibility.onApplied([&terminal, palette, redraw, defer, window, loop, resizeSidebar](const desktop::Accessibility& now) {
            terminal.adjust(now.contrast, now.vision);
            auto&      sizes  = palette->m_vars;
            const auto scaled = desktop::toolkitTextSizes(now.textScale);
            const auto size   = [&scaled](std::string_view key) { return std::stoi(std::ranges::find(scaled, key, &std::pair<std::string, std::string>::first)->second); };
            const std::array current{sizes.h1Size, sizes.h2Size, sizes.h3Size, sizes.fontSize, sizes.smallFontSize};
            sizes.h1Size        = size("h1_size");
            sizes.h2Size        = size("h2_size");
            sizes.h3Size        = size("h3_size");
            sizes.fontSize      = size("font_size");
            sizes.smallFontSize = size("small_font_size");
            if (current == std::array{sizes.h1Size, sizes.h2Size, sizes.h3Size, sizes.fontSize, sizes.smallFontSize})
                return;
            // The window grows with the text, so the page still fits: a page laid out taller
            // than its window comes out wrong. It grows first, with the sidebar, whose new
            // width is laid out with the window's new size, and the text follows once the
            // compositor has had time to apply it; the compositor has the last word. With
            // smaller text the window keeps its size, and the sidebar its width until the
            // window is next resized.
            if (sizes.fontSize > current[3]) {
                resizeSidebar();
                if (const auto scale = window->scale(); scale > 0)
                    window->setSize(window->pixelSize() / scale * (static_cast<double>(sizes.fontSize) / current[3]));
                loop.after(std::chrono::milliseconds{200}, [redraw] { redraw(true); });
            } else {
                defer([redraw, resizeSidebar] {
                    redraw(true);
                    resizeSidebar();
                });
            }
        });

        window->open();
        session->start();
        backend->enterLoop();
        return 0;
    }

}
