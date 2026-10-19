# SPDX-License-Identifier: GPL-3.0-or-later
find_package(GTest REQUIRED)

add_executable(hyprland-studio-tests bindings-test.cpp events-test.cpp modifiers-test.cpp)
target_link_libraries(hyprland-studio-tests PRIVATE hyprland-studio-ipc GTest::gtest_main)
target_compile_definitions(hyprland-studio-tests PRIVATE FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

include(GoogleTest)
gtest_discover_tests(hyprland-studio-tests)
