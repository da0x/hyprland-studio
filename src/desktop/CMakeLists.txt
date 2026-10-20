# SPDX-License-Identifier: GPL-3.0-or-later
pkg_check_modules(fontconfig REQUIRED IMPORTED_TARGET fontconfig)
find_package(nlohmann_json 3.11 REQUIRED)

add_library(hyprland-studio-desktop STATIC accessibility.cpp clipboard.cpp process.cpp colour.cpp gsettings.cpp system.cpp terminal-theme.cpp terminal-themes.cpp theme.cpp wallpaper.cpp waybar-style.cpp waybar.cpp)
target_include_directories(hyprland-studio-desktop PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-desktop PUBLIC hyprland-studio-configuration hyprland-studio-foundation PRIVATE PkgConfig::fontconfig nlohmann_json::nlohmann_json hyprland-studio-syntax)
# terminal-themes.cpp embeds the bundled themes; rebuild it when one changes.
file(GLOB terminalThemeFiles CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/terminal-themes/*.conf)
set_property(SOURCE terminal-themes.cpp APPEND PROPERTY OBJECT_DEPENDS ${terminalThemeFiles})
