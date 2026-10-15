qt_add_executable(mark-shot-rust-image-contract-test
    tests/rust_image_contract_test.cpp
    src/annotation_filters/image_filter_effects.cpp)
target_include_directories(mark-shot-rust-image-contract-test PRIVATE src)
target_link_libraries(mark-shot-rust-image-contract-test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
target_compile_definitions(mark-shot-rust-image-contract-test PRIVATE
    MARK_SHOT_FILTER_FIXTURE_PATH="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/image_filter_golden.json")
add_test(NAME rust-image-contract COMMAND mark-shot-rust-image-contract-test)

qt_add_executable(mark-shot-runtime-contract-test
    tests/runtime_contract_test.cpp
    "$<FILTER:$<TARGET_OBJECTS:mark-shot>,EXCLUDE,/main\\.cpp\\.(o|obj)$>")
target_include_directories(mark-shot-runtime-contract-test PRIVATE "$<TARGET_PROPERTY:mark-shot,INCLUDE_DIRECTORIES>")
target_compile_definitions(mark-shot-runtime-contract-test PRIVATE "$<TARGET_PROPERTY:mark-shot,COMPILE_DEFINITIONS>"
    MARK_SHOT_OPENAI_PLUGIN="$<TARGET_FILE:mark-shot-translate-openai>")
target_link_libraries(mark-shot-runtime-contract-test PRIVATE "$<TARGET_PROPERTY:mark-shot,LINK_LIBRARIES>" Qt6::Test)
get_target_property(_app_standard mark-shot CXX_STANDARD)
set_target_properties(mark-shot-runtime-contract-test PROPERTIES
    CXX_STANDARD ${_app_standard} RUNTIME_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}")
if(WIN32)
    # 【测试】【Windows 入口】同步主程序类型，为继承的 QT_NEEDS_QMAIN 定义链接 Qt 入口库
    set_target_properties(mark-shot-runtime-contract-test PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
add_dependencies(mark-shot-runtime-contract-test mark-shot mark-shot-translate-openai)
add_test(NAME runtime-contract COMMAND mark-shot-runtime-contract-test)
set_tests_properties(runtime-contract PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

add_test(NAME source-module-limits COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_SOURCE_DIR}/scripts/check_source_limits.py")

include(cmake/rust_editor_tests.cmake)

add_test(NAME runtime-startup COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/runtime_startup_test.py" "$<TARGET_FILE:mark-shot>")
set_tests_properties(runtime-startup PROPERTIES TIMEOUT 50)
