option(MARK_SHOT_KDE_INTEGRATION_TESTS "Run pinned window checks in an isolated KWin session" OFF)
if(NOT MARK_SHOT_KDE_INTEGRATION_TESTS)
    return()
endif()
if(NOT UNIX OR APPLE OR NOT TARGET Qt6::DBus)
    message(FATAL_ERROR "KDE integration checks require Linux and Qt DBus")
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_program(MARK_SHOT_KWIN_EXECUTABLE NAMES kwin_wayland REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(KdeTestEi REQUIRED IMPORTED_TARGET libei-1.0)

qt_add_executable(mark-shot-kde-pinned-windows-test
    tests/kde/pinned_windows_test.cpp
    tests/kde/kwin_script_probe.cpp
    tests/kde/kwin_script_probe.h
    tests/kde/kwin_eis_input.cpp
    tests/kde/kwin_eis_input.h
    tests/kde/pinned_resize_checks.cpp
    tests/kde/pinned_resize_checks.h
    src/pinned_window_top.cpp
    src/pinned_window/pinned_kde_keep_above.cpp
    src/pinned_window/pinned_kde_keep_above_script.cpp
    src/pinned_window/pinned_kde_keep_above_bridge.cpp
    src/pinned_window/pinned_native_resize.cpp
    src/pinned_window/pinned_resize_controller.cpp
    src/pinned_window/pinned_layer_shell_geometry.cpp
    src/pinned_window/pinned_layer_shell_screen_binding.cpp
    src/layer_shell_runtime.cpp
    src/windows_integration.cpp
    src/debug_log.cpp
)
target_include_directories(mark-shot-kde-pinned-windows-test PRIVATE src tests/kde)
target_compile_definitions(mark-shot-kde-pinned-windows-test PRIVATE MARK_SHOT_WITH_DBUS=1)
target_link_libraries(mark-shot-kde-pinned-windows-test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus Qt6::Test PkgConfig::KdeTestEi)

foreach(variant IN ITEMS wayland xwayland scaled-zh)
    set(kde_arguments --platform wayland)
    if(variant STREQUAL "xwayland")
        set(kde_arguments --platform xcb)
    elseif(variant STREQUAL "scaled-zh")
        list(APPEND kde_arguments --language zh --scale 1.25 --outputs 2)
    else()
        list(APPEND kde_arguments --idle-check)
    endif()
    add_test(NAME kde-pinned-windows-${variant}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test-kde-pinned-windows.py
            --binary $<TARGET_FILE:mark-shot-kde-pinned-windows-test>
            --kwin ${MARK_SHOT_KWIN_EXECUTABLE}
            --artifacts ${CMAKE_CURRENT_BINARY_DIR}/kde-test-results/${variant}
            ${kde_arguments}
    )
    set_tests_properties(kde-pinned-windows-${variant} PROPERTIES TIMEOUT 70 LABELS "kde;integration")
endforeach()
