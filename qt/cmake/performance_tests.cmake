find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(MARK_SHOT_LINUX)
    add_test(NAME windows-packaging
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/windows_packaging_test.py")
    add_test(NAME runtime-idle-memory
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_SOURCE_DIR}/scripts/performance/measure_idle_memory.py"
            "$<TARGET_FILE:mark-shot>" --assert-no-media)
    set_tests_properties(windows-packaging runtime-idle-memory PROPERTIES TIMEOUT 30)
endif()
