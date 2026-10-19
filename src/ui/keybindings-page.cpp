// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/keybindings-page.hpp"

#include "ui/markup.hpp"
#include "ui/texts.hpp"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <format>
#include <string_view>
#include <utility>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr double rowHeight    = 30;
        constexpr double keysWidth    = 300;
        constexpr double notesWidth   = 240;
        constexpr double buttonHeight = 30;

        // Hyprland names mouse buttons by their Linux input codes.
        std::string keyName(const std::string& key) {
            constexpr std::array names{
                std::pair{std::string_view{"mouse:272"}, std::string_view{"left button"}},
                std::pair{std::string_view{"mouse:273"}, std::string_view{"right button"}},
                std::pair{std::string_view{"mouse:274"}, std::string_view{"middle button"}},
                std::pair{std::string_view{"mouse_down"}, std::string_view{"wheel down"}},
                std::pair{std::string_view{"mouse_up"}, std::string_view{"wheel up"}},
            };
            const auto found = std::ranges::find(names, std::string_view{key}, &std::pair<std::string_view, std::string_view>::first);
            return found != names.end() ? std::string{found->second} : key;
        }

        // What the binding does: its description in words when it has one, or else its
        // action as the Lua that would make it.
        std::string action(const application::KeybindingRow& row, const Hyprtoolkit::CPalette& palette) {
            if (!row.live.description.empty())
                return escapeMarkup(row.live.description);
            if (row.recorded)
                return row.recorded->description.empty() ? luaMarkup(row.recorded->action.lua(), palette) : escapeMarkup(row.recorded->description);
            // Under Lua, Hyprland reports no action at all, only an internal number.
            if (row.live.fromLua())
                return {};
            return escapeMarkup(row.live.argument.empty() ? row.live.dispatcher : std::format("{} {}", row.live.dispatcher, row.live.argument));
        }

        struct Flags {
            std::string_view submap;
            bool             release      = false;
            bool             repeat       = false;
            bool             longPress    = false;
            bool             locked       = false;
            bool             nonConsuming = false;
            bool             inactive     = false;
        };

        // Where the binding was set, in the link colour since clicking opens it, then
        // anything unusual about it.
        std::string notes(const std::optional<configuration::Location>& location, const Flags& flags, const Hyprtoolkit::CPalette& palette) {
            std::vector<std::string> extras;
            const auto               add = [&extras](bool set, std::string text) {
                if (set)
                    extras.push_back(std::move(text));
            };
            add(flags.inactive, "not active");
            add(!flags.submap.empty(), std::format("submap {}", flags.submap));
            add(flags.release, "on release");
            add(flags.repeat, "repeats");
            add(flags.longPress, "long press");
            add(flags.locked, "also when locked");
            add(flags.nonConsuming, "passes the key on");

            std::string text = location ? coloured(location->brief(), hex(palette.m_colors.linkText)) : std::string{};
            for (const auto& extra : extras)
                text += (text.empty() ? "" : "  ·  ") + escapeMarkup(extra);
            return text;
        }

        // The path the way a person would write it: under their home folder, starting with ~.
        std::string shortPath(const std::filesystem::path& path) {
            const char* home = std::getenv("HOME");
            if (home == nullptr || *home == '\0')
                return path.string();
            const auto relative = path.lexically_relative(home);
            if (relative.empty() || relative.native().starts_with(".."))
                return path.string();
            return (std::filesystem::path{"~"} / relative).string();
        }

        CSharedPointer<Hyprtoolkit::CButtonElement> button(std::string label, double width, std::function<void()> clicked) {
            return Hyprtoolkit::CButtonBuilder::begin()
                ->label(std::move(label))
                ->noBorder(true)
                ->size(CDynamicSize{width > 0 ? CDynamicSize::HT_SIZE_ABSOLUTE : CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_ABSOLUTE, {width > 0 ? width : 1.0, buttonHeight}})
                ->onMainClick([clicked = std::move(clicked)](CSharedPointer<Hyprtoolkit::CButtonElement>) { clicked(); })
                ->commence();
        }

    }

    KeybindingsPage::KeybindingsPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer) :
        palette{std::move(palette)},
        defer{defer},
        list{this->palette, defer,
             {RowList::Column{.width = keysWidth, .monospace = false, .secondary = false, .markup = true, .heading = "Keys"},
              RowList::Column{.width = 0, .monospace = false, .secondary = false, .markup = true, .heading = "Does"},
              RowList::Column{.width = notesWidth, .monospace = false, .secondary = true, .markup = true, .heading = "Set at"}},
             rowHeight},
        sourceView{this->palette, defer} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);

        root->addChild(pageTitle("Keybindings", this->palette));
        status = paragraph("Connecting to Hyprland.", this->palette);
        root->addChild(status);
        luaNote = paragraph({}, this->palette, true);
        root->addChild(luaNote);

        // A two-way switch, then one tab per config file while the source is showing.
        const auto toolbar = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, buttonHeight}})->commence();
        listButton         = button("Bindings", 100, [this] { showList(); });
        sourceButton       = button("Source", 100, [this] {
            if (!openFile && !sources.empty())
                showSource(sources.front().path, 0, true);
        });
        fileButtons = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, buttonHeight}})->commence();
        fileButtons->setGrow(true, false);
        toolbar->addChild(listButton);
        toolbar->addChild(sourceButton);
        toolbar->addChild(Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {20.0, 1.0}})->commence());
        toolbar->addChild(fileButtons);
        root->addChild(Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 8.0}})->commence());
        root->addChild(toolbar);

        caption = paragraph({}, this->palette, true);

        // Holds the list or the source; its height is what is left.
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 6.0}})->commence());
        root->addChild(body);

        list.setClicked([this](std::size_t row) {
            if (row < rowLocations.size() && rowLocations[row])
                showSource(rowLocations[row]->file, rowLocations[row]->line, true);
        });
        list.setRows({RowList::Row{.cells = {{}, escapeMarkup("Reading your keybindings."), {}}, .marked = false}});
        showList();
    }

    void KeybindingsPage::updateSwitch() {
        // Rebuilt rather than changed, the only way hyprtoolkit restyles a button.
        listButton->rebuild()->accent(!openFile)->commence();
        sourceButton->rebuild()->accent(openFile.has_value())->commence();
    }

    void KeybindingsPage::showList() {
        openFile.reset();
        markedLine = 0;
        body->clearChildren();
        body->addChild(list.element());
        updateSwitch();
        updateFileButtons();
    }

    const application::SourceFile* KeybindingsPage::source(const std::filesystem::path& file) const {
        const auto found = std::ranges::find(sources, file, &application::SourceFile::path);
        return found != sources.end() ? &*found : nullptr;
    }

    void KeybindingsPage::showSource(const std::filesystem::path& file, int line, bool jump) {
        const auto* shown = source(file);
        if (shown == nullptr)
            return;
        const bool wasList = !openFile;
        openFile           = file;
        markedLine         = line;
        sourceView.show(*shown, line, jump);
        caption->setText(escapeMarkup(line > 0 ? std::format("{}, line {}. As it is on disk now, which Hyprland may not have reloaded yet.", shortPath(file), line)
                                               : std::format("{}. As it is on disk now, which Hyprland may not have reloaded yet.", shortPath(file))));
        if (wasList) {
            body->clearChildren();
            body->addChild(caption);
            body->addChild(sourceView.element());
            updateSwitch();
        }
        updateFileButtons();
    }

    void KeybindingsPage::updateFileButtons() {
        fileButtons->clearChildren();
        if (!openFile || sources.size() < 2)
            return;
        const auto folder = sources.front().path.parent_path();
        for (const auto& file : sources) {
            auto tab = button(file.path.lexically_relative(folder).string(), 0, [this, path = file.path] {
                // Showing a file replaces this button, so not from inside its handler.
                defer([this, path] { showSource(path, 0, true); });
            });
            if (file.path == *openFile)
                tab->rebuild()->accent(true)->commence();
            fileButtons->addChild(tab);
        }
    }

    void KeybindingsPage::showConnection(application::Connection connection, const std::string& reason) {
        this->connection = connection;
        connectionReason = reason;
        updateStatus();
    }

    void KeybindingsPage::showKeybindings(const std::expected<application::Keybindings, std::string>& keybindings) {
        if (!keybindings) {
            bindingsError = keybindings.error();
            updateStatus();
            return;
        }
        bindingsError.clear();
        configurationProblem = keybindings->configurationProblem;
        configurationDoubts  = keybindings->configurationDoubts;
        count                = keybindings->rows.size();
        inactive             = keybindings->inactive.size();

        const auto unexplained = std::ranges::count_if(keybindings->rows, [](const auto& row) { return row.live.fromLua() && !row.recorded && row.live.description.empty(); });
        luaNote->setText(unexplained == 0 ? std::string{" "}
                                          : std::format("Hyprland does not report what a Lua binding does, and {} of these could not be found in your config.", unexplained));

        std::vector<RowList::Row> rows;
        rowLocations.clear();
        for (const auto& row : keybindings->rows) {
            const auto& live     = row.live;
            const auto  location = row.recorded ? std::optional{row.recorded->location} : std::nullopt;
            rows.push_back(RowList::Row{
                .cells  = {keyCaps(live.modifiers.with(keyName(live.key)), *palette), action(row, *palette),
                           notes(location,
                                 Flags{.submap = live.submap, .release = live.release, .repeat = live.repeat, .longPress = live.longPress, .locked = live.locked, .nonConsuming = live.nonConsuming},
                                 *palette)},
                .marked = false,
            });
            rowLocations.push_back(location);
        }
        for (const auto& binding : keybindings->inactive) {
            rows.push_back(RowList::Row{
                .cells  = {keyCaps(binding.modifiers.with(keyName(binding.key)), *palette),
                           binding.description.empty() ? luaMarkup(binding.action.lua(), *palette) : escapeMarkup(binding.description),
                           notes(binding.location,
                                 Flags{.submap       = binding.submap,
                                       .release      = binding.release,
                                       .repeat       = binding.repeat,
                                       .longPress    = binding.longPress,
                                       .locked       = binding.locked,
                                       .nonConsuming = binding.nonConsuming,
                                       .inactive     = true},
                                 *palette)},
                .marked = false,
            });
            rowLocations.emplace_back(binding.location);
        }
        list.setRows(std::move(rows));

        // The source view follows the files on disk, staying where it was.
        sources = keybindings->sources;
        if (openFile) {
            if (source(*openFile) != nullptr)
                showSource(*openFile, markedLine, false);
            else
                showList();
        }
        updateStatus();
    }

    void KeybindingsPage::updateStatus() {
        std::string text;
        switch (connection) {
            case application::Connection::connecting: text = "Connecting to Hyprland."; break;
            case application::Connection::live: text = std::format("{} bindings, live from Hyprland.", count); break;
            case application::Connection::lost:
                text = std::format("Not connected to Hyprland: {}. Trying again; the list below may be out of date.", connectionReason);
                break;
        }
        if (inactive != 0)
            text += std::format(" {} more in your config are not active.", inactive);
        if (!configurationProblem.empty())
            text += std::format(" Your config: {}.", configurationProblem);
        for (const auto& doubt : configurationDoubts)
            text += std::format(" {}", doubt);
        if (!bindingsError.empty())
            text += std::format(" Could not read the bindings: {}.", bindingsError);
        status->setText(escapeMarkup(text));
    }

}
