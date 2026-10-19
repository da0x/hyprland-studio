# SPDX-License-Identifier: GPL-3.0-or-later
add_library(hyprland-studio-ui STATIC keybindings-page.cpp main-window.cpp markup.cpp row-list.cpp source-view.cpp texts.cpp)
target_include_directories(hyprland-studio-ui PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-ui PUBLIC hyprland-studio-application PRIVATE PkgConfig::hyprtoolkit hyprland-studio-syntax)
