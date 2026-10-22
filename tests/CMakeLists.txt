# SPDX-License-Identifier: GPL-3.0-or-later
find_package(GTest REQUIRED)
find_package(nlohmann_json 3.11 REQUIRED)

add_executable(lantern-hyprland-tests accessibility-test.cpp devices-test.cpp displays-test.cpp essentials-test.cpp idle-test.cpp night-light-test.cpp input-test.cpp options-test.cpp activation-test.cpp autostart-test.cpp autostart-entries-test.cpp default-apps-test.cpp fastfetch-test.cpp screenshots-test.cpp trash-test.cpp user-themes-test.cpp window-look-test.cpp color-test.cpp cursor-test.cpp bindings-test.cpp command-test.cpp desktop-test.cpp lua-edit-test.cpp theme-test.cpp events-test.cpp file-change-test.cpp folder-test.cpp wallpaper-test.cpp launch-test.cpp main-config-test.cpp modifiers-test.cpp recording-test.cpp syntax-test.cpp)
target_link_libraries(lantern-hyprland-tests PRIVATE lantern-hyprland-activation lantern-hyprland-application lantern-hyprland-configuration lantern-hyprland-desktop lantern-hyprland-ipc nlohmann_json::nlohmann_json lantern-hyprland-syntax GTest::gtest_main)
target_compile_definitions(lantern-hyprland-tests PRIVATE FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

include(GoogleTest)
gtest_discover_tests(lantern-hyprland-tests)
