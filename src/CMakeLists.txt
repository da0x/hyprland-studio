# SPDX-License-Identifier: GPL-3.0-or-later
add_subdirectory(foundation)
add_subdirectory(lua)
add_subdirectory(syntax)
add_subdirectory(configuration)
add_subdirectory(ipc)
add_subdirectory(application)
add_subdirectory(ui)

add_executable(hyprland-studio main.cpp)
target_link_libraries(hyprland-studio PRIVATE hyprland-studio-ui)
install(TARGETS hyprland-studio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
