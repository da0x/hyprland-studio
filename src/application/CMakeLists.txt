# SPDX-License-Identifier: GPL-3.0-or-later
find_package(nlohmann_json 3.11 REQUIRED)

add_library(hyprland-studio-application STATIC keybindings.cpp loaded-config.cpp outside-program.cpp session.cpp settings.cpp studio-folder.cpp source-file.cpp worker.cpp)
target_include_directories(hyprland-studio-application PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-application PUBLIC hyprland-studio-configuration hyprland-studio-desktop hyprland-studio-ipc hyprland-studio-syntax PRIVATE nlohmann_json::nlohmann_json)
