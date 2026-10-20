// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "desktop/system.hpp"
#include "desktop/waybar.hpp"
#include "ui/row-list.hpp"

#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ui {

    // The pieces the setup pages (Bar, Wallpaper) are built from.

    // A line of markup that wraps, as wide as its column.
    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement> markupLine(const std::string& markup, const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>& palette,
                                                                                          Hyprtoolkit::CFontSize size  = Hyprtoolkit::CFontSize{Hyprtoolkit::CFontSize::HT_FONT_TEXT},
                                                                                          float                  alpha = 0.85F);

    // Empty space of a height.
    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> space(double height);

    // A path under the home folder with ~ for the home folder.
    [[nodiscard]] std::string shortPath(const std::filesystem::path& path);

    // A command to type on a panel of its own, with a button that copies it when wl-copy is
    // installed. `lead`, markup, goes before it on the same line.
    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> commandRow(const std::string& command, const std::optional<std::filesystem::path>& copier,
                                                                                      const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>& palette,
                                                                                      const std::function<void(std::function<void()>)>& defer, const std::string& lead = {});

    // One thing a page needs: whether it is installed, why it is needed, and how to install it.
    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> requirementBlock(const desktop::Requirement& requirement, const desktop::Distribution& distribution,
                                                                                            const std::optional<std::filesystem::path>&                     copier,
                                                                                            const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>& palette,
                                                                                            const std::function<void(std::function<void()>)>&               defer);

    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CButtonElement> button(std::string label, double width, bool accent, std::function<void()> clicked);

    // A full-width row, so what is in it starts at the left edge rather than the column's middle.
    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement> leftRow(double height = 34.0);

    // Puts a page's action buttons in its footer, at the right, the main action last: the
    // order GNOME and macOS use, and the place KDE and GTK dialogs put them.
    // `note`, when given, goes just before the buttons: where a page says what is applied,
    // so that saying it does not move anything else on the page. `away` goes at the left,
    // far from the main action, for what should not be clicked by accident, such as Undo.
    void setFooter(const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement>&            footer,
                   const std::vector<Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CButtonElement>>& buttons,
                   const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement>&                    note = nullptr,
                   const std::vector<Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CButtonElement>>& away = {});

    [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> heading(const std::string& text, const Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>& palette);

    // A diff as list rows: a header per file, then its changes with a sign and a colour.
    [[nodiscard]] std::vector<RowList::Row> diffRows(const std::vector<configuration::FileChange>& changes, const Hyprtoolkit::CPalette& palette);

}
