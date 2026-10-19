// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/row-list.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;

        constexpr double rowHeight        = 28;
        constexpr double rowGap           = 2;
        constexpr double keysWidth        = 300;
        constexpr double notesWidth       = 260;
        constexpr int    rowsPerWheelStep = 3;

    }

    RowList::RowList(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer) :
        palette{std::move(palette)}, defer{std::move(defer)} {
        const auto text = [p = this->palette] { return p->m_colors.text; };

        root = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(4)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        root->setGrow(false, true);

        // Takes the height left over; its height decides how many rows fit.
        viewport = Hyprtoolkit::CNullBuilder::begin()->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, 1.0}})->commence();
        viewport->setGrow(false, true);
        viewport->setReceivesMouse(true);
        viewport->setMouseAxis([this](Hyprtoolkit::Input::eAxisAxis axis, float delta) {
            if (axis == Hyprtoolkit::Input::AXIS_AXIS_VERTICAL && delta != 0.F)
                scrollBy(delta > 0.F ? rowsPerWheelStep : -rowsPerWheelStep);
        });
        // Called during a layout pass, so the rows change afterwards.
        viewport->setRepositioned([this] {
            const double height = viewport->size().y;
            if (fitPending)
                return;
            fitPending = true;
            this->defer([this, height] {
                fitPending = false;
                fit(height);
            });
        });

        column = Hyprtoolkit::CColumnLayoutBuilder::begin()->gap(static_cast<std::size_t>(rowGap))->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        viewport->addChild(column);

        position = Hyprtoolkit::CTextBuilder::begin()->text(" ")->fontSize(CFontSize{CFontSize::HT_FONT_SMALL})->color(text)->a(0.6F)->commence();

        root->addChild(viewport);
        root->addChild(position);
    }

    void RowList::setRows(std::vector<Row> newRows) {
        rows  = std::move(newRows);
        first = std::min(first, rows.size() > fitting ? rows.size() - fitting : std::size_t{0});
        fill();
    }

    RowList::Shown RowList::makeRow() const {
        const auto text = [p = palette] { return p->m_colors.text; };
        Shown      made;
        made.row  = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(12)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, rowHeight}})->commence();
        made.keys = Hyprtoolkit::CTextBuilder::begin()
                        ->text(" ")
                        ->fontFamily(std::string{palette->m_vars.fontFamilyMonospace})
                        ->color(text)
                        ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {keysWidth, 1.0}})
                        ->commence();
        made.action = Hyprtoolkit::CTextBuilder::begin()->text(" ")->color(text)->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {1.0, 1.0}})->commence();
        made.action->setGrow(true, false);
        made.notes = Hyprtoolkit::CTextBuilder::begin()
                         ->text(" ")
                         ->fontSize(CFontSize{CFontSize::HT_FONT_SMALL})
                         ->color(text)
                         ->a(0.6F)
                         ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {notesWidth, 1.0}})
                         ->commence();
        made.row->addChild(made.keys);
        made.row->addChild(made.action);
        made.row->addChild(made.notes);
        return made;
    }

    void RowList::fit(double height) {
        fitting = static_cast<std::size_t>(std::max(0.0, std::floor((height + rowGap) / (rowHeight + rowGap))));

        // Row elements are made once and kept; ones that do not fit are only detached.
        while (shown.size() < fitting)
            shown.push_back(makeRow());
        for (std::size_t index = 0; index < shown.size(); ++index) {
            column->removeChild(shown[index].row);
            if (index < fitting)
                column->addChild(shown[index].row);
        }

        first = std::min(first, rows.size() > fitting ? rows.size() - fitting : std::size_t{0});
        fill();
    }

    void RowList::scrollBy(std::ptrdiff_t delta) {
        const auto last  = static_cast<std::ptrdiff_t>(rows.size() > fitting ? rows.size() - fitting : 0);
        const auto moved = std::clamp(static_cast<std::ptrdiff_t>(first) + delta, std::ptrdiff_t{0}, last);
        if (static_cast<std::size_t>(moved) == first)
            return;
        first = static_cast<std::size_t>(moved);
        fill();
    }

    void RowList::fill() {
        for (std::size_t index = 0; index < std::min(fitting, shown.size()); ++index) {
            const auto  at    = first + index;
            const auto* row   = at < rows.size() ? &rows[at] : nullptr;
            // An empty text is laid out as nothing, so blank cells hold a space.
            const auto  value = [](const std::string& text) { return text.empty() ? std::string{" "} : text; };
            shown[index].keys->setText(row != nullptr ? value(row->keys) : " ");
            shown[index].action->setText(row != nullptr ? value(row->action) : " ");
            shown[index].notes->setText(row != nullptr ? value(row->notes) : " ");
        }

        if (rows.size() <= fitting || fitting == 0)
            position->setText(" ");
        else
            position->setText(std::format("Rows {} to {} of {}. Scroll for more.", first + 1, std::min(first + fitting, rows.size()), rows.size()));
    }

}
