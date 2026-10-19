# SPDX-License-Identifier: GPL-3.0-or-later
pkg_check_modules(lua REQUIRED IMPORTED_TARGET lua5.5)

add_library(hyprland-studio-lua STATIC state.cpp value.cpp)
target_include_directories(hyprland-studio-lua PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-lua PRIVATE PkgConfig::lua)
