# SPDX-License-Identifier: GPL-3.0-or-later
add_library(hyprland-studio-configuration STATIC bindings.cpp location.cpp recording.cpp value.cpp)
target_include_directories(hyprland-studio-configuration PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-configuration PUBLIC hyprland-studio-foundation PRIVATE hyprland-studio-lua)
# recording.cpp embeds recorder.lua; rebuild it when the script changes.
set_property(SOURCE recording.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/recorder.lua)
