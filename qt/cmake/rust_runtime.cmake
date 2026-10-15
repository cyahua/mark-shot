# 【构建】【运行目录】保持可执行文件、图层插件和 provider 插件的相对位置一致
set(MARK_SHOT_RUNTIME_DIR "${CMAKE_BINARY_DIR}")
if(CMAKE_CONFIGURATION_TYPES)
    set(MARK_SHOT_RUNTIME_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
set_target_properties(mark-shot PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}")
if(TARGET mark-shot-layer-shell)
    set_target_properties(mark-shot-layer-shell PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}")
endif()
foreach(provider IN ITEMS ocr-rapid translate-openai translate-tencent translate-baidu
        translate-youdao translate-gemini translate-anthropic code-scan-zxing)
    if(TARGET mark-shot-${provider})
        set_target_properties(mark-shot-${provider} PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}/plugins"
            RUNTIME_OUTPUT_DIRECTORY "${MARK_SHOT_RUNTIME_DIR}/plugins")
    endif()
endforeach()
