# SPDX-License-Identifier: GPL-3.0-or-later
add_library(hyprland-studio-application STATIC keybindings.cpp session.cpp worker.cpp)
target_include_directories(hyprland-studio-application PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-application PUBLIC hyprland-studio-configuration hyprland-studio-ipc)
