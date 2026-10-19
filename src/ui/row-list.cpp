// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/row-list.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace ui {

    namespace {

        using Hyprtoolkit::CDynamicSize;
        using Hyprtoolkit::CFontSize;

        constexpr double rowGap           = 2;
        constexpr int    rowsPerWheelStep = 3;

    }

    std::string escapeMarkup(std::string_view text) {
        std::string escaped;
        escaped.reserve(text.size());
        for (const char character : text) {
            switch (character) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                default: escaped += character;
            }
        }
        return escaped;
    }

    RowList::RowList(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer, std::vector<Column> columns,
                     double rowHeight) :
        palette{std::move(palette)}, defer{std::move(defer)}, columns{std::move(columns)}, rowHeight{rowHeight} {
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
        first = std::min(first, lastFirst());
        fill();
    }

    void RowList::setClicked(std::function<void(std::size_t)> function) {
        clicked = std::move(function);
    }

    void RowList::scrollTo(std::size_t row) {
        if (fitting == 0) {
            wanted = row;
            return;
        }
        first = std::min(row > fitting / 3 ? row - fitting / 3 : 0, lastFirst());
        fill();
    }

    std::size_t RowList::lastFirst() const {
        return rows.size() > fitting ? rows.size() - fitting : 0;
    }

    RowList::Shown RowList::makeRow(std::size_t index) {
        const auto text = [p = palette] { return p->m_colors.text; };
        Shown      made;
        made.row = Hyprtoolkit::CRowLayoutBuilder::begin()->gap(12)->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.0, rowHeight}})->commence();

        // The row takes the pointer, so the wheel has to scroll from here too.
        made.row->setReceivesMouse(true);
        made.row->setMouseAxis([this](Hyprtoolkit::Input::eAxisAxis axis, float delta) {
            if (axis == Hyprtoolkit::Input::AXIS_AXIS_VERTICAL && delta != 0.F)
                scrollBy(delta > 0.F ? rowsPerWheelStep : -rowsPerWheelStep);
        });
        made.row->setMouseButton([this, index](Hyprtoolkit::Input::eMouseButton button, bool pressed) {
            const auto at = first + index;
            if (button != Hyprtoolkit::Input::MOUSE_BUTTON_LEFT || !pressed || !clicked || at >= rows.size())
                return;
            // What a click does may change the page this row is on, so not from inside its handler.
            defer([this, at] {
                if (clicked)
                    clicked(at);
            });
        });

        for (const auto& spec : columns) {
            auto builder = Hyprtoolkit::CTextBuilder::begin()->text(" ")->color(text)->size(
                CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {spec.width > 0 ? spec.width : 1.0, 1.0}});
            if (spec.monospace)
                builder->fontFamily(std::string{palette->m_vars.fontFamilyMonospace});
            if (spec.secondary)
                builder->fontSize(CFontSize{CFontSize::HT_FONT_SMALL})->a(0.6F);
            auto cell = builder->commence();
            if (spec.width <= 0)
                cell->setGrow(true, false);
            made.row->addChild(cell);
            made.cells.push_back(std::move(cell));
        }
        return made;
    }

    void RowList::fit(double height) {
        fitting = static_cast<std::size_t>(std::max(0.0, std::floor((height + rowGap) / (rowHeight + rowGap))));

        // Row elements are made once and kept; ones that do not fit are only detached.
        while (shown.size() < fitting)
            shown.push_back(makeRow(shown.size()));
        for (std::size_t index = 0; index < shown.size(); ++index) {
            column->removeChild(shown[index].row);
            if (index < fitting)
                column->addChild(shown[index].row);
        }

        if (wanted && fitting != 0) {
            const auto row = *wanted;
            wanted.reset();
            scrollTo(row);
            return;
        }
        first = std::min(first, lastFirst());
        fill();
    }

    void RowList::scrollBy(std::ptrdiff_t delta) {
        const auto moved = std::clamp(static_cast<std::ptrdiff_t>(first) + delta, std::ptrdiff_t{0}, static_cast<std::ptrdiff_t>(lastFirst()));
        if (static_cast<std::size_t>(moved) == first)
            return;
        first = static_cast<std::size_t>(moved);
        fill();
    }

    void RowList::fill() {
        for (std::size_t index = 0; index < std::min(fitting, shown.size()); ++index) {
            const auto  at  = first + index;
            const auto* row = at < rows.size() ? &rows[at] : nullptr;
            for (std::size_t cell = 0; cell < columns.size(); ++cell) {
                const std::string_view value = row != nullptr && cell < row->cells.size() ? std::string_view{row->cells[cell]} : std::string_view{};
                // An empty text is laid out as nothing, so blank cells hold a space.
                shown[index].cells[cell]->setText(value.empty() ? std::string{" "} : columns[cell].markup ? std::string{value} : escapeMarkup(value));
            }
        }

        if (rows.size() <= fitting || fitting == 0)
            position->setText(" ");
        else
            position->setText(std::format("Rows {} to {} of {}. Scroll for more.", first + 1, std::min(first + fitting, rows.size()), rows.size()));
    }

}
