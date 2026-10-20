// SPDX-License-Identifier: GPL-3.0-or-later
#include "desktop/wallpaper.hpp"

#include "desktop/system.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <fstream>
#include <set>
#include <sstream>
#include <system_error>

namespace desktop {

    namespace {

        constexpr std::array fits{
            FitInfo{FitMode::cover, "cover", "Cover"},
            FitInfo{FitMode::contain, "contain", "Contain"},
            FitInfo{FitMode::fill, "fill", "Fill"},
            FitInfo{FitMode::tile, "tile", "Tile"},
        };

        std::string_view trim(std::string_view text) {
            while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
                text.remove_prefix(1);
            while (!text.empty() && (text.back() == ' ' || text.back() == '\t' || text.back() == '\r'))
                text.remove_suffix(1);
            return text;
        }

        // Where a value sits in the text, so it can be replaced without touching the rest.
        struct Value {
            std::size_t      begin = 0;
            std::size_t      end   = 0;
            std::string_view text;
        };

        struct Block {
            std::optional<Value> monitor;
            std::optional<Value> path;
            std::optional<Value> fit;
            // The start of the line with the closing brace, and the indentation of the keys.
            std::size_t          close  = 0;
            std::string          indent = "    ";
        };

        struct Parsed {
            std::vector<Block> blocks;
            bool               oldFormat = false;
        };

        // hyprpaper.conf is hyprlang: `key = value` lines, `#` comments, and named blocks.
        // Only what the studio needs is read: the wallpaper blocks' keys.
        Parsed parse(std::string_view config) {
            Parsed                parsed;
            std::optional<Block>  open;
            std::size_t           at = 0;
            while (at < config.size()) {
                const auto end   = std::min(config.find('\n', at), config.size());
                const auto line  = config.substr(at, end - at);
                auto       code  = line.substr(0, line.find('#'));
                const auto inner = trim(code);
                if (!open) {
                    if (inner == "wallpaper {" || inner == "wallpaper{")
                        open = Block{};
                    else if (const auto equals = inner.find('='); equals != std::string_view::npos) {
                        const auto key = trim(inner.substr(0, equals));
                        if (key == "wallpaper" || key == "preload")
                            parsed.oldFormat = true;
                    }
                } else if (inner == "}") {
                    open->close = at;
                    parsed.blocks.push_back(*open);
                    open.reset();
                } else if (const auto equals = inner.find('='); equals != std::string_view::npos) {
                    const auto key        = trim(inner.substr(0, equals));
                    const auto keyAt      = line.find(key);
                    const auto valueStart = line.find('=', keyAt) + 1;
                    auto       value      = trim(code.substr(valueStart));
                    // The value's own place in the line, after any spaces.
                    const auto valueAt    = value.empty() ? valueStart : code.find(value, valueStart);
                    const Value found{at + valueAt, at + valueAt + value.size(), value};
                    open->indent = std::string{line.substr(0, keyAt)};
                    if (key == "monitor")
                        open->monitor = found;
                    else if (key == "path")
                        open->path = found;
                    else if (key == "fit_mode")
                        open->fit = found;
                }
                at = end + 1;
            }
            return parsed;
        }

        bool isFallback(const Block& block) {
            return !block.monitor || block.monitor->text.empty();
        }

        std::filesystem::path homeOf(const foundation::Environment& environment) {
            return foundation::variable(environment, "HOME").value_or("");
        }

        std::optional<std::string> readFile(const std::filesystem::path& file) {
            std::ifstream stream{file, std::ios::binary};
            if (!stream)
                return std::nullopt;
            std::stringstream text;
            text << stream.rdbuf();
            return text.str();
        }

        bool isImage(const std::filesystem::path& file) {
            auto extension = file.extension().string();
            std::ranges::transform(extension, extension.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
            constexpr std::array images{".png", ".jpg", ".jpeg", ".webp", ".jxl"};
            return std::ranges::contains(images, std::string_view{extension});
        }

    }

    std::span<const FitInfo> fitModes() {
        return fits;
    }

    std::vector<Requirement> wallpaperRequirements(const foundation::Environment& environment) {
        Requirement hyprpaper{
            .name     = "hyprpaper",
            .purpose  = "Hyprland's own wallpaper program. Without it, Hyprland shows its default wallpaper.",
            .upstream = "https://github.com/hyprwm/hyprpaper",
            .package  = "hyprpaper",
            .found    = std::nullopt,
        };
        if (const auto program = findProgram("hyprpaper", foundation::variable(environment, "PATH").value_or("")))
            hyprpaper.found = program->string();
        return {std::move(hyprpaper)};
    }

    std::filesystem::path hyprpaperConfigFile(const foundation::Environment& environment) {
        if (const auto home = foundation::variable(environment, "XDG_CONFIG_HOME"); home && std::filesystem::path{*home}.is_absolute())
            return std::filesystem::path{*home} / "hypr" / "hyprpaper.conf";
        return homeOf(environment) / ".config" / "hypr" / "hyprpaper.conf";
    }

    std::optional<Wallpaper> fallbackWallpaper(std::string_view config, const std::filesystem::path& home) {
        for (const auto& block : parse(config).blocks) {
            if (!isFallback(block) || !block.path)
                continue;
            std::filesystem::path image{std::string{block.path->text}};
            if (block.path->text.starts_with("~/"))
                image = home / std::string{block.path->text.substr(2)};
            Wallpaper wallpaper{image, FitMode::cover};
            if (block.fit)
                for (const auto& info : fits)
                    if (info.word == block.fit->text)
                        wallpaper.fit = info.mode;
            return wallpaper;
        }
        return std::nullopt;
    }

    std::vector<std::string> monitorsWithOwnWallpaper(std::string_view config) {
        std::vector<std::string> monitors;
        for (const auto& block : parse(config).blocks)
            if (!isFallback(block))
                monitors.emplace_back(block.monitor->text);
        return monitors;
    }

    std::expected<std::string, std::string> withFallbackWallpaper(std::string_view config, const Wallpaper& wallpaper) {
        const auto parsed = parse(config);
        if (parsed.oldFormat)
            return std::unexpected{"hyprpaper.conf is written in the format from before hyprpaper 0.8 (wallpaper = monitor,path), which the studio does not change"};

        std::string_view fitWord = "cover";
        for (const auto& info : fits)
            if (info.mode == wallpaper.fit)
                fitWord = info.word;

        const auto found = std::ranges::find_if(parsed.blocks, isFallback);
        if (found == parsed.blocks.end()) {
            std::string text{config};
            if (!text.empty() && !text.ends_with('\n'))
                text += '\n';
            if (!text.empty())
                text += '\n';
            text += std::format("wallpaper {{\n    monitor =\n    path = {}\n    fit_mode = {}\n}}\n", wallpaper.image.string(), fitWord);
            return text;
        }

        // Replaced from the end of the text backwards, so earlier offsets stay right.
        struct Edit {
            std::size_t begin;
            std::size_t end;
            std::string text;
        };
        std::vector<Edit> edits;
        const auto&       block = *found;
        if (block.path)
            edits.push_back({block.path->begin, block.path->end, wallpaper.image.string()});
        if (block.fit)
            edits.push_back({block.fit->begin, block.fit->end, std::string{fitWord}});
        std::string added;
        if (!block.path)
            added += std::format("{}path = {}\n", block.indent, wallpaper.image.string());
        if (!block.fit)
            added += std::format("{}fit_mode = {}\n", block.indent, fitWord);
        if (!added.empty())
            edits.push_back({block.close, block.close, added});
        std::ranges::sort(edits, std::ranges::greater{}, &Edit::begin);

        std::string text{config};
        for (const auto& edit : edits)
            text.replace(edit.begin, edit.end - edit.begin, edit.text);
        return text;
    }

    std::expected<configuration::FileChange, std::string> planWallpaper(const Wallpaper& wallpaper, const foundation::Environment& environment) {
        const auto file    = hyprpaperConfigFile(environment);
        const auto current = configuration::planFile(file, "");
        if (!current)
            return std::unexpected{current.error()};
        auto text = withFallbackWallpaper(current->current.value_or(""), wallpaper);
        if (!text)
            return std::unexpected{text.error()};
        return configuration::FileChange{file, current->current, std::move(*text)};
    }

    std::expected<std::vector<std::string>, std::string> showWallpaperArguments(const Wallpaper& wallpaper) {
        const auto path = wallpaper.image.string();
        if (path.contains(','))
            return std::unexpected{std::format("{} has a comma in it, which hyprctl cannot pass on", path)};
        // hyprctl's words for the fit, which differ from the config's for fill.
        std::string_view fit = "cover";
        switch (wallpaper.fit) {
            case FitMode::cover: fit = "cover"; break;
            case FitMode::contain: fit = "contain"; break;
            case FitMode::fill: fit = "stretch"; break;
            case FitMode::tile: fit = "tile"; break;
        }
        return std::vector<std::string>{"hyprpaper", "wallpaper", std::format(", {}, {}", path, fit)};
    }

    std::optional<Wallpaper> currentWallpaper(const foundation::Environment& environment) {
        const auto text = readFile(hyprpaperConfigFile(environment));
        return text ? fallbackWallpaper(*text, homeOf(environment)) : std::nullopt;
    }

    std::vector<std::filesystem::path> findWallpapers(const foundation::Environment& environment, std::span<const std::filesystem::path> systemFolders) {
        const auto                         home = homeOf(environment);
        std::vector<std::filesystem::path> found;
        std::set<std::filesystem::path>    seen;
        const auto                         look = [&found, &seen](const std::filesystem::path& folder, bool deeper) {
            std::error_code                    error;
            std::vector<std::filesystem::path> here;
            std::vector<std::filesystem::path> inside;
            for (const auto& entry : std::filesystem::directory_iterator{folder, error}) {
                if (entry.is_directory(error))
                    inside.push_back(entry.path());
                else if (entry.is_regular_file(error) && isImage(entry.path())) {
                    // Hyprland's folder also holds hyprlock's pictures; only its walls are wallpapers.
                    if (folder.filename() == "hypr" && !entry.path().filename().string().starts_with("wall"))
                        continue;
                    here.push_back(entry.path());
                }
            }
            std::ranges::sort(here);
            for (auto& file : here)
                if (seen.insert(file).second)
                    found.push_back(std::move(file));
            if (deeper) {
                std::ranges::sort(inside);
                for (const auto& folderInside : inside) {
                    std::vector<std::filesystem::path> images;
                    for (const auto& entry : std::filesystem::directory_iterator{folderInside, error})
                        if (entry.is_regular_file(error) && isImage(entry.path()))
                            images.push_back(entry.path());
                    std::ranges::sort(images);
                    for (auto& file : images)
                        if (seen.insert(file).second)
                            found.push_back(std::move(file));
                }
            }
        };
        if (!home.empty()) {
            look(home / "Pictures" / "Wallpapers", true);
            look(home / "Pictures", false);
            look(home / ".local" / "share" / "backgrounds", true);
        }
        for (const auto& folder : systemFolders)
            look(folder, true);
        return found;
    }

}
