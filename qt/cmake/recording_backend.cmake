include(cmake/recording_backend_sources.cmake)
target_sources(mark-shot PRIVATE ${MARK_SHOT_BACKEND_LOADER_SOURCES})

if(NOT WIN32)
    file(RELATIVE_PATH _recording_install_dir "${CMAKE_INSTALL_FULL_BINDIR}"
        "${CMAKE_INSTALL_FULL_LIBDIR}/mark-shot/recording")
    target_compile_definitions(mark-shot PRIVATE
        MARK_SHOT_RECORDING_INSTALL_DIR="${_recording_install_dir}")
endif()

if(FFmpegLibav_FOUND)
    add_library(mark-shot-recording-libav MODULE ${MARK_SHOT_LIBAV_BACKEND_SOURCES})
    target_include_directories(mark-shot-recording-libav PRIVATE src)
    target_compile_definitions(mark-shot-recording-libav PRIVATE HAVE_LIBAV_RECORDING)
    target_link_libraries(mark-shot-recording-libav PRIVATE Qt6::Core Qt6::Gui MarkShot::FFmpegLibav)
    set_target_properties(mark-shot-recording-libav PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        LIBRARY_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}/recording"
        RUNTIME_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}/recording")
    if(FFmpegAvfilter_FOUND)
        target_compile_definitions(mark-shot-recording-libav PRIVATE HAVE_LIBAVFILTER)
        target_link_libraries(mark-shot-recording-libav PRIVATE MarkShot::FFmpegAvfilter)
    endif()
    if(PulseAudioRecording_FOUND)
        target_compile_definitions(mark-shot-recording-libav PRIVATE HAVE_PULSE_RECORDING)
        target_link_libraries(mark-shot-recording-libav PRIVATE PkgConfig::PulseAudioRecording)
    endif()
    if(WIN32)
        set_target_properties(mark-shot-recording-libav PROPERTIES PREFIX "" CXX_STANDARD 20)
        target_link_libraries(mark-shot-recording-libav PRIVATE ksuser ole32 uuid)
    elseif(APPLE)
        set_target_properties(mark-shot-recording-libav PROPERTIES SUFFIX ".dylib")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # 【录制】【后端卸载】不生成会强制保留整个动态库的 GNU unique 符号
        target_compile_options(mark-shot-recording-libav PRIVATE -fno-gnu-unique)
    endif()
    add_dependencies(mark-shot mark-shot-recording-libav)
    if(WIN32)
        set(_backend_install_dir "${CMAKE_INSTALL_BINDIR}/recording")
    else()
        set(_backend_install_dir "${CMAKE_INSTALL_LIBDIR}/mark-shot/recording")
    endif()
    install(TARGETS mark-shot-recording-libav
        RUNTIME DESTINATION "${_backend_install_dir}"
        LIBRARY DESTINATION "${_backend_install_dir}")
    message(STATUS "mark-shot: FFmpeg recording backend enabled (loaded only when recording starts)")
else()
    message(STATUS "mark-shot: FFmpeg recording backend unavailable")
endif()
