// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/Text.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace ui {

    // A scrolling list of three-column rows: keys, what they do, and notes.
    //
    // It keeps only as many row elements as fit and scrolls by changing their text, so a
    // list of thousands costs the same as a screenful. It does not use hyprtoolkit's scroll
    // area: in 0.6.0 that schedules a layout pass while the window is walking its scheduled
    // passes (IToolkitWindow::onPreRender), which corrupts the walk and crashes. For the same
    // reason, rows are added and removed only from `defer`, never during a layout pass.
    class RowList {
      public:
        struct Row {
            std::string keys;
            std::string action;
            std::string notes;
        };

        RowList(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer);

        [[nodiscard]] Hyprutils::Memory::CSharedPointer<Hyprtoolkit::IElement> element() const { return root; }

        void setRows(std::vector<Row> rows);

      private:
        struct Shown {
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CRowLayoutElement> row;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>      keys;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>      action;
            Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>      notes;
        };

        void                                                         fit(double height);
        void                                                         scrollBy(std::ptrdiff_t rows);
        void                                                         fill();
        Shown                                                        makeRow() const;

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette>     palette;
        std::function<void(std::function<void()>)>                   defer;

        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> root;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CNullElement>         viewport;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CColumnLayoutElement> column;
        Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CTextElement>         position;

        std::vector<Shown>                                           shown;
        std::vector<Row>                                             rows;
        std::size_t                                                  first   = 0;
        std::size_t                                                  fitting = 0;
        bool                                                         fitPending = false;
    };

}
