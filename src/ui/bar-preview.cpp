// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/bar-preview.hpp"

#include "ui/markup.hpp"

#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>

#include <algorithm>
#include <charconv>
#include <format>
#include <string>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;
        using Hyprutils::Memory::CSharedPointer;

        // Waybar's sizes in the studio's config, in pixels.
        constexpr double barHeight      = 38;
        constexpr double fullHeight     = 34;
        constexpr double edgeGap        = 20;
        constexpr double topGap         = 10;
        constexpr double pillWidth      = 30;
        constexpr double pillGap        = 4;
        constexpr double groupPadding   = 6;
        constexpr double clockWidth     = 180;
        constexpr double statusWidth    = 200;
        // Waybar's 15px font, in points.
        constexpr float  fontPoints     = 11.25F;
        constexpr float  islandOpacity  = 0.92F;

        Hyprtoolkit::CHyprColor colour(std::string_view hexColour, float alpha = 1.F) {
            unsigned value = 0;
            if (hexColour.size() == 7)
                std::from_chars(hexColour.data() + 1, hexColour.data() + 7, value, 16);
            return {static_cast<float>((value >> 16U) & 0xFFU) / 255.F, static_cast<float>((value >> 8U) & 0xFFU) / 255.F, static_cast<float>(value & 0xFFU) / 255.F, alpha};
        }

        CSharedPointer<Hyprtoolkit::IElement> gap(double width, double height = 1) {
            return Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {width, height}})->commence();
        }

        CSharedPointer<Hyprtoolkit::IElement> spring() {
            const auto made = gap(1);
            made->setGrow(true, false);
            return made;
        }

        CSharedPointer<Hyprtoolkit::CRowLayoutElement> row(double width, double height, double spacing = 0);

        // Text centred in a box `width` wide.
        CSharedPointer<Hyprtoolkit::IElement> label(const std::string& markup, std::string_view hexColour, double width, double scale, double height = 0) {
            const auto text = Hyprtoolkit::CTextBuilder::begin()
                                  ->text(std::format("<span font_family=\"Adwaita Sans, Symbols Nerd Font\">{}</span>", markup))
                                  ->fontSize(CFontSize{CFontSize::HT_FONT_ABSOLUTE, fontPoints * static_cast<float>(scale)})
                                  ->color([c = colour(hexColour)] { return c; })
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.0, 1.0}})
                                  ->commence();
            // A column centres a narrower child across but starts it at the top, so a spacer
            // above takes half of what the line leaves. A column sized to its content
            // comes out empty in 0.6.0, so the box has a fixed height.
            const auto boxHeight  = height > 0 ? height : barHeight * scale;
            const auto lineHeight = fontPoints * scale * 4.0 / 3.0 * 1.2;
            const auto box        = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {width, boxHeight}})->commence();
            box->addChild(gap(1, std::max(0.0, (boxHeight - lineHeight) / 2)));
            box->addChild(text);
            return box;
        }

        // Left, centre and right thirds. In hyprtoolkit 0.6.0 only the first growing child
        // of a row takes the space left over, so two springs cannot centre anything; a
        // column centres a narrower child, so the middle third is one.
        CSharedPointer<Hyprtoolkit::IElement> thirds(const CSharedPointer<Hyprtoolkit::IElement>& left, const CSharedPointer<Hyprtoolkit::IElement>& centre,
                                                     const CSharedPointer<Hyprtoolkit::IElement>& right, double height, bool grow) {
            const auto whole = row(grow ? 1 : 0, height);
            if (grow)
                whole->setGrow(true, false);
            const auto third = [height] { return CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0 / 3.0, height}}; };
            const auto start = Hyprtoolkit::CRowLayoutBuilder::begin()->size(third())->commence();
            start->addChild(left);
            const auto middle = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(third())->commence();
            middle->addChild(centre);
            const auto end = Hyprtoolkit::CRowLayoutBuilder::begin()->size(third())->commence();
            end->addChild(spring());
            end->addChild(right);
            whole->addChild(start);
            whole->addChild(middle);
            whole->addChild(end);
            return whole;
        }

        CSharedPointer<Hyprtoolkit::CRowLayoutElement> row(double width, double height, double spacing) {
            return Hyprtoolkit::CRowLayoutBuilder::begin()
                ->gap(static_cast<std::size_t>(spacing))
                ->size(CDynamicSize{width > 0 ? CDynamicSize::HT_SIZE_ABSOLUTE : CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {width > 0 ? width : 1.0, height}})
                ->commence();
        }

        double workspacesWidth(double scale) {
            return (5 * pillWidth + 4 * pillGap + 2 * groupPadding) * scale;
        }

        // Workspace 1 active, 2 and 3 with windows, 4 and 5 empty.
        CSharedPointer<Hyprtoolkit::IElement> workspaces(const desktop::Palette& palette, double scale, double height) {
            const auto made = row(workspacesWidth(scale), height, pillGap * scale);
            made->addChild(gap((groupPadding - pillGap) * scale));
            for (int number = 1; number <= 5; ++number) {
                const auto textColour = number == 1 ? palette.onAccent : number <= 3 ? palette.subtle : palette.raised;
                const auto pill       = Hyprtoolkit::CRectangleBuilder::begin()
                                      ->color([c = colour(palette.accent, number == 1 ? 1.F : 0.F)] { return c; })
                                      ->rounding(static_cast<int>(7 * scale))
                                      ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {pillWidth * scale, height - 10 * scale}})
                                      ->commence();
                const auto inside = row(pillWidth * scale, height - 10 * scale);
                inside->addChild(label(std::to_string(number), textColour, pillWidth * scale, scale, height - 10 * scale));
                pill->addChild(inside);
                made->addChild(pill);
            }
            return made;
        }

        CSharedPointer<Hyprtoolkit::IElement> clock(const desktop::Palette& palette, double scale, double height) {
            const auto made = row(clockWidth * scale, height);
            made->addChild(label("<b>Tue 20 Oct   14:30</b>", palette.text, clockWidth * scale, scale, height));
            return made;
        }

        // Audio, network and a tray icon.
        CSharedPointer<Hyprtoolkit::IElement> status(const desktop::Palette& palette, double scale, double height) {
            const auto made = row(statusWidth * scale, height);
            made->addChild(label("\U000F057E  40%", palette.audio, 80 * scale, scale, height));
            made->addChild(label("\U000F0928  home", palette.network, 90 * scale, scale, height));
            const auto trayIcon = Hyprtoolkit::CRectangleBuilder::begin()
                                      ->color([c = colour(palette.subtle)] { return c; })
                                      ->rounding(static_cast<int>(4 * scale))
                                      ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {16 * scale, 16 * scale}})
                                      ->commence();
            made->addChild(gap(12 * scale));
            made->addChild(trayIcon);
            return made;
        }

        CSharedPointer<Hyprtoolkit::CRectangleElement> panel(const desktop::Palette& palette, double width, double height, double rounding, bool border, float alpha) {
            auto builder = Hyprtoolkit::CRectangleBuilder::begin()
                               ->color([c = colour(palette.base, alpha)] { return c; })
                               ->rounding(static_cast<int>(rounding))
                               ->size(CDynamicSize{width > 0 ? CDynamicSize::HT_SIZE_ABSOLUTE : CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {width > 0 ? width : 1.0, height}});
            if (border)
                builder->borderColor([c = colour(palette.surface)] { return c; })->borderThickness(1);
            return builder->commence();
        }

        CSharedPointer<Hyprtoolkit::IElement> island(const CSharedPointer<Hyprtoolkit::IElement>& content, double width, const desktop::Palette& palette, double scale) {
            const auto made = panel(palette, width, barHeight * scale, 10 * scale, true, islandOpacity);
            made->addChild(content);
            return made;
        }

    }

    double barPreviewHeight(const desktop::WaybarStyle& style, double scale) {
        return style.shape == desktop::Shape::fullWidth ? (fullHeight + 2 * topGap) * scale : (barHeight + 2 * topGap) * scale;
    }

    CSharedPointer<Hyprtoolkit::IElement> barPreview(const desktop::WaybarStyle& style, double scale) {
        const auto& palette  = *style.palette;
        const bool  floating = style.shape != desktop::Shape::fullWidth;
        const auto  height   = barPreviewHeight(style, scale);

        // The desktop behind the bar: a soft tone that shows a light or a dark bar.
        const auto backdrop = Hyprtoolkit::CRectangleBuilder::begin()
                                  ->color([c = palette.dark ? colour("#4b5263") : colour("#9aa3b5")] { return c; })
                                  ->rounding(static_cast<int>(8 * scale))
                                  ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, height}})
                                  ->commence();
        const auto stack = Hyprtoolkit::CColumnLayoutBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        backdrop->addChild(stack);
        if (floating)
            stack->addChild(gap(1, topGap * scale));

        const auto line    = row(0, (floating ? barHeight : fullHeight) * scale);
        const auto bar     = line;
        const auto barSize = (floating ? barHeight : fullHeight) * scale;
        if (floating)
            line->addChild(gap(edgeGap * scale));

        switch (style.shape) {
            case desktop::Shape::islands: {
                line->addChild(thirds(island(workspaces(palette, scale, barSize), workspacesWidth(scale), palette, scale),
                                      island(clock(palette, scale, barSize), clockWidth * scale, palette, scale),
                                      island(status(palette, scale, barSize), statusWidth * scale, palette, scale), barSize, true));
                break;
            }
            case desktop::Shape::floating:
            case desktop::Shape::fullWidth: {
                const auto whole = panel(palette, 1, barSize, floating ? 10 * scale : 0, floating, floating ? islandOpacity : 1.F);
                whole->setGrow(true, false);
                whole->addChild(thirds(workspaces(palette, scale, barSize), clock(palette, scale, barSize), status(palette, scale, barSize), barSize, false));
                line->addChild(whole);
                break;
            }
        }
        if (floating)
            line->addChild(gap(edgeGap * scale));
        stack->addChild(bar);
        return backdrop;
    }

}
