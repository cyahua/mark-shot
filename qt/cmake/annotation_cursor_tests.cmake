qt_add_executable(mark-shot-capture-cross-cursor-test
    tests/capture_cross_cursor_test.cpp
    src/capture_cross_cursor.cpp
    src/selection_loupe.cpp
)
target_include_directories(mark-shot-capture-cross-cursor-test PRIVATE src)
target_link_libraries(mark-shot-capture-cross-cursor-test PRIVATE Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME capture-cross-cursor COMMAND mark-shot-capture-cross-cursor-test)
set_tests_properties(capture-cross-cursor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# 1. 【标注测试】【生产窗口】复用主程序对象，只替换程序入口，直接验证实际窗口的事件和绘制
qt_add_executable(mark-shot-annotation-cursor-test
    tests/annotation_cursor_test.cpp
    "$<FILTER:$<TARGET_OBJECTS:mark-shot>,EXCLUDE,/main\\.cpp\\.(o|obj)$>"
)
target_include_directories(mark-shot-annotation-cursor-test PRIVATE "$<TARGET_PROPERTY:mark-shot,INCLUDE_DIRECTORIES>")
target_compile_definitions(mark-shot-annotation-cursor-test PRIVATE "$<TARGET_PROPERTY:mark-shot,COMPILE_DEFINITIONS>")
target_link_libraries(mark-shot-annotation-cursor-test PRIVATE "$<TARGET_PROPERTY:mark-shot,LINK_LIBRARIES>" Qt6::Test)
get_target_property(annotation_cursor_cxx_standard mark-shot CXX_STANDARD)
set_target_properties(mark-shot-annotation-cursor-test PROPERTIES CXX_STANDARD ${annotation_cursor_cxx_standard})
if(WIN32)
    # 2. 【标注测试】【Windows 入口】同步主程序类型，为继承的 QT_NEEDS_QMAIN 定义链接 Qt 入口库
    set_target_properties(mark-shot-annotation-cursor-test PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
add_dependencies(mark-shot-annotation-cursor-test mark-shot)
add_test(NAME annotation-cursor COMMAND mark-shot-annotation-cursor-test)
set_tests_properties(annotation-cursor PROPERTIES TIMEOUT 30
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_STYLE_OVERRIDE=Fusion;QT_SCALE_FACTOR=1")
