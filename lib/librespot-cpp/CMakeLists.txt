cmake_minimum_required(VERSION 3.21)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(LIB_IS_TOP ON)
else()
  set(LIB_IS_TOP OFF)
endif()

if(LIB_IS_TOP)
  project(librespot-cpp CXX)
endif()

set(RUST_SOURCE_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/librespot.rs)
set(CARGO_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)

set(CARGO_TARGET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/target)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "librespot build debug mode")
	set(CARGO_CMD cargo build --verbose)
	set(TARGET_DIR "debug")
  set(RUST_PART_LIB "${CARGO_TARGET_DIR}/debug/liblibrespot_cpp.a")
else ()
  message(STATUS "librespot build in release mode")
	set(CARGO_CMD cargo build --release --verbose)
	set(TARGET_DIR "release")
  set(RUST_PART_LIB "${CARGO_TARGET_DIR}/release/liblibrespot_cpp.a")
endif ()

if (ENABLE_LTO)
	set(RUST_FLAGS "-Clinker-plugin-lto" "-Clinker=clang-11" "-Clink-arg=-fuse-ld=lld-11")
endif ()

set(RUST_PART "${CARGO_TARGET_DIR}/cxxbridge/librespot-cpp/src")
set(RUST_PART_CXX "${CARGO_TARGET_DIR}/cxxbridge/librespot-cpp/src/librespot.rs.cc")

add_custom_command(
  OUTPUT ${RUST_PART_CXX} ${RUST_PART_LIB}
  COMMAND ${CARGO_CMD} --manifest-path ${CARGO_MANIFEST}
  DEPENDS ${RUST_SOURCE_FILE}
  USES_TERMINAL
  COMMENT "Running cargo..."
)

add_library(librespot-cpp STATIC ${RUST_PART_CXX})

target_link_libraries(librespot-cpp pthread dl ssl crypto asound ${RUST_PART_LIB})

target_include_directories(
  librespot-cpp PUBLIC ${RUST_PART}
)


if(LIB_IS_TOP)
  add_executable(librespot_example example/test.cpp)
  target_link_libraries(librespot_example PRIVATE librespot-cpp)
endif()
