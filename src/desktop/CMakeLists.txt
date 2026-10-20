# SPDX-License-Identifier: GPL-3.0-or-later
pkg_check_modules(fontconfig REQUIRED IMPORTED_TARGET fontconfig)

add_library(hyprland-studio-desktop STATIC clipboard.cpp system.cpp waybar.cpp)
target_include_directories(hyprland-studio-desktop PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-desktop PUBLIC hyprland-studio-foundation PRIVATE PkgConfig::fontconfig)
