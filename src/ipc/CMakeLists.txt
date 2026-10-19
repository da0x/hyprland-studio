# SPDX-License-Identifier: GPL-3.0-or-later
find_package(nlohmann_json 3.11 REQUIRED)

add_library(hyprland-studio-ipc STATIC bindings.cpp events.cpp instance.cpp request.cpp socket.cpp)
target_include_directories(hyprland-studio-ipc PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-ipc PUBLIC hyprland-studio-foundation PRIVATE nlohmann_json::nlohmann_json)
