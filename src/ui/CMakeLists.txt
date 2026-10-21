# SPDX-License-Identifier: GPL-3.0-or-later
add_library(hyprland-studio-ui STATIC accessibility-page.cpp console-page.cpp event-log-page.cpp live-tree-page.cpp options-page.cpp activate-page.cpp choice-cards.cpp windows-page.cpp bar-page.cpp bar-preview.cpp keybindings-page.cpp main-window.cpp markup.cpp program-switch.cpp restyle.cpp row-list.cpp source-view.cpp terminal-page.cpp texts.cpp theme-page.cpp wallpaper-page.cpp widgets.cpp)
target_include_directories(hyprland-studio-ui PUBLIC ${PROJECT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/generated)

# Whether hyprtoolkit's scroll area can be used; see toolkit.hpp.in.
if(HYPRLAND_STUDIO_FIXED_HYPRTOOLKIT)
    set(HYPRLAND_STUDIO_FIXED_HYPRTOOLKIT_VALUE true)
else()
    set(HYPRLAND_STUDIO_FIXED_HYPRTOOLKIT_VALUE false)
endif()
configure_file(toolkit.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/generated/ui/toolkit.hpp @ONLY)
target_link_libraries(hyprland-studio-ui PUBLIC hyprland-studio-application PRIVATE hyprland-studio-activation PkgConfig::hyprtoolkit hyprland-studio-desktop hyprland-studio-syntax)
