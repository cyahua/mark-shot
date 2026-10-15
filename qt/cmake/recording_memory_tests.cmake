qt_add_executable(mark-shot-recording-memory-policy-test
    tests/recording_memory_policy_test.cpp
    src/recording/recording_memory_policy.cpp)
target_include_directories(mark-shot-recording-memory-policy-test PRIVATE src)
target_link_libraries(mark-shot-recording-memory-policy-test PRIVATE Qt6::Core Qt6::Test)
add_test(NAME recording-memory-policy COMMAND mark-shot-recording-memory-policy-test)

if(TARGET mark-shot-recording-libav)
    qt_add_executable(mark-shot-recording-encoder-memory-test
        tests/recording_encoder_memory_test.cpp
        src/recording/libav/libav_video_encoder_setup.cpp
        src/recording/libav/libav_hw_encoder_context.cpp
        src/recording/libav_error.cpp
        src/recording/recording_encoder_probe.cpp
        src/recording/recording_quality_options.cpp
        src/recording/recording_memory_policy.cpp
        src/debug_log.cpp)
    target_include_directories(mark-shot-recording-encoder-memory-test PRIVATE src)
    target_compile_definitions(mark-shot-recording-encoder-memory-test PRIVATE HAVE_LIBAV_RECORDING)
    target_link_libraries(mark-shot-recording-encoder-memory-test PRIVATE Qt6::Core Qt6::Gui Qt6::Test MarkShot::FFmpegLibav)
    add_test(NAME recording-encoder-memory COMMAND mark-shot-recording-encoder-memory-test)

    qt_add_executable(mark-shot-recording-memory-benchmark
        tests/recording_memory_benchmark.cpp
        ${MARK_SHOT_BACKEND_LOADER_SOURCES}
        src/recording/recording_async_writer.cpp
        src/recording/recording_async_writer.h
        src/recording/recording_bgra_buffer_pool.cpp
        src/recording/recording_frame_payload.cpp
        src/recording/recording_frame_queue.cpp
        src/recording/recording_quality_options.cpp
        src/recording/recording_memory_policy.cpp
        src/debug_log.cpp)
    target_include_directories(mark-shot-recording-memory-benchmark PRIVATE src tests)
    target_link_libraries(mark-shot-recording-memory-benchmark PRIVATE Qt6::Core Qt6::Gui)
    if(WIN32)
        target_link_libraries(mark-shot-recording-memory-benchmark PRIVATE psapi)
    endif()
    set_target_properties(mark-shot-recording-memory-benchmark PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}")
    add_dependencies(mark-shot-recording-memory-benchmark mark-shot-recording-libav)
endif()
