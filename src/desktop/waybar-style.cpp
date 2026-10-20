// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/waybar-style.hpp"

#include "desktop/waybar.hpp"

#include <algorithm>
#include <array>
#include <format>

namespace desktop {

    namespace {

        // The palettes' colours as their authors publish them: Catppuccin, Rosé Pine,
        // Tokyo Night, Nord and Gruvbox are all under the MIT licence.
        constexpr std::array palettes{
            Palette{.name = "Catppuccin Mocha", .dark = true, .base = "#1e1e2e", .onAccent = "#11111b", .surface = "#313244", .raised = "#45475a", .muted = "#6c7086",
                    .subtle = "#a6adc8", .text = "#cdd6f4", .accent = "#cba6f7", .audio = "#89b4fa", .network = "#94e2d5", .alert = "#f38ba8",
                    .terminal = {{"#45475a", "#f38ba8", "#a6e3a1", "#f9e2af", "#89b4fa", "#f5c2e7", "#94e2d5", "#bac2de", "#585b70", "#f38ba8", "#a6e3a1", "#f9e2af", "#89b4fa", "#f5c2e7", "#94e2d5", "#a6adc8"}}, .cursor = "#f5e0dc", .selection = "#585b70", .gnomeAccent = "purple"},
            Palette{.name = "Catppuccin Latte", .dark = false, .base = "#eff1f5", .onAccent = "#eff1f5", .surface = "#ccd0da", .raised = "#bcc0cc", .muted = "#9ca0b0",
                    .subtle = "#6c6f85", .text = "#4c4f69", .accent = "#8839ef", .audio = "#1e66f5", .network = "#179299", .alert = "#d20f39",
                    .terminal = {{"#5c5f77", "#d20f39", "#40a02b", "#df8e1d", "#1e66f5", "#ea76cb", "#179299", "#acb0be", "#6c6f85", "#d20f39", "#40a02b", "#df8e1d", "#1e66f5", "#ea76cb", "#179299", "#bcc0cc"}}, .cursor = "#dc8a78", .selection = "#acb0be", .gnomeAccent = "purple", .terminalFrom = "Catppuccin Mocha"},
            Palette{.name = "Rosé Pine", .dark = true, .base = "#191724", .onAccent = "#191724", .surface = "#26233a", .raised = "#403d52", .muted = "#6e6a86",
                    .subtle = "#908caa", .text = "#e0def4", .accent = "#c4a7e7", .audio = "#9ccfd8", .network = "#ebbcba", .alert = "#eb6f92",
                    .terminal = {{"#26233a", "#eb6f92", "#31748f", "#f6c177", "#9ccfd8", "#c4a7e7", "#ebbcba", "#e0def4", "#6e6a86", "#eb6f92", "#31748f", "#f6c177", "#9ccfd8", "#c4a7e7", "#ebbcba", "#e0def4"}}, .cursor = "#524f67", .selection = "#403d52", .gnomeAccent = "purple"},
            Palette{.name = "Tokyo Night", .dark = true, .base = "#1a1b26", .onAccent = "#1a1b26", .surface = "#292e42", .raised = "#3b4261", .muted = "#565f89",
                    .subtle = "#a9b1d6", .text = "#c0caf5", .accent = "#7aa2f7", .audio = "#7dcfff", .network = "#9ece6a", .alert = "#f7768e",
                    .terminal = {{"#15161e", "#f7768e", "#9ece6a", "#e0af68", "#7aa2f7", "#bb9af7", "#7dcfff", "#a9b1d6", "#414868", "#f7768e", "#9ece6a", "#e0af68", "#7aa2f7", "#bb9af7", "#7dcfff", "#c0caf5"}}, .cursor = "#c0caf5", .selection = "#283457", .gnomeAccent = "blue"},
            Palette{.name = "Nord", .dark = true, .base = "#2e3440", .onAccent = "#2e3440", .surface = "#3b4252", .raised = "#4c566a", .muted = "#616e88",
                    .subtle = "#d8dee9", .text = "#eceff4", .accent = "#88c0d0", .audio = "#81a1c1", .network = "#a3be8c", .alert = "#bf616a",
                    .terminal = {{"#3b4252", "#bf616a", "#a3be8c", "#ebcb8b", "#81a1c1", "#b48ead", "#88c0d0", "#e5e9f0", "#4c566a", "#bf616a", "#a3be8c", "#ebcb8b", "#81a1c1", "#b48ead", "#8fbcbb", "#eceff4"}}, .cursor = "#d8dee9", .selection = "#434c5e", .gnomeAccent = "teal"},
            Palette{.name = "Gruvbox Dark", .dark = true, .base = "#282828", .onAccent = "#282828", .surface = "#3c3836", .raised = "#504945", .muted = "#7c6f64",
                    .subtle = "#a89984", .text = "#ebdbb2", .accent = "#fabd2f", .audio = "#83a598", .network = "#8ec07c", .alert = "#fb4934",
                    .terminal = {{"#282828", "#cc241d", "#98971a", "#d79921", "#458588", "#b16286", "#689d6a", "#a89984", "#928374", "#fb4934", "#b8bb26", "#fabd2f", "#83a598", "#d3869b", "#8ec07c", "#ebdbb2"}}, .cursor = "#ebdbb2", .selection = "#504945", .gnomeAccent = "yellow"},
        };

        constexpr std::array shapes{
            ShapeInfo{Shape::islands, "Islands", "Three rounded groups that float apart."},
            ShapeInfo{Shape::floating, "Floating", "One rounded bar, as far from the edges as your windows."},
            ShapeInfo{Shape::fullWidth, "Full width", "A solid bar from edge to edge."},
        };

        std::string_view shapeName(Shape shape) {
            for (const auto& info : shapes)
                if (info.shape == shape)
                    return info.name;
            return {};
        }

        bool under(const std::filesystem::path& file, const std::filesystem::path& folder) {
            const auto relative = file.lexically_relative(folder);
            return !relative.empty() && !relative.native().starts_with("..");
        }

        // What every shape shares: the font, the workspaces and the modules' colours.
        constexpr std::string_view moduleRules = R"(
#workspaces button {
    padding: 0 10px;
    margin: 5px 2px;
    border-radius: 7px;
    color: @subtle;
    background: transparent;
    background-image: none;
    box-shadow: none;
    text-shadow: none;
}

#workspaces button.empty {
    color: @raised;
}

#workspaces button:hover {
    background: @surface;
    color: @text;
}

#workspaces button.active {
    background: @accent;
    color: @on_accent;
}

#workspaces button.urgent {
    background: @alert;
    color: @on_accent;
}

#clock {
    padding: 0 12px;
    font-weight: 600;
}

#pulseaudio,
#network,
#tray {
    padding: 0 10px;
}

#pulseaudio {
    color: @audio;
}

#pulseaudio.muted {
    color: @muted;
}

#network {
    color: @network;
}

#network.disconnected {
    color: @alert;
}

#tray > .needs-attention {
    background: @alert;
    border-radius: 7px;
}

tooltip {
    background: @base;
    border: 1px solid @raised;
    border-radius: 8px;
}

tooltip label {
    color: @text;
}
)";

        std::string_view shapeRules(Shape shape) {
            switch (shape) {
                case Shape::islands:
                    return R"(
window#waybar {
    background: transparent;
    color: @text;
}

.modules-left,
.modules-center,
.modules-right {
    background: alpha(@base, 0.92);
    border: 1px solid @surface;
    border-radius: 10px;
    padding: 0 6px;
}
)";
                case Shape::floating:
                    return R"(
window#waybar {
    background: alpha(@base, 0.92);
    color: @text;
    border: 1px solid @surface;
    border-radius: 10px;
}

.modules-left,
.modules-right {
    padding: 0 6px;
}
)";
                case Shape::fullWidth:
                    return R"(
window#waybar {
    background: @base;
    color: @text;
    border-bottom: 1px solid @surface;
}

.modules-left,
.modules-right {
    padding: 0 6px;
}
)";
            }
            return {};
        }

    }

    std::span<const Palette> waybarPalettes() {
        return palettes;
    }

    std::span<const ShapeInfo> waybarShapes() {
        return shapes;
    }

    std::string waybarConfigText(const WaybarStyle& style) {
        const auto& palette  = *style.palette;
        const bool  floating = style.shape != Shape::fullWidth;
        return std::format(R"({{
    "layer": "top",
    "position": "top",
    "height": {},
    "margin-top": {},
    "margin-left": {},
    "margin-right": {},
    "spacing": 8,
    "reload_style_on_change": true,
    "modules-left": [
        "hyprland/workspaces"
    ],
    "modules-center": [
        "clock"
    ],
    "modules-right": [
        "pulseaudio",
        "network",
        "tray"
    ],
    "hyprland/workspaces": {{
        "format": "{{name}}",
        "on-click": "activate",
        "sort-by-number": true,
        "persistent-workspaces": {{
            "*": 5
        }}
    }},
    "clock": {{
        "format": "{{:%a %d %b   %H:%M}}",
        "tooltip-format": "<tt>{{calendar}}</tt>",
        "calendar": {{
            "mode": "month",
            "format": {{
                "months": "<span color='{}'><b>{{}}</b></span>",
                "weekdays": "<span color='{}'>{{}}</span>",
                "today": "<span color='{}'><b><u>{{}}</u></b></span>"
            }}
        }}
    }},
    "pulseaudio": {{
        "format": "{{icon}}  {{volume}}%",
        "format-muted": "{}  muted",
        "format-icons": {{
            "headphone": "{}",
            "default": [
                "{}",
                "{}",
                "{}"
            ]
        }},
        "scroll-step": 5,
        "on-click": "pavucontrol",
        "on-click-right": "wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle"
    }},
    "network": {{
        "format-wifi": "{}  {{essid}}",
        "format-ethernet": "{}  wired",
        "format-disconnected": "{}  offline",
        "tooltip-format": "{{ifname}}  {{ipaddr}}/{{cidr}}",
        "on-click": "nm-connection-editor"
    }},
    "tray": {{
        "icon-size": 18,
        "spacing": 10
    }}
}}
)",
                           floating ? 38 : 34, floating ? 10 : 0, floating ? 20 : 0, floating ? 20 : 0, palette.accent, palette.subtle, palette.alert, "\U000F075F", "\U000F02CB", "\U000F057F", "\U000F0580", "\U000F057E", "\U000F0928", "\U000F0200", "\U000F092D");
    }

    std::string waybarStyleText(const WaybarStyle& style) {
        const auto& palette = *style.palette;
        auto        text    = std::format(R"(/* {}, {}. */

@define-color base      {};
@define-color on_accent {};
@define-color surface   {};
@define-color raised    {};
@define-color muted     {};
@define-color subtle    {};
@define-color text      {};
@define-color accent    {};
@define-color audio     {};
@define-color network   {};
@define-color alert     {};

* {{
    font-family: "Adwaita Sans", "Symbols Nerd Font";
    font-size: 15px;
    min-height: 0;
    border: none;
    border-radius: 0;
}}
)",
                                      palette.name, shapeName(style.shape), palette.base, palette.onAccent, palette.surface, palette.raised, palette.muted, palette.subtle,
                                      palette.text, palette.accent, palette.audio, palette.network, palette.alert);
        text += shapeRules(style.shape);
        text += moduleRules;
        return text;
    }

    std::filesystem::path waybarUserFolder(const foundation::Environment& environment, const std::filesystem::path& systemFolder) {
        if (const auto config = waybarConfig(environment, systemFolder); config && !under(*config, systemFolder))
            return config->parent_path();
        if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"); home && std::filesystem::path{*home}.is_absolute())
            return std::filesystem::path{*home} / "waybar";
        return std::filesystem::path{foundation::variable(environment, "HOME").value_or("")} / ".config" / "waybar";
    }

    std::expected<std::vector<configuration::FileChange>, std::string> planWaybarStyle(const WaybarStyle& style, const foundation::Environment& environment,
                                                                                      const std::filesystem::path& systemFolder) {
        const auto folder = waybarUserFolder(environment, systemFolder);
        // Keep the name of a config that is already there: Waybar reads config before config.jsonc.
        auto configFile = folder / "config.jsonc";
        if (const auto config = waybarConfig(environment, systemFolder); config && config->parent_path() == folder)
            configFile = *config;

        std::vector<configuration::FileChange> changes;
        for (auto& [file, text] : {std::pair{configFile, waybarConfigText(style)}, std::pair{folder / "style.css", waybarStyleText(style)}}) {
            auto change = configuration::planFile(file, text);
            if (!change)
                return std::unexpected{change.error()};
            changes.push_back(std::move(*change));
        }
        return changes;
    }

    std::optional<WaybarStyle> currentWaybarStyle(const foundation::Environment& environment, const std::filesystem::path& systemFolder) {
        for (const auto& info : shapes) {
            for (const auto& palette : palettes) {
                const WaybarStyle style{info.shape, &palette};
                const auto        planned = planWaybarStyle(style, environment, systemFolder);
                if (planned && std::ranges::none_of(*planned, [](const auto& change) { return change.changes(); }))
                    return style;
            }
        }
        return std::nullopt;
    }

}
