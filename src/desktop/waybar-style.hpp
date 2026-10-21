// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "foundation/environment.hpp"

#include <array>
#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace desktop {

    // The colours of a Waybar style, as #rrggbb. Each comes from a published palette.
    struct Palette {
        std::string_view name;
        bool             dark = true;
        // The bar itself.
        std::string_view base;
        // Text on the accent.
        std::string_view onAccent;
        // Borders and the hovered workspace.
        std::string_view surface;
        // Empty workspaces and the tooltip's border.
        std::string_view raised;
        // Muted audio.
        std::string_view muted;
        // Workspaces with windows, and the calendar's weekdays.
        std::string_view subtle;
        std::string_view text;
        // The active workspace.
        std::string_view accent;
        std::string_view audio;
        std::string_view network;
        // Urgent workspaces, no network, and today in the calendar.
        std::string_view alert;
        // The nearest of GNOME's accent colours, for GTK apps.
        std::string_view                 gnomeAccent;
    };

    // How the bar sits on the screen.
    enum class Shape {
        // Three rounded groups that float apart: workspaces, the clock, the rest.
        islands,
        // One rounded bar that floats, as far from the edges as the windows.
        floating,
        // A solid bar from edge to edge.
        fullWidth,
    };

    struct ShapeInfo {
        Shape            shape;
        std::string_view name;
        std::string_view summary;
    };

    [[nodiscard]] std::span<const Palette>   waybarPalettes();
    [[nodiscard]] std::span<const ShapeInfo> waybarShapes();

    // How round the bar is and how far it sits from the screen's edges: the windows' own,
    // set on the Windows page, so that the bar lines up with them. The defaults are what
    // the studio's styles had before they followed the windows.
    struct BarGeometry {
        // The corners of the bar, or of its islands; the workspace buttons' follow.
        int rounding = 10;
        // From the sides, and from the top.
        int edgeGap  = 20;
        // From the top: the edge gap when the bar follows the windows; half of it in
        // files the studio wrote before.
        int topGap   = 10;

        friend bool operator==(const BarGeometry&, const BarGeometry&) = default;
    };

    // A complete style: the shape, the palette, and the geometry.
    struct WaybarStyle {
        Shape          shape    = Shape::islands;
        const Palette* palette  = nullptr;
        BarGeometry    geometry = {};

        bool operator==(const WaybarStyle&) const = default;
    };

    // Waybar's config and stylesheet for the style: plain Waybar files with nothing of the
    // studio's in them.
    [[nodiscard]] std::string waybarConfigText(const WaybarStyle& style);
    [[nodiscard]] std::string waybarStyleText(const WaybarStyle& style);

    // The folder the studio writes Waybar's files to: the user's folder Waybar reads now,
    // or, when it reads the system's example, $XDG_CONFIG_HOME/waybar or ~/.config/waybar.
    [[nodiscard]] std::filesystem::path waybarUserFolder(const foundation::Environment& environment, const std::filesystem::path& systemFolder = "/etc/xdg");

    // The config and the stylesheet that `style` would write, with what is there now.
    [[nodiscard]] std::expected<std::vector<configuration::FileChange>, std::string> planWaybarStyle(const WaybarStyle& style, const foundation::Environment& environment,
                                                                                                    const std::filesystem::path& systemFolder = "/etc/xdg");

    // The studio's style that Waybar's files hold now, if they hold one exactly.
    // Only styles with `geometry` are looked for, since the files hold it too.
    [[nodiscard]] std::optional<WaybarStyle> currentWaybarStyle(const foundation::Environment& environment, const BarGeometry& geometry = {},
                                                                const std::filesystem::path& systemFolder = "/etc/xdg");

}
