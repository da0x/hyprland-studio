// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/keybindings-page.hpp"

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

        std::string action(const application::KeybindingRow& row) {
            if (!row.live.description.empty())
                return row.live.description;
            if (row.recorded)
                return row.recorded->description.empty() ? row.recorded->action.lua() : row.recorded->description;
            // Under Lua, Hyprland reports no action at all, only an internal number.
            if (row.live.fromLua())
                return {};
            return row.live.argument.empty() ? row.live.dispatcher : std::format("{} {}", row.live.dispatcher, row.live.argument);
        }

        struct Flags {
            std::string_view submap;
            bool             release      = false;
            bool             repeat       = false;
            bool             longPress    = false;
            bool             locked       = false;
            bool             nonConsuming = false;
        };

        // Where the binding was set, then anything unusual about it.
        std::string notes(const std::optional<configuration::Location>& location, const Flags& flags) {
            std::string text = location ? location->brief() : std::string{};
            const auto  add  = [&text](bool set, std::string_view note) {
                if (!set)
                    return;
                if (!text.empty())
                    text += ", ";
                text += note;
            };
            add(!flags.submap.empty(), std::format("submap {}", flags.submap));
            add(flags.release, "on release");
            add(flags.repeat, "repeats");
            add(flags.longPress, "long press");
            add(flags.locked, "also when locked");
            add(flags.nonConsuming, "passes the key on");
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

        constexpr double rowHeight   = 28;
        constexpr double keysWidth   = 300;
        constexpr double notesWidth  = 260;

    }

    KeybindingsPage::KeybindingsPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer) :
        palette{std::move(palette)},
        defer{defer},
        list{this->palette, defer,
             {RowList::Column{.width = keysWidth, .monospace = true, .secondary = false, .markup = false}, RowList::Column{.width = 0, .monospace = false, .secondary = false, .markup = false},
              RowList::Column{.width = notesWidth, .monospace = false, .secondary = true, .markup = false}},
             rowHeight},
        sourceView{this->palette, defer} {
        const auto text = [p = this->palette] { return p->m_colors.text; };

        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(20);

        root->addChild(Hyprtoolkit::CTextBuilder::begin()->text("Keybindings")->fontSize(CFontSize{CFontSize::HT_FONT_H1})->color(text)->commence());

        status = Hyprtoolkit::CTextBuilder::begin()->text("Connecting to Hyprland.")->noEllipsize(true)->color(text)->a(0.7F)->commence();
        root->addChild(status);

        // An empty text is laid out as nothing, so "nothing to say" is a space.
        luaNote = Hyprtoolkit::CTextBuilder::begin()->text(" ")->noEllipsize(true)->fontSize(CFontSize{CFontSize::HT_FONT_SMALL})->color(text)->a(0.6F)->commence();
        root->addChild(luaNote);

        // The switch, then one button per config file while the source is showing.
        const auto toolbar = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 30.0}})->commence();
        switchButton       = Hyprtoolkit::CButtonBuilder::begin()
                           ->label("Show source")
                           ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {130.0, 30.0}})
                           ->onMainClick([this](CSharedPointer<Hyprtoolkit::CButtonElement>) {
                               if (openFile)
                                   showList();
                               else if (!sources.empty())
                                   showSource(sources.front().path, 0, true);
                           })
                           ->commence();
        fileButtons = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 30.0}})->commence();
        fileButtons->setGrow(true, false);
        toolbar->addChild(switchButton);
        toolbar->addChild(fileButtons);
        root->addChild(toolbar);

        caption = Hyprtoolkit::CTextBuilder::begin()->text(" ")->fontSize(CFontSize{CFontSize::HT_FONT_SMALL})->color(text)->a(0.6F)->commence();

        // Holds the list or the source; its height is what is left.
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(body);

        list.setClicked([this](std::size_t row) {
            if (row < rowLocations.size() && rowLocations[row])
                showSource(rowLocations[row]->file, rowLocations[row]->line, true);
        });
        list.setRows({RowList::Row{.cells = {{}, "Reading your keybindings.", {}}}});
        showList();
    }

    void KeybindingsPage::showList() {
        openFile.reset();
        markedLine = 0;
        switchButton->setLabel("Show source");
        body->clearChildren();
        body->addChild(list.element());
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
        caption->setText(escapeMarkup(line > 0 ? std::format("{}, line {}. What is on disk now; Hyprland may have loaded an older version.", shortPath(file), line)
                                               : std::format("{}. What is on disk now; Hyprland may have loaded an older version.", shortPath(file))));
        if (wasList) {
            switchButton->setLabel("Show list");
            body->clearChildren();
            body->addChild(caption);
            body->addChild(sourceView.element());
        }
        updateFileButtons();
    }

    void KeybindingsPage::updateFileButtons() {
        fileButtons->clearChildren();
        if (!openFile || sources.empty())
            return;
        const auto folder = sources.front().path.parent_path();
        for (const auto& file : sources) {
            auto label = file.path.lexically_relative(folder).string();
            if (file.path == *openFile)
                label = std::format("[ {} ]", label);
            fileButtons->addChild(Hyprtoolkit::CButtonBuilder::begin()
                                      ->label(std::move(label))
                                      ->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 30.0}})
                                      ->onMainClick([this, path = file.path](CSharedPointer<Hyprtoolkit::CButtonElement>) {
                                          // Showing a file replaces this button, so not from inside its handler.
                                          defer([this, path] { showSource(path, 0, true); });
                                      })
                                      ->commence());
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
            rows.push_back(RowList::Row{.cells = {
                                            live.modifiers.with(keyName(live.key)),
                                            action(row),
                                            notes(location, Flags{.submap = live.submap, .release = live.release, .repeat = live.repeat, .longPress = live.longPress, .locked = live.locked, .nonConsuming = live.nonConsuming}),
                                        }});
            rowLocations.push_back(location);
        }
        for (const auto& binding : keybindings->inactive) {
            auto text = notes(binding.location, Flags{.submap = binding.submap, .release = binding.release, .repeat = binding.repeat, .longPress = binding.longPress, .locked = binding.locked, .nonConsuming = binding.nonConsuming});
            rows.push_back(RowList::Row{.cells = {
                                            binding.modifiers.with(keyName(binding.key)),
                                            binding.description.empty() ? binding.action.lua() : binding.description,
                                            std::format("not active, {}", text),
                                        }});
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
        if (!bindingsError.empty())
            text += std::format(" Could not read the bindings: {}.", bindingsError);
        status->setText(escapeMarkup(text));
    }

}
