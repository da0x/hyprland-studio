// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <string>
#include <string_view>

namespace desktop {

    // A colour in sRGB, each channel from 0 to 1.
    struct Colour {
        double red   = 0;
        double green = 0;
        double blue  = 0;

        bool operator==(const Colour&) const = default;
    };

    // #rrggbb, either case; anything else is black.
    [[nodiscard]] Colour      parseColour(std::string_view hex);
    [[nodiscard]] std::string hexOf(const Colour& colour);

    // WCAG's contrast ratio of two colours, from 1 to 21.
    [[nodiscard]] double contrast(const Colour& one, const Colour& other);

    // The colour in OKLCH: lightness 0 to 1, chroma, and hue in degrees. A perceptual space,
    // so changing the lightness leaves the hue and the colourfulness as they look.
    struct Lch {
        double lightness = 0;
        double chroma    = 0;
        double hue       = 0;
    };
    [[nodiscard]] Lch    lchOf(const Colour& colour);
    // Back to sRGB; a colour outside it loses chroma until it fits.
    [[nodiscard]] Colour colourOf(const Lch& lch);

    // The colour with the least change of lightness, towards white on a dark background and
    // towards black on a light one, that has at least `ratio` contrast with `background`.
    [[nodiscard]] Colour withContrast(const Colour& colour, const Colour& background, double ratio);

}
