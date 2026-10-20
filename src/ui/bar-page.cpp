// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/bar-page.hpp"

#include "desktop/clipboard.hpp"
#include "desktop/waybar.hpp"
#include "ui/markup.hpp"
#include "ui/texts.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>

#include <filesystem>
#include <format>
#include <optional>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        // A line of markup that wraps, as wide as its column.
        CSharedPointer<Hyprtoolkit::CTextElement> markupLine(const std::string& markup, const CSharedPointer<Hyprtoolkit::CPalette>& palette, CFontSize size = CFontSize{CFontSize::HT_FONT_TEXT},
                                                             float alpha = 0.85F) {
            return Hyprtoolkit::CTextBuilder::begin()
                ->text(std::string{markup})
                ->fontSize(std::move(size))
                ->color([palette] { return palette->m_colors.text; })
                ->a(alpha)
                ->noEllipsize(true)
                ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                ->commence();
        }

        CSharedPointer<Hyprtoolkit::IElement> space(double height) {
            return Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, height}})->commence();
        }

        // A path under the home folder with ~ for the home folder.
        std::string shortPath(const std::filesystem::path& path) {
            const auto* home = std::getenv("HOME"); // NOLINT(concurrency-mt-unsafe): read once, on the UI thread
            if (home == nullptr)
                return path.string();
            const auto relative = path.lexically_relative(home);
            if (relative.empty() || relative.native().starts_with(".."))
                return path.string();
            return (std::filesystem::path{"~"} / relative).string();
        }

        // A command to type, in the monospace font on a panel of its own.
        CSharedPointer<Hyprtoolkit::IElement> commandPanel(const std::string& command, const CSharedPointer<Hyprtoolkit::CPalette>& palette) {
            const auto panel = Hyprtoolkit::CRectangleBuilder::begin()
                                   ->color([palette] { return palette->m_colors.alternateBase; })
                                   ->rounding(palette->m_vars.smallRounding)
                                   ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 40.0}})
                                   ->commence();
            // Takes the row's width left over by the Copy button.
            panel->setGrow(true, false);
            const auto text = Hyprtoolkit::CTextBuilder::begin()
                                  ->text(std::format("<span font_family=\"{}\">{}</span>", escapeMarkup(palette->m_vars.fontFamilyMonospace), escapeMarkup(command)))
                                  ->color([palette] { return palette->m_colors.text; })
                                  ->align(Hyprtoolkit::HT_FONT_ALIGN_LEFT)
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                                  ->commence();
            text->setMargin(12);
            panel->addChild(text);
            return panel;
        }

        // The command's panel, with a button that copies it when wl-copy is installed.
        CSharedPointer<Hyprtoolkit::IElement> commandRow(const std::string& command, const std::optional<std::filesystem::path>& copier,
                                                         const CSharedPointer<Hyprtoolkit::CPalette>& palette, const std::function<void(std::function<void()>)>& defer) {
            const auto row = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(8)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 40.0}})->commence();
            row->addChild(commandPanel(command, palette));
            if (!copier)
                return row;
            row->addChild(Hyprtoolkit::CButtonBuilder::begin()
                              ->label("Copy")
                              ->noBorder(true)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {90.0, 40.0}})
                              ->onMainClick([command, program = *copier, defer](CSharedPointer<Hyprtoolkit::CButtonElement> button) {
                                  const auto copied = desktop::copyText(program, command);
                                  defer([copied, weak = Hyprutils::Memory::CWeakPointer<Hyprtoolkit::CButtonElement>{button}] {
                                      if (const auto shown = weak.lock())
                                          shown->rebuild()->label(copied ? "Copied" : "Not copied")->commence();
                                  });
                              })
                              ->commence());
            return row;
        }

        CSharedPointer<Hyprtoolkit::IElement> requirementBlock(const desktop::Requirement& requirement, const desktop::Distribution& distribution,
                                                               const std::optional<std::filesystem::path>& copier, const CSharedPointer<Hyprtoolkit::CPalette>& palette,
                                                               const std::function<void(std::function<void()>)>& defer) {
            const bool dark  = darkTheme(*palette);
            const auto green = dark ? std::string{"#a6e3a1"} : std::string{"#2f7d32"};
            const auto red   = dark ? std::string{"#f38ba8"} : std::string{"#c62828"};

            const auto block = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
            const auto mark  = requirement.met() ? coloured("✓", green) : coloured("✗", red);
            const auto state = requirement.met() ? escapeMarkup(*requirement.found) : coloured("not installed", red);
            block->addChild(markupLine(std::format("{}  <span weight=\"600\">{}</span>   {}", mark, escapeMarkup(requirement.name), state), palette));
            block->addChild(paragraph(requirement.purpose, palette, true));
            block->addChild(markupLine(coloured(escapeMarkup(requirement.upstream), hex(palette->m_colors.linkText)), palette, CFontSize{CFontSize::HT_FONT_SMALL}));
            if (requirement.met())
                return block;

            block->addChild(space(4));
            if (const auto command = desktop::installCommand(requirement, distribution); !command.empty()) {
                block->addChild(paragraph("Run this in a terminal, then press Check again:", palette));
                block->addChild(commandRow(command, copier, palette, defer));
                if (!copier)
                    block->addChild(paragraph("A Copy button appears here when wl-clipboard is installed; the studio copies through its wl-copy.", palette, true));
            } else {
                const auto where = distribution.name.empty() ? std::string{"your distribution"} : distribution.name;
                block->addChild(paragraph(std::format("Install it from {}'s packages, then press Check again. On Arch Linux the package is {}.", where, requirement.package), palette));
            }
            return block;
        }

    }

    BarPage::BarPage(CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer) : palette{std::move(palette)}, defer{std::move(defer)} {
        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(10)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        root->setMargin(28);
        root->addChild(pageTitle("Bar", this->palette));
        root->addChild(paragraph("The studio has no bar of its own. It recommends Waybar, the bar most Hyprland desktops use, and helps you set it up. "
                                 "It never installs anything: when something is missing, it shows the command for you to run.",
                                 this->palette));
        root->addChild(space(8));

        checks = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(22)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})->commence();
        root->addChild(checks);
        root->addChild(space(8));
        // In a full-width row, so it starts at the left edge rather than the column's middle.
        const auto actions = Hyprtoolkit::CRowLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 34.0}})->commence();
        actions->addChild(Hyprtoolkit::CButtonBuilder::begin()
                              ->label("Check again")
                              ->noBorder(true)
                              ->accent(true)
                              ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {140.0, 34.0}})
                              ->onMainClick([this](CSharedPointer<Hyprtoolkit::CButtonElement>) { this->defer([this] { check(); }); })
                              ->commence());
        root->addChild(actions);
        check();
    }

    void BarPage::check() {
        const auto environment  = foundation::currentEnvironment();
        const auto distribution = desktop::currentDistribution();

        const auto copier       = desktop::clipboardProgram(foundation::variable(environment, "PATH").value_or(""));

        checks->clearChildren();
        for (const auto& requirement : desktop::waybarRequirements(environment))
            checks->addChild(requirementBlock(requirement, distribution, copier, palette, defer));

        const auto config = desktop::waybarConfig(environment);
        std::string reads;
        if (!config)
            reads = "Waybar has no config to read, so it will not start.";
        else if (config->string().starts_with("/etc/"))
            reads = std::format("Waybar reads its example config, {}. It was written for Sway, so it shows none of Hyprland's workspaces.", config->string());
        else
            reads = std::format("Waybar reads {}.", shortPath(*config));
        checks->addChild(paragraph(reads, palette));
    }

}
