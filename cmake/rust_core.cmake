include_guard(GLOBAL)
find_program(CARGO_EXECUTABLE NAMES cargo REQUIRED)
find_package(Threads REQUIRED)
set(MARK_SHOT_RUST_TARGET "" CACHE STRING "Optional explicit Rust target triple")
if(CMAKE_CROSSCOMPILING AND NOT MARK_SHOT_RUST_TARGET)
    message(FATAL_ERROR "Cross builds require MARK_SHOT_RUST_TARGET matching the C++ toolchain")
endif()
set(_cargo_target_dir "${CMAKE_BINARY_DIR}/cargo")
set(_cargo_profile "$<IF:$<CONFIG:Debug>,dev,release>")
set(_archive_profile "$<IF:$<CONFIG:Debug>,debug,release>")
set(_target_args)
set(_target_subdir "")
if(MARK_SHOT_RUST_TARGET)
    list(APPEND _target_args --target "${MARK_SHOT_RUST_TARGET}")
    set(_target_subdir "${MARK_SHOT_RUST_TARGET}/")
endif()
if(MSVC)
    set(_archive_name "shot_ffi.lib")
else()
    set(_archive_name "libshot_ffi.a")
endif()
set(_archive "${_cargo_target_dir}/${_target_subdir}${_archive_profile}/${_archive_name}")
file(GLOB_RECURSE _rust_inputs CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/crates/*.rs"
    "${CMAKE_CURRENT_SOURCE_DIR}/crates/*.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/crates/*/Cargo.toml")
add_custom_command(OUTPUT "${_archive}"
    COMMAND "${CMAKE_COMMAND}" -E env "CARGO_TARGET_DIR=${_cargo_target_dir}"
        "${CARGO_EXECUTABLE}" build --locked --package shot-ffi
        --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml"
        --profile "${_cargo_profile}" ${_target_args}
    DEPENDS ${_rust_inputs} "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml" "${CMAKE_CURRENT_SOURCE_DIR}/Cargo.lock"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Building Rust screenshot core" VERBATIM)
add_custom_target(shot-rust-build ALL DEPENDS "${_archive}")
add_library(shot_rust_core STATIC IMPORTED GLOBAL)
set_target_properties(shot_rust_core PROPERTIES
    IMPORTED_LOCATION "${_cargo_target_dir}/${_target_subdir}release/${_archive_name}"
    IMPORTED_LOCATION_DEBUG "${_cargo_target_dir}/${_target_subdir}debug/${_archive_name}"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/qt/bridge"
    INTERFACE_LINK_LIBRARIES "Threads::Threads;${CMAKE_DL_LIBS}")
add_dependencies(shot_rust_core shot-rust-build)
if(WIN32)
    set_property(TARGET shot_rust_core APPEND PROPERTY INTERFACE_LINK_LIBRARIES ws2_32 userenv bcrypt ntdll advapi32)
elseif(UNIX AND NOT APPLE)
    set_property(TARGET shot_rust_core APPEND PROPERTY INTERFACE_LINK_LIBRARIES m)
endif()
