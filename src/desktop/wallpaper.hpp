// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "configuration/file-change.hpp"
#include "desktop/waybar.hpp"
#include "foundation/environment.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace desktop {

    // How hyprpaper fits an image to the screen.
    enum class FitMode {
        cover,
        contain,
        fill,
        tile,
    };

    struct FitInfo {
        FitMode          mode;
        // As hyprpaper.conf writes it.
        std::string_view word;
        std::string_view name;
    };

    [[nodiscard]] std::span<const FitInfo> fitModes();

    struct Wallpaper {
        std::filesystem::path image;
        FitMode               fit = FitMode::cover;

        bool operator==(const Wallpaper&) const = default;
    };

    // hyprpaper, which draws the wallpaper.
    [[nodiscard]] std::vector<Requirement> wallpaperRequirements(const foundation::Environment& environment);

    // $XDG_CONFIG_HOME/hypr/hyprpaper.conf, or ~/.config/hypr/hyprpaper.conf.
    [[nodiscard]] std::filesystem::path hyprpaperConfigFile(const foundation::Environment& environment);

    // The wallpaper hyprpaper.conf sets for every monitor without one of its own: the
    // wallpaper block whose monitor is empty. `home` expands a path that starts with ~.
    [[nodiscard]] std::optional<Wallpaper> fallbackWallpaper(std::string_view config, const std::filesystem::path& home);

    // The monitors hyprpaper.conf gives a wallpaper of their own, which the fallback does not change.
    [[nodiscard]] std::vector<std::string> monitorsWithOwnWallpaper(std::string_view config);

    // `config` with the fallback block's path and fit_mode set to `wallpaper`, and every
    // other byte as it was. A block is added at the end when there is none. Refuses the
    // format from before hyprpaper 0.8, which used `wallpaper = monitor,path` lines.
    [[nodiscard]] std::expected<std::string, std::string> withFallbackWallpaper(std::string_view config, const Wallpaper& wallpaper);

    // The change to hyprpaper.conf that sets `wallpaper` for every monitor.
    [[nodiscard]] std::expected<configuration::FileChange, std::string> planWallpaper(const Wallpaper& wallpaper, const foundation::Environment& environment);

    // The arguments that make `hyprctl` show `wallpaper` on every monitor at once, through
    // hyprpaper's own socket, without writing anything. Refuses a path with a comma,
    // which hyprctl would split.
    [[nodiscard]] std::expected<std::vector<std::string>, std::string> showWallpaperArguments(const Wallpaper& wallpaper);

    // The fallback wallpaper hyprpaper.conf sets now.
    [[nodiscard]] std::optional<Wallpaper> currentWallpaper(const foundation::Environment& environment);

    // Images in the folders wallpapers are usually kept in: ~/Pictures/Wallpapers,
    // ~/Pictures, ~/.local/share/backgrounds, and then `systemFolders` and the folders in
    // them. In a folder called hypr only Hyprland's own wall*.png count.
    [[nodiscard]] std::vector<std::filesystem::path> findWallpapers(const foundation::Environment&     environment,
                                                                    std::span<const std::filesystem::path> systemFolders);

}
