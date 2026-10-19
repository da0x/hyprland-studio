// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace syntax {

    // What a piece of source is, as far as colouring it goes.
    enum class Kind {
        comment,
        keyword,
        string,
        number,
        constant,
        function,
        field,
        operation,
        punctuation,
    };

    // A run of bytes of one kind: [begin, end).
    struct Span {
        std::size_t begin = 0;
        std::size_t end   = 0;
        Kind        kind  = Kind::comment;

        friend bool operator==(const Span&, const Span&) = default;
    };

    // The coloured runs of a Lua source, in order and never overlapping. Bytes outside
    // every span are plain. A source with syntax errors is still coloured as far as the
    // parser can make sense of it.
    [[nodiscard]] std::vector<Span> highlightLua(std::string_view source);

}
