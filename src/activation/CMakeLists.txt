# SPDX-License-Identifier: GPL-3.0-or-later
find_package(nlohmann_json 3.11 REQUIRED)

add_library(hyprland-studio-activation STATIC activator.cpp effects.cpp record.cpp)
target_include_directories(hyprland-studio-activation PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-activation PUBLIC hyprland-studio-configuration PRIVATE nlohmann_json::nlohmann_json hyprland-studio-desktop hyprland-studio-syntax)
