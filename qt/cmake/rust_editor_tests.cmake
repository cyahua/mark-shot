qt_add_executable(mark-shot-rust-editor-contract-test
    tests/rust_editor_contract_test.cpp
    bridge/editor_history.cpp
    bridge/annotation_snapshot_codec.cpp
    src/ui/theme.cpp)
target_include_directories(mark-shot-rust-editor-contract-test PRIVATE src)
target_link_libraries(mark-shot-rust-editor-contract-test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME rust-editor-contract COMMAND mark-shot-rust-editor-contract-test)
set_tests_properties(rust-editor-contract PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)

qt_add_executable(mark-shot-rust-state-contract-test
    tests/rust_state_contract_test.cpp
    "$<FILTER:$<TARGET_OBJECTS:mark-shot>,EXCLUDE,/main\\.cpp\\.(o|obj)$>")
target_include_directories(mark-shot-rust-state-contract-test PRIVATE "$<TARGET_PROPERTY:mark-shot,INCLUDE_DIRECTORIES>")
target_compile_definitions(mark-shot-rust-state-contract-test PRIVATE "$<TARGET_PROPERTY:mark-shot,COMPILE_DEFINITIONS>"
    MARK_SHOT_DEFAULTS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/app_config_golden.json"
    MARK_SHOT_STATE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/annotation_state_golden.json")
target_link_libraries(mark-shot-rust-state-contract-test PRIVATE "$<TARGET_PROPERTY:mark-shot,LINK_LIBRARIES>" Qt6::Test)
get_target_property(_state_app_standard mark-shot CXX_STANDARD)
set_target_properties(mark-shot-rust-state-contract-test PROPERTIES CXX_STANDARD ${_state_app_standard})
if(WIN32)
    set_target_properties(mark-shot-rust-state-contract-test PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
add_dependencies(mark-shot-rust-state-contract-test mark-shot)
add_test(NAME rust-state-contract COMMAND mark-shot-rust-state-contract-test)
set_tests_properties(rust-state-contract PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 30)
