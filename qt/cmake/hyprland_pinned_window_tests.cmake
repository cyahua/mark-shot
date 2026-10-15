option(MARK_SHOT_HYPRLAND_INTEGRATION_TESTS "Run pinned image checks in an isolated Hyprland session" OFF)
if(NOT MARK_SHOT_HYPRLAND_INTEGRATION_TESTS)
    return()
endif()
if(NOT MARK_SHOT_LINUX OR NOT TARGET mark-shot-layer-shell)
    message(FATAL_ERROR "Hyprland integration checks require Linux and the layer-shell plugin")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HyprlandTestWayland REQUIRED IMPORTED_TARGET wayland-client)
find_program(MARK_SHOT_HYPRLAND_EXECUTABLE NAMES Hyprland REQUIRED)
find_program(MARK_SHOT_HYPRCTL_EXECUTABLE NAMES hyprctl REQUIRED)
find_program(MARK_SHOT_HYPRLAND_TEST_KWIN_EXECUTABLE NAMES kwin_wayland REQUIRED)
find_program(MARK_SHOT_WAYLAND_SCANNER_EXECUTABLE NAMES wayland-scanner REQUIRED)

set(pointer_protocol ${CMAKE_CURRENT_SOURCE_DIR}/tests/wayland/wlr-virtual-pointer-unstable-v1.xml)
set(pointer_generated ${CMAKE_CURRENT_BINARY_DIR}/generated/hyprland-test)
file(MAKE_DIRECTORY ${pointer_generated})
add_custom_command(
    OUTPUT ${pointer_generated}/virtual-pointer-client-protocol.h ${pointer_generated}/virtual-pointer-protocol.c
    COMMAND ${MARK_SHOT_WAYLAND_SCANNER_EXECUTABLE} client-header ${pointer_protocol}
        ${pointer_generated}/virtual-pointer-client-protocol.h
    COMMAND ${MARK_SHOT_WAYLAND_SCANNER_EXECUTABLE} private-code ${pointer_protocol}
        ${pointer_generated}/virtual-pointer-protocol.c
    DEPENDS ${pointer_protocol}
    VERBATIM
)
add_executable(mark-shot-hyprland-virtual-pointer
    tests/wayland/virtual_pointer.c
    ${pointer_generated}/virtual-pointer-client-protocol.h
    ${pointer_generated}/virtual-pointer-protocol.c
)
set_target_properties(mark-shot-hyprland-virtual-pointer PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_compile_features(mark-shot-hyprland-virtual-pointer PRIVATE c_std_11)
target_include_directories(mark-shot-hyprland-virtual-pointer PRIVATE ${pointer_generated})
target_link_libraries(mark-shot-hyprland-virtual-pointer PRIVATE PkgConfig::HyprlandTestWayland)

foreach(scale IN ITEMS 1 1.5 2)
    add_test(NAME hyprland-pinned-windows-${scale}
        COMMAND ${CMAKE_COMMAND} -E env PYTHONDONTWRITEBYTECODE=1
            ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-hyprland-pinned-windows.py
            --binary $<TARGET_FILE:mark-shot>
            --pointer $<TARGET_FILE:mark-shot-hyprland-virtual-pointer>
            --hyprland ${MARK_SHOT_HYPRLAND_EXECUTABLE}
            --hyprctl ${MARK_SHOT_HYPRCTL_EXECUTABLE}
            --kwin ${MARK_SHOT_HYPRLAND_TEST_KWIN_EXECUTABLE}
            --scale ${scale}
            --artifacts ${CMAKE_CURRENT_BINARY_DIR}/hyprland-test-results/${scale}
    )
    set_tests_properties(hyprland-pinned-windows-${scale} PROPERTIES TIMEOUT 65 LABELS "hyprland;integration")
endforeach()
