# SPDX-License-Identifier: GPL-3.0-or-later
pkg_check_modules(tree-sitter REQUIRED IMPORTED_TARGET tree-sitter tree-sitter-lua)

add_library(hyprland-studio-syntax STATIC highlight.cpp)
target_include_directories(hyprland-studio-syntax PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hyprland-studio-syntax PRIVATE PkgConfig::tree-sitter)
# highlight.cpp embeds lua-highlights.scm; rebuild it when the query changes.
set_property(SOURCE highlight.cpp APPEND PROPERTY OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/lua-highlights.scm)
