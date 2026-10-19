# SPDX-License-Identifier: GPL-3.0-or-later
add_library(hyprland-studio-foundation STATIC file-descriptor.cpp modifiers.cpp)
target_include_directories(hyprland-studio-foundation PUBLIC ${PROJECT_SOURCE_DIR}/src)
