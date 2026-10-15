# 【测试】【用户数据保护】所有 Qt 测试使用独立配置、数据和运行目录，不能回退真实 HOME
get_property(_qt_tests DIRECTORY PROPERTY TESTS)
foreach(_test IN LISTS _qt_tests)
    string(MAKE_C_IDENTIFIER "${_test}" _safe_name)
    set(_home "${CMAKE_BINARY_DIR}/test-homes/${_safe_name}")
    foreach(_directory IN ITEMS config data cache runtime local roaming)
        file(MAKE_DIRECTORY "${_home}/${_directory}")
    endforeach()
    file(CHMOD "${_home}/runtime" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)
    set_property(TEST ${_test} APPEND PROPERTY ENVIRONMENT
        "HOME=${_home}" "USERPROFILE=${_home}"
        "APPDATA=${_home}/roaming" "LOCALAPPDATA=${_home}/local"
        "MARK_SHOT_CONFIG_DIR=${_home}/config/mark-shot-rs"
        "XDG_CONFIG_HOME=${_home}/config" "XDG_DATA_HOME=${_home}/data"
        "XDG_CACHE_HOME=${_home}/cache" "XDG_RUNTIME_DIR=${_home}/runtime")
endforeach()
