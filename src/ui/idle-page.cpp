// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/idle-page.hpp"

#include "application/command.hpp"
#include "application/config-check.hpp"
#include "application/lantern-folder.hpp"
#include "configuration/autostart.hpp"
#include "configuration/main-config.hpp"
#include "desktop/process.hpp"
#include "desktop/system.hpp"
#include "desktop/theme.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"
#include "ui/widgets.hpp"

#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <sstream>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr auto changeName = "Idle and lock";

        std::optional<std::filesystem::path> program(std::string_view name) {
            return desktop::findProgram(name, foundation::variable(foundation::currentEnvironment(), "PATH").value_or(""));
        }

        std::optional<std::string> fileText(const std::filesystem::path& file) {
            std::ifstream stream{file, std::ios::binary};
            if (!stream)
                return std::nullopt;
            std::stringstream text;
            text << stream.rdbuf();
            return text.str();
        }

        // A brightness to set: a backlight, and a built-in screen for it to light. Some
        // computers with no built-in screen, such as the 2013 Mac Pro, still list one.
        bool hasBacklight() {
            std::error_code error;
            if (std::filesystem::directory_iterator{"/sys/class/backlight", error} == std::filesystem::directory_iterator{})
                return false;
            for (const auto& entry : std::filesystem::directory_iterator{"/sys/class/drm", error}) {
                const auto name = entry.path().filename().string();
                if (name.contains("-eDP-") || name.contains("-LVDS-") || name.contains("-DSI-"))
                    return true;
            }
            return false;
        }

        // "In 5 minutes" choices for a time, with the one the file has when it is none of them.
        std::vector<int> minuteChoices(std::vector<int> choices, int now) {
            if (std::ranges::find(choices, now) == choices.end()) {
                choices.push_back(now);
                std::ranges::sort(choices);
            }
            return choices;
        }

        std::string minutesText(int minutes) {
            if (minutes == 0)
                return "Never";
            if (minutes % 60 == 0)
                return std::format("After {} hour{}", minutes / 60, minutes == 60 ? "" : "s");
            return std::format("After {} minute{}", minutes, minutes == 1 ? "" : "s");
        }

    }

    IdlePage::IdlePage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop) :
        palette{std::move(palette)},
        defer{std::move(defer)},
        loop{std::move(loop)},
        diff{this->palette, this->defer,
             {RowList::Column{.width = 24, .monospace = true, .secondary = false, .markup = true, .heading = ""},
              RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true, .heading = ""}},
             20} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Idle and lock", this->palette));
        root->addChild(paragraph("What happens while the computer is left alone, the lock screen, and the power mode. hypridle watches for the time to pass and "
                                 "hyprlock draws the lock screen; Lantern writes their settings after you review them.",
                                 this->palette));
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(body);
        footer = leftRow();
        root->addChild(footer);
        refresh();
    }

    void IdlePage::refresh() {
        if (reviewing)
            return;
        const auto environment = foundation::currentEnvironment();
        current                = desktop::idleIn(fileText(desktop::hypridleConfigFile(environment)).value_or(""));
        wanted                 = current;
        const auto lock        = fileText(desktop::hyprlockConfigFile(environment));
        lockNow                = !lock ? LockLook::none : desktop::lanternLockScreen(*lock) ? LockLook::lantern : LockLook::yours;
        themedLock             = lockNow == LockLook::lantern;
        showOverview();

        powerModes.clear();
        if (const auto power = program("powerprofilesctl")) {
            (void)application::runCommand(loop, *power, {"list"}, [this](bool succeeded, std::string output) {
                if (!succeeded)
                    return;
                // "* balanced:" or "  performance:", the chosen one marked.
                std::istringstream lines{output};
                for (std::string line; std::getline(lines, line);) {
                    if (line.empty() || line.back() != ':')
                        continue;
                    const auto mode = line.substr(2, line.size() - 3);
                    powerModes.push_back(mode);
                    if (line.starts_with('*'))
                        powerMode = mode;
                }
                if (!reviewing)
                    showOverview();
            });
        }
        brightness.reset();
        if (const auto brightnessctl = program("brightnessctl"); brightnessctl && hasBacklight()) {
            // "gmux_backlight,backlight,500,50%,1000"
            (void)application::runCommand(loop, *brightnessctl, {"-m"}, [this](bool succeeded, std::string output) {
                const auto percent = output.find('%');
                const auto comma   = output.rfind(',', percent);
                int        value   = 0;
                if (!succeeded || percent == std::string::npos || comma == std::string::npos ||
                    std::from_chars(output.data() + comma + 1, output.data() + percent, value).ec != std::errc{})
                    return;
                brightness = value;
                if (!reviewing)
                    showOverview();
            });
        }
    }

    void IdlePage::restyle() {
        if (!reviewing)
            showOverview();
    }

    std::expected<std::vector<configuration::FileChange>, std::string> IdlePage::plan() const {
        const auto                             environment = foundation::currentEnvironment();
        std::vector<configuration::FileChange> planned;
        if (wanted != current) {
            auto idle = configuration::planEdit(desktop::hypridleConfigFile(environment), [this](std::string_view text) { return desktop::withIdle(text, wanted); });
            if (!idle)
                return std::unexpected{idle.error()};
            if (*idle)
                planned.push_back(std::move(**idle));
            // hypridle runs from when Hyprland starts.
            if (wanted != desktop::IdleSettings{}) {
                const auto config = configuration::mainConfig(configuration::lanternLaunch());
                if (!config)
                    return std::unexpected{config.error()};
                auto start = configuration::planEdit(config->file, [lua = config->lua()](std::string_view text) { return configuration::withAutostart(text, lua, "hypridle", {}); });
                if (!start)
                    return std::unexpected{start.error()};
                if (*start)
                    planned.push_back(std::move(**start));
            }
        }
        if (themedLock && lockNow != LockLook::lantern) {
            const auto config  = configuration::mainConfig(configuration::lanternLaunch());
            const auto* theme  = config ? desktop::currentTheme(*config) : nullptr;
            const auto& colour = theme ? *theme : desktop::waybarPalettes().front();
            auto        lock   = configuration::planFile(desktop::hyprlockConfigFile(environment), desktop::lockScreen(colour));
            if (!lock)
                return std::unexpected{lock.error()};
            planned.push_back(std::move(*lock));
        }
        return planned;
    }

    void IdlePage::showOverview() {
        reviewing = false;
        body->clearChildren();
        const auto columns = leftRow(0);
        // Both as tall as the page, so each starts at the top: a row centres what is shorter.
        columns->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->gap(24)->commence();
        const auto left  = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.5, 1.0}})->commence();
        const auto right = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.46, 1.0}})->commence();
        columns->addChild(left);
        columns->addChild(right);
        body->addChild(columns);
        fillIdle(left);
        fillLock(right);
        fillPower(right);
        fillStatus();
    }

    void IdlePage::fillIdle(const CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column) {
        column->addChild(heading("When the computer is left alone", palette));
        if (!program("hypridle"))
            column->addChild(paragraph("hypridle is not installed, so nothing happens yet. Install it with sudo pacman -S hypridle; Lantern starts it once these "
                                       "are applied.",
                                       palette, true));
        const auto timeRow = [this, &column](const std::string& name, const std::string& about, std::vector<int> choices, int desktop::IdleSettings::*setting) {
            const auto               shown = minuteChoices(std::move(choices), wanted.*setting);
            std::vector<std::string> items;
            for (const auto minutes : shown)
                items.push_back(minutesText(minutes));
            column->addChild(settingRow(name, about, std::move(items), static_cast<std::size_t>(std::ranges::find(shown, wanted.*setting) - shown.begin()), palette,
                                        [this, shown, setting](std::size_t chosen) {
                                            if (chosen < shown.size())
                                                wanted.*setting = shown[chosen];
                                            showOverview();
                                        },
                                        defer));
        };
        if (hasBacklight())
            timeRow(program("brightnessctl") ? "Dim the screen" : "Dim the screen (needs brightnessctl)", "to a tenth, back on the first touch", {0, 1, 2, 3, 5, 10},
                    &desktop::IdleSettings::dim);
        timeRow("Lock", "the lock screen asks for your password", {0, 1, 2, 3, 5, 10, 15, 30, 60}, &desktop::IdleSettings::lock);
        timeRow("Turn the screens off", "back on with a key or the mouse", {0, 1, 2, 3, 5, 10, 15, 30, 60}, &desktop::IdleSettings::screensOff);
        timeRow("Suspend", "the computer sleeps, using almost no power", {0, 10, 15, 30, 60, 120}, &desktop::IdleSettings::suspend);
        column->addChild(settingRow("Lock before sleeping", "so that it wakes up locked", {"No", "Yes"}, wanted.lockBeforeSleep ? 1 : 0, palette,
                                    [this](std::size_t chosen) {
                                        wanted.lockBeforeSleep = chosen == 1;
                                        showOverview();
                                    },
                                    defer));
    }

    void IdlePage::fillLock(const CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column) {
        column->addChild(heading("Lock screen", palette));
        const auto kept = lockNow == LockLook::yours ? "Yours, from hyprlock.conf" : lockNow == LockLook::none ? "hyprlock's plain one" : "The theme's colours";
        std::vector<std::string> items{kept};
        if (lockNow != LockLook::lantern)
            items.emplace_back("The theme's colours");
        column->addChild(settingRow("Look", "the blurred desktop, a clock and a password field", std::move(items), themedLock && lockNow != LockLook::lantern ? 1 : 0, palette,
                                    [this](std::size_t chosen) {
                                        themedLock = lockNow == LockLook::lantern || chosen == 1;
                                        showOverview();
                                    },
                                    defer));
        if (const auto hyprlock = program("hyprlock")) {
            const auto row = leftRow(scaled(40.0));
            row->addChild(button("Lock now", 120, false, [hyprlock = *hyprlock] { (void)desktop::startDetached(hyprlock); }));
            row->addChild(markupLine("Your password unlocks it.", palette));
            column->addChild(row);
        } else
            column->addChild(paragraph("hyprlock is not installed; install it with sudo pacman -S hyprlock.", palette, true));
    }

    void IdlePage::fillPower(const CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column) {
        column->addChild(space(scaled(10.0)));
        column->addChild(heading("Power", palette));
        if (powerModes.empty())
            column->addChild(paragraph(program("powerprofilesctl") ? "The power modes could not be read." : "Power modes need power-profiles-daemon: sudo pacman -S power-profiles-daemon.",
                                       palette, true));
        else {
            const auto at = std::ranges::find(powerModes, powerMode);
            column->addChild(settingRow("Power mode", "saving power, or the most speed", powerModes, static_cast<std::size_t>(at - powerModes.begin()), palette,
                                        [this](std::size_t chosen) {
                                            if (chosen >= powerModes.size())
                                                return;
                                            if (const auto power = program("powerprofilesctl"))
                                                (void)application::runCommand(loop, *power, {"set", powerModes[chosen]}, [this](bool, std::string) { refresh(); });
                                        },
                                        defer));
        }
        if (!hasBacklight())
            return;
        if (!brightness) {
            column->addChild(paragraph(program("brightnessctl") ? "The brightness could not be read." : "Brightness needs brightnessctl: sudo pacman -S brightnessctl.", palette, true));
            return;
        }
        std::vector<int> levels{10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        if (std::ranges::find(levels, *brightness) == levels.end()) {
            levels.push_back(*brightness);
            std::ranges::sort(levels);
        }
        std::vector<std::string> items;
        for (const auto level : levels)
            items.push_back(std::format("{}%", level));
        column->addChild(settingRow("Brightness", "of the built-in screen", std::move(items), static_cast<std::size_t>(std::ranges::find(levels, *brightness) - levels.begin()),
                                    palette,
                                    [this, levels](std::size_t chosen) {
                                        if (chosen >= levels.size())
                                            return;
                                        if (const auto brightnessctl = program("brightnessctl"))
                                            (void)application::runCommand(loop, *brightnessctl, {"set", std::format("{}%", levels[chosen])}, [this](bool, std::string) { refresh(); });
                                    },
                                    defer));
    }

    void IdlePage::fillStatus() {
        const auto                                               planned = plan();
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> buttons;
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> away;
        if (configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), changeName))
            away.push_back(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        std::string note;
        if (!planned)
            note = escapeMarkup(std::format("Lantern cannot apply it: {}.", planned.error()));
        else if (planned->empty())
            note = outcome.empty() ? "" : std::format("{}  {}", coloured("✓", hex(palette->m_colors.accent)), escapeMarkup(outcome));
        else {
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

    void IdlePage::showReview() {
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
        body->addChild(heading("Review: idle and lock", palette));
        body->addChild(paragraph("Applying writes the changes below, after keeping a copy of each file so they can be undone, and starts hypridle again so that it "
                                 "reads them.",
                                 palette, true));
        setFooter(footer, {button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }), button("Apply", 110, true, [this] { defer([this] { apply(); }); })});
        diff.setRows(diffRows(changes, *palette));
        body->addChild(diff.element());
    }

    void IdlePage::apply() {
        if (!reviewing)
            return;
        const auto environment = foundation::currentEnvironment();
        const auto record      = configuration::applyChanges(changes, application::backupsFolder(environment), changeName);
        reviewing              = false;
        if (!record) {
            outcome = std::format("Nothing was written: {}.", record.error());
            return showOverview();
        }
        outcome = "Applied.";
        if (const auto hyprctl = desktop::findProgram("hyprctl", foundation::variable(environment, "PATH").value_or("")))
            application::checkHyprlandConfig(loop, *hyprctl, *record, [this](std::optional<std::string> problem) {
                if (!problem)
                    return;
                outcome = *problem;
                if (!reviewing)
                    showOverview();
            });
        restartIdle();
        refresh();
    }

    void IdlePage::restartIdle() {
        for (const auto process : desktop::findProcesses("hypridle"))
            (void)desktop::stopProcess(process);
        const auto hypridle = program("hypridle");
        if (!hypridle || desktop::idleIn(fileText(desktop::hypridleConfigFile(foundation::currentEnvironment())).value_or("")) == desktop::IdleSettings{})
            return;
        // A moment for the old one to exit first.
        loop.after(std::chrono::milliseconds{300}, [hypridle = *hypridle] { (void)desktop::startDetached(hypridle); });
    }

    void IdlePage::undo() {
        const auto latest = configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), changeName);
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone)
            outcome = std::format("Nothing was undone: {}.", undone.error());
        else {
            outcome = "Undone.";
            restartIdle();
        }
        refresh();
    }

}
