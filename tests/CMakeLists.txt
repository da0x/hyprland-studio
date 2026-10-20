# SPDX-License-Identifier: GPL-3.0-or-later
find_package(GTest REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)

add_executable(hyprland-studio-tests autostart-test.cpp bindings-test.cpp command-test.cpp desktop-test.cpp lua-edit-test.cpp theme-test.cpp events-test.cpp file-change-test.cpp wallpaper-test.cpp launch-test.cpp main-config-test.cpp modifiers-test.cpp recording-test.cpp syntax-test.cpp)
target_link_libraries(hyprland-studio-tests PRIVATE hyprland-studio-application hyprland-studio-configuration hyprland-studio-desktop hyprland-studio-ipc nlohmann_json::nlohmann_json hyprland-studio-syntax GTest::gtest_main)
target_compile_definitions(hyprland-studio-tests PRIVATE FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

include(GoogleTest)
gtest_discover_tests(hyprland-studio-tests)
