# 【GUI测试】【内存测量】复用实际应用对象，避免测试只覆盖脱离窗口的图像处理函数
qt_add_executable(mark-shot-gui-memory-benchmark
    tests/gui_memory_benchmark.cpp
    tests/gui_memory_scenarios.cpp
    "$<FILTER:$<TARGET_OBJECTS:mark-shot>,EXCLUDE,/main\\.cpp\\.(o|obj)$>")
target_include_directories(mark-shot-gui-memory-benchmark PRIVATE tests
    "$<TARGET_PROPERTY:mark-shot,INCLUDE_DIRECTORIES>")
target_compile_definitions(mark-shot-gui-memory-benchmark PRIVATE
    "$<TARGET_PROPERTY:mark-shot,COMPILE_DEFINITIONS>")
target_link_libraries(mark-shot-gui-memory-benchmark PRIVATE
    "$<TARGET_PROPERTY:mark-shot,LINK_LIBRARIES>" Qt6::Test)
get_target_property(gui_memory_standard mark-shot CXX_STANDARD)
set_target_properties(mark-shot-gui-memory-benchmark PROPERTIES
    CXX_STANDARD ${gui_memory_standard}
    RUNTIME_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}"
    WIN32_EXECUTABLE FALSE)
if(WIN32)
    target_link_libraries(mark-shot-gui-memory-benchmark PRIVATE psapi)
endif()
add_dependencies(mark-shot-gui-memory-benchmark mark-shot)
add_test(NAME gui-memory-smoke COMMAND mark-shot-gui-memory-benchmark
    --scenario cycle --width 800 --height 600 --strokes 3)
set_tests_properties(gui-memory-smoke PROPERTIES TIMEOUT 60
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_SCALE_FACTOR=1")

qt_add_executable(mark-shot-sharp-viewport-rendering-test
    tests/sharp_viewport_rendering_test.cpp
    tests/support/sharp_viewport_reference.cpp
    src/shot_window_viewport_rendering.cpp)
target_include_directories(mark-shot-sharp-viewport-rendering-test PRIVATE
    "$<TARGET_PROPERTY:mark-shot,INCLUDE_DIRECTORIES>")
target_link_libraries(mark-shot-sharp-viewport-rendering-test PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent Qt6::Test)
set_target_properties(mark-shot-sharp-viewport-rendering-test PROPERTIES CXX_STANDARD ${gui_memory_standard})
add_test(NAME sharp-viewport-rendering COMMAND mark-shot-sharp-viewport-rendering-test)

qt_add_executable(mark-shot-region-detection-scheduler-test
    tests/region_detection_scheduler_test.cpp
    src/region_detection/region_detection_scheduler.cpp
    ${MARK_SHOT_REGION_DETECTION_SOURCES})
target_include_directories(mark-shot-region-detection-scheduler-test PRIVATE src)
target_link_libraries(mark-shot-region-detection-scheduler-test PRIVATE
    Qt6::Core Qt6::Gui Qt6::Concurrent Qt6::Test)
add_test(NAME region-detection-scheduler COMMAND mark-shot-region-detection-scheduler-test)
