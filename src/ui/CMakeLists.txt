# SPDX-License-Identifier: GPL-3.0-or-later
add_library(hyprland-studio-ui STATIC main-window.cpp)
target_include_directories(hyprland-studio-ui PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-ui PRIVATE PkgConfig::hyprtoolkit)
