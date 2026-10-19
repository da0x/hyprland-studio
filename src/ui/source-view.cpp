// SPDX-License-Identifier: GPL-3.0-or-later
#include "ui/source-view.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace ui {

    namespace {

        constexpr double lineHeight   = 20;
        constexpr double numberWidth  = 56;
        constexpr int    tabWidth     = 4;

        std::string hex(const Hyprtoolkit::CHyprColor& color) {
            const auto channel = [](double value) { return static_cast<int>(std::lround(std::clamp(value, 0.0, 1.0) * 255)); };
            return std::format("#{:02x}{:02x}{:02x}", channel(color.r), channel(color.g), channel(color.b));
        }

        // Colours from the user's hyprtoolkit theme, so the source matches the rest of the window.
        std::string colourOf(const Hyprtoolkit::CPalette& palette, syntax::Kind kind) {
            const auto& colors = palette.m_colors;
            switch (kind) {
                case syntax::Kind::comment: return hex(colors.text.mix(colors.base, 0.45F));
                case syntax::Kind::keyword: return hex(colors.accent);
                case syntax::Kind::string: return hex(colors.accentSecondary);
                case syntax::Kind::number:
                case syntax::Kind::constant: return hex(colors.accentSecondary.mix(colors.text, 0.4F));
                case syntax::Kind::function: return hex(colors.linkText);
                case syntax::Kind::field: return hex(colors.brightText);
                case syntax::Kind::operation:
                case syntax::Kind::punctuation: return hex(colors.text.mix(colors.base, 0.25F));
            }
            return hex(colors.text);
        }

        // One line as Pango markup: each span in its colour, tabs as spaces.
        std::string lineMarkup(std::string_view text, std::size_t begin, std::size_t end, const std::vector<syntax::Span>& spans, std::vector<syntax::Span>::const_iterator& span,
                               const Hyprtoolkit::CPalette& palette) {
            std::string markup;
            int         column = 0;
            std::size_t at     = begin;
            const auto  plain  = [&](std::size_t until) {
                for (; at < until; ++at) {
                    const char character = text[at];
                    if (character == '\t') {
                        const int spaces = tabWidth - (column % tabWidth);
                        markup.append(static_cast<std::size_t>(spaces), ' ');
                        column += spaces;
                        continue;
                    }
                    // A continuation byte of UTF-8 does not start a new column.
                    if ((static_cast<unsigned char>(character) & 0xC0U) != 0x80U)
                        ++column;
                    markup += escapeMarkup(std::string_view{&text[at], 1});
                }
            };

            while (at < end) {
                while (span != spans.end() && span->end <= at)
                    ++span;
                if (span == spans.end() || span->begin >= end) {
                    plain(end);
                    break;
                }
                plain(std::max(at, span->begin));
                const auto until = std::min(span->end, end);
                markup += std::format("<span foreground=\"{}\">", colourOf(palette, span->kind));
                plain(until);
                markup += "</span>";
            }
            return markup;
        }

    }

    SourceView::SourceView(Hyprutils::Memory::CSharedPointer<Hyprtoolkit::CPalette> palette, std::function<void(std::function<void()>)> defer) :
        palette{std::move(palette)},
        lines{this->palette, std::move(defer),
              {RowList::Column{.width = numberWidth, .monospace = true, .secondary = true, .markup = true}, RowList::Column{.width = 0, .monospace = true, .secondary = false, .markup = true}},
              lineHeight} {}

    void SourceView::show(const application::SourceFile& file, int marked, bool jump) {
        std::vector<RowList::Row> rows;
        if (!file.error.empty()) {
            rows.push_back(RowList::Row{.cells = {{}, escapeMarkup(std::format("Could not read this file: {}.", file.error))}});
            lines.setRows(std::move(rows));
            return;
        }

        const std::string_view text   = file.text;
        const auto             mark   = hex(palette->m_colors.accent);
        auto                   span   = file.spans.cbegin();
        std::size_t            begin  = 0;
        int                    number = 1;
        while (begin < text.size()) {
            auto end = text.find('\n', begin);
            if (end == std::string_view::npos)
                end = text.size();
            auto shownEnd = end;
            if (shownEnd > begin && text[shownEnd - 1] == '\r')
                --shownEnd;

            auto code = lineMarkup(text, begin, shownEnd, file.spans, span, *palette);
            auto gutter = std::format("{:>5}", number);
            if (number == marked) {
                gutter = std::format("<span foreground=\"{}\"><b>{}</b></span>", mark, gutter);
                code   = std::format("<span background=\"{}\" bgalpha=\"25%\">{} </span>", mark, code);
            }
            rows.push_back(RowList::Row{.cells = {std::move(gutter), std::move(code)}});
            begin = end + 1;
            ++number;
        }

        lines.setRows(std::move(rows));
        if (jump)
            lines.scrollTo(marked > 0 ? static_cast<std::size_t>(marked - 1) : 0);
    }

}
