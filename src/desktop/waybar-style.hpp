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
        // The 16 terminal colours, color0 to color15, as the palette's authors publish
        // them for terminals, and the cursor and selection.
        std::array<std::string_view, 16> terminal{};
        std::string_view                 cursor;
        std::string_view                 selection;
        // The nearest of GNOME's accent colours, for GTK apps.
        std::string_view                 gnomeAccent;
        // For a light palette, the dark one from the same family that terminals use:
        // most terminal programs assume a dark background and print in white.
        std::string_view                 terminalFrom;
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

    // A complete style: the shape and the palette.
    struct WaybarStyle {
        Shape          shape   = Shape::islands;
        const Palette* palette = nullptr;

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
    [[nodiscard]] std::optional<WaybarStyle> currentWaybarStyle(const foundation::Environment& environment, const std::filesystem::path& systemFolder = "/etc/xdg");

}
