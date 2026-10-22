// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/input-page.hpp"

#include "application/config-check.hpp"
#include "application/lantern-folder.hpp"
#include "configuration/main-config.hpp"
#include "desktop/system.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"
#include "ui/widgets.hpp"

#include <hyprtoolkit/element/Combobox.hpp>

#include <algorithm>
#include <format>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprutils::Memory::CSharedPointer;

        constexpr auto changeName = "Keyboard and mouse";
        // xkb switches between at most four layouts.
        constexpr std::size_t mostLayouts = 4;

        template <typename Value>
        struct Option {
            std::string label;
            Value       value;
        };

        // A dropdown of `options`, with the value the config has when it is none of them,
        // shown by `yours`.
        template <typename Value>
        CSharedPointer<Hyprtoolkit::CRowLayoutElement> optionRow(const std::string& name, const std::string& about, std::vector<Option<Value>> options, const Value& now,
                                                                  const std::function<std::string(const Value&)>& yours, std::function<void(const Value&)> choose,
                                                                  const CSharedPointer<Hyprtoolkit::CPalette>& palette, const std::function<void(std::function<void()>)>& defer) {
            if (std::ranges::none_of(options, [&now](const auto& option) { return option.value == now; }))
                options.push_back({yours(now), now});
            std::vector<std::string> labels;
            for (const auto& option : options)
                labels.push_back(option.label);
            const auto at = static_cast<std::size_t>(std::ranges::find(options, now, &Option<Value>::value) - options.begin());
            return settingRow(name, about, std::move(labels), at, palette, [options, choose = std::move(choose)](std::size_t index) { choose(options[index].value); }, defer);
        }

        std::string layoutName(const std::vector<desktop::KnownLayout>& known, const desktop::Layout& layout) {
            const auto found = std::ranges::find(known, layout.code, &desktop::KnownLayout::code);
            if (found == known.end())
                return layout.code;
            if (layout.variant.empty())
                return found->name;
            const auto variant = std::ranges::find(found->variants, layout.variant, &desktop::KnownLayout::code);
            return variant != found->variants.end() ? variant->name : std::format("{} ({})", found->name, layout.variant);
        }

    }

    InputPage::InputPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, application::EventLoop loop) :
        palette{std::move(palette)},
        defer{std::move(defer)},
        loop{std::move(loop)},
        diff{this->palette, this->defer,
             {RowList::Column{.width = 24, .monospace = true, .secondary = false, .markup = true, .heading = ""},
              RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true, .heading = ""}},
             20},
        known{desktop::installedLayouts()} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Keyboard and mouse", this->palette));
        root->addChild(paragraph("Your keyboard's layouts and the keys that switch between them, what Caps Lock does, how held keys repeat, how the pointer "
                                 "moves, and how the touchpad clicks and scrolls. Nothing is written until you review the change, and Hyprland uses it at once.",
                                 this->palette));
        body = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        body->setGrow(false, true);
        root->addChild(body);
        footer = leftRow();
        root->addChild(footer);
        reread();
        wanted = current.value_or(desktop::InputSettings{});
        showOverview();
    }

    void InputPage::reread() {
        const auto config = configuration::mainConfig(configuration::lanternLaunch());
        current           = config ? desktop::currentInput(*config) : std::nullopt;
        touchpad          = desktop::touchpadConnected();
    }

    void InputPage::restyle() {
        if (!reviewing)
            showOverview();
    }

    std::expected<std::optional<configuration::FileChange>, std::string> InputPage::plan() const {
        const auto config = configuration::mainConfig(configuration::lanternLaunch());
        if (!config)
            return std::unexpected{config.error()};
        return desktop::planInput(wanted, *config);
    }

    void InputPage::showOverview() {
        reviewing = false;
        body->clearChildren();
        // The keyboard on the left, the mouse on the right.
        const auto columns = leftRow(0);
        // Both as tall as the page, so each starts at the top: a row centers what is shorter.
        columns->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->gap(24)->commence();
        const auto keyboard = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.5, 1.0}})->commence();
        const auto mouse    = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(6)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {0.46, 1.0}})->commence();
        columns->addChild(keyboard);
        columns->addChild(mouse);
        body->addChild(columns);
        fillKeyboard(keyboard);
        fillMouse(mouse);
        fillTouchpad(mouse);
        fillStatus();
    }

    void InputPage::fillKeyboard(const CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column) {
        column->addChild(heading("Layouts", palette));
        // Each layout, with its variant and a button that takes it away.
        std::vector<std::string> layoutNames;
        for (const auto& layout : known)
            layoutNames.push_back(layout.name);
        for (std::size_t index = 0; index < wanted.layouts.size(); ++index) {
            const auto& layout = wanted.layouts[index];
            const auto  row    = leftRow(scaled(40.0));
            row->rebuild()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, scaled(40.0)}})->gap(6)->commence();
            const auto found = std::ranges::find(known, layout.code, &desktop::KnownLayout::code);
            auto       names = layoutNames;
            auto       at    = static_cast<std::size_t>(found - known.begin());
            if (found == known.end()) {
                names.push_back(layout.code);
                at = names.size() - 1;
            }
            row->addChild(Hyprtoolkit::CComboboxBuilder::begin()
                              ->items(std::move(names))
                              ->currentItem(at)
                              ->onChanged([this, index](CSharedPointer<Hyprtoolkit::CComboboxElement>, std::size_t chosen) {
                                  defer([this, index, chosen] {
                                      if (chosen < known.size())
                                          wanted.layouts[index] = desktop::Layout{known[chosen].code, ""};
                                      showOverview();
                                  });
                              })
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {scaled(220.0), scaled(34.0)}})
                              ->commence());
            if (found != known.end() && !found->variants.empty()) {
                std::vector<std::string> variants{"Standard"};
                for (const auto& variant : found->variants)
                    variants.push_back(variant.name);
                const auto  variant = std::ranges::find(found->variants, layout.variant, &desktop::KnownLayout::code);
                std::size_t chosen  = layout.variant.empty() ? 0 : static_cast<std::size_t>(variant - found->variants.begin()) + 1;
                // One xkb does not have, which it ignores, shown as it is rather than as none.
                if (!layout.variant.empty() && variant == found->variants.end()) {
                    variants.push_back(std::format("{}, unknown to xkb", layout.variant));
                    chosen = variants.size() - 1;
                }
                row->addChild(Hyprtoolkit::CComboboxBuilder::begin()
                                  ->items(std::move(variants))
                                  ->currentItem(chosen)
                                  ->onChanged([this, index, codes = found->variants](CSharedPointer<Hyprtoolkit::CComboboxElement>, std::size_t chosen) {
                                      defer([this, index, chosen, codes] {
                                          if (chosen > codes.size())
                                              return;
                                          wanted.layouts[index].variant = chosen == 0 ? "" : codes[chosen - 1].code;
                                          showOverview();
                                      });
                                  })
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {scaled(220.0), scaled(34.0)}})
                                  ->commence());
            }
            if (wanted.layouts.size() > 1)
                row->addChild(button("−", scaled(34.0), false, [this, index] {
                    defer([this, index] {
                        wanted.layouts.erase(wanted.layouts.begin() + static_cast<std::ptrdiff_t>(index));
                        showOverview();
                    });
                }));
            column->addChild(row);
        }
        if (wanted.layouts.size() < mostLayouts) {
            const auto row = leftRow(scaled(40.0));
            row->addChild(button("Add a layout", scaled(160.0), false, [this] {
                defer([this] {
                    wanted.layouts.push_back(desktop::Layout{"us", ""});
                    showOverview();
                });
            }));
            column->addChild(row);
        }
        if (wanted.layouts.size() > 1)
            column->addChild(optionRow<std::string>(
                "Switch layouts with", std::format("the keys that go from {} to the next", layoutName(known, wanted.layouts.front())),
                {{"Alt + Shift", "grp:alt_shift_toggle"},
                 {"Ctrl + Shift", "grp:ctrl_shift_toggle"},
                 {"Super + Space", "grp:win_space_toggle"},
                 {"Both Shift keys", "grp:shifts_toggle"},
                 {"Caps Lock", "grp:caps_toggle"},
                 {"No keys", ""}},
                wanted.switchKeys, [](const std::string& option) { return std::format("Yours: {}", option); },
                [this](const std::string& option) {
                    wanted.switchKeys = option;
                    showOverview();
                },
                palette, defer));

        column->addChild(space(8));
        column->addChild(heading("Typing", palette));
        column->addChild(optionRow<std::string>("Caps Lock key", "what pressing it does",
                                                {{"Caps Lock", ""},
                                                 {"Escape", "caps:escape"},
                                                 {"Escape, Shift + it for Caps Lock", "caps:escape_shifted_capslock"},
                                                 {"Swapped with Escape", "caps:swapescape"},
                                                 {"Another Ctrl", "ctrl:nocaps"},
                                                 {"Backspace", "caps:backspace"},
                                                 {"Super", "caps:super"},
                                                 {"Nothing", "caps:none"}},
                                                wanted.capsLock, [](const std::string& option) { return std::format("Yours: {}", option); },
                                                [this](const std::string& option) {
                                                    wanted.capsLock = option;
                                                    showOverview();
                                                },
                                                palette, defer));
        column->addChild(optionRow<int>("Repeat after", "how long a key is held before it repeats",
                                        {{"200 ms", 200}, {"250 ms", 250}, {"300 ms", 300}, {"400 ms", 400}, {"500 ms", 500}, {"600 ms, Hyprland's own", 600}, {"800 ms", 800}, {"1 s", 1000}},
                                        wanted.repeatDelay, [](const int& delay) { return std::format("{} ms", delay); },
                                        [this](const int& delay) {
                                            wanted.repeatDelay = delay;
                                            showOverview();
                                        },
                                        palette, defer));
        column->addChild(optionRow<int>("Repeat speed", "how many times a second a held key repeats",
                                        {{"15 a second", 15}, {"20 a second", 20}, {"25 a second, Hyprland's own", 25}, {"30 a second", 30}, {"40 a second", 40}, {"50 a second", 50}},
                                        wanted.repeatRate, [](const int& rate) { return std::format("{} a second", rate); },
                                        [this](const int& rate) {
                                            wanted.repeatRate = rate;
                                            showOverview();
                                        },
                                        palette, defer));
        column->addChild(optionRow<bool>("Num Lock", "whether it is on when Hyprland starts", {{"Off at start", false}, {"On at start", true}}, wanted.numLock,
                                         [](const bool&) { return std::string{}; },
                                         [this](const bool& on) {
                                             wanted.numLock = on;
                                             showOverview();
                                         },
                                         palette, defer));
    }

    void InputPage::fillMouse(const CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column) {
        column->addChild(heading("Mouse and pointer", palette));
        column->addChild(optionRow<double>("Pointer speed", "how far the pointer goes for the same move",
                                           {{"Slowest", -0.75}, {"Slower", -0.5}, {"Slow", -0.25}, {"As the mouse reports it", 0.0}, {"Fast", 0.25}, {"Faster", 0.5}, {"Fastest", 0.75}},
                                           wanted.sensitivity, [](const double& speed) { return std::format("Yours: {:g}", speed); },
                                           [this](const double& speed) {
                                               wanted.sensitivity = speed;
                                               showOverview();
                                           },
                                           palette, defer));
        column->addChild(optionRow<bool>("Acceleration", "whether a faster move takes the pointer further",
                                         {{"On, as most desktops have it", false}, {"Off, as games prefer", true}}, wanted.flat, [](const bool&) { return std::string{}; },
                                         [this](const bool& flat) {
                                             wanted.flat = flat;
                                             showOverview();
                                         },
                                         palette, defer));
        column->addChild(choice("Buttons", "which button clicks and which opens menus", "Left clicks, for the right hand", "Right clicks, for the left hand",
                                &desktop::InputSettings::leftHanded));
        column->addChild(choice("Scrolling", "which way the wheel moves the page", "Down moves the page down", "Natural, as on a phone",
                                &desktop::InputSettings::mouseNatural));
    }

    void InputPage::fillTouchpad(const CSharedPointer<Hyprtoolkit::CColumnLayoutElement>& column) {
        column->addChild(space(scaled(10.0)));
        column->addChild(heading("Touchpad", palette));
        if (!touchpad)
            column->addChild(markupLine("No touchpad is connected. These take effect when one is.", palette));
        column->addChild(choice("Tap to click", "whether a tap is a click", "Off, press to click", "On", &desktop::InputSettings::tapToClick));
        column->addChild(choice("Right click", "how to open a menu", "Press the bottom-right corner", "Press with two fingers", &desktop::InputSettings::twoFingerClick));
        column->addChild(choice("While typing", "whether a palm on the touchpad moves the pointer", "Keeps working", "Pauses for a moment",
                                &desktop::InputSettings::typingDisables));
        column->addChild(choice("Scrolling", "which way two fingers move the page", "Down moves the page down", "Natural, as on a phone",
                                &desktop::InputSettings::touchpadNatural));
        column->addChild(optionRow<double>("Scroll speed", "how far the page moves for the same swipe",
                                           {{"Slower", 0.5}, {"Slow", 0.75}, {"As the touchpad reports it", 1.0}, {"Fast", 1.5}, {"Faster", 2.0}}, wanted.touchpadScroll,
                                           [](const double& factor) { return std::format("Yours: {:g}", factor); },
                                           [this](const double& factor) {
                                               wanted.touchpadScroll = factor;
                                               showOverview();
                                           },
                                           palette, defer));
        column->addChild(choice("Three fingers sideways", "a swipe to the next workspace", "Do nothing", "Switch workspaces", &desktop::InputSettings::swipeWorkspaces));
    }

    CSharedPointer<Hyprtoolkit::IElement> InputPage::choice(const std::string& name, const std::string& about, const std::string& off, const std::string& on,
                                                            bool desktop::InputSettings::*setting) {
        return optionRow<bool>(name, about, {{off, false}, {on, true}}, wanted.*setting, [](const bool&) { return std::string{}; },
                               [this, setting](const bool& value) {
                                   wanted.*setting = value;
                                   showOverview();
                               },
                               palette, defer);
    }

    void InputPage::fillStatus() {
        const auto                                               planned = plan();
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> buttons;
        std::vector<CSharedPointer<Hyprtoolkit::CButtonElement>> away;
        if (configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), changeName))
            away.push_back(button("Undo last change", 160, false, [this] { defer([this] { undo(); }); }));
        std::string note;
        if (!planned)
            note = escapeMarkup(std::format("Lantern cannot apply it: {}.", planned.error()));
        else if (!*planned)
            note = std::format("{}  {}", colored("✓", hex(palette->m_colors.accent)), escapeMarkup(outcome.empty() ? "Your keyboard and mouse are set like this." : outcome));
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

    void InputPage::showReview() {
        auto planned = plan();
        if (!planned || !*planned) {
            outcome = planned ? "" : planned.error();
            return showOverview();
        }
        reviewing = true;
        change    = std::move(**planned);
        outcome.clear();
        body->clearChildren();
        body->addChild(space(4));
        body->addChild(heading("Review: keyboard and mouse", palette));
        body->addChild(paragraph("Applying writes the change below, after keeping a copy of the config so it can be undone. Hyprland reads it at once; if it reports a "
                                 "problem with it, Lantern puts the config back.",
                                 palette, true));
        setFooter(footer, {button("Cancel", 110, false, [this] { defer([this] { showOverview(); }); }), button("Apply", 110, true, [this] { defer([this] { apply(); }); })});
        diff.setRows(diffRows({change}, *palette));
        body->addChild(diff.element());
    }

    void InputPage::apply() {
        if (!reviewing)
            return;
        const auto environment = foundation::currentEnvironment();
        const auto record      = configuration::applyChanges({change}, application::backupsFolder(environment), changeName);
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
                reread();
                if (!reviewing)
                    showOverview();
            });
        reread();
        showOverview();
    }

    void InputPage::undo() {
        const auto latest = configuration::latestChange(application::backupsFolder(foundation::currentEnvironment()), changeName);
        if (!latest)
            return showOverview();
        if (const auto undone = configuration::undoChange(*latest); !undone)
            outcome = std::format("Nothing was undone: {}.", undone.error());
        else
            outcome = "Undone.";
        reread();
        wanted = current.value_or(wanted);
        showOverview();
    }

}
