cmake_minimum_required(VERSION 4.4)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD f35a9ac6-8463-4d38-8eec-5d6008153e7d)
set(CMAKE_PROJECT_NAME spo-cli)

set(CMAKE_C_COMPILER    /usr/bin/clang)
set(CMAKE_CXX_COMPILER  /usr/bin/clang++)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g0")
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()
# set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(${CMAKE_PROJECT_NAME})

option(SPOCLI_TEST "Set to ON to build tests" OFF)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)

# lib

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/.deps)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/lib)

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

set(FTXUI_BUILD_MODULES ON)
set(FTXUI_ENABLE_CCACHE ON)
set(FTXUI_ENABLE_INSTALL OFF)
set(TOMLPLUSPLUS_BUILD_MODULES ON)
set(HTTPLIB_BUILD_MODULES ON)
set(HTTPLIB_COMPILE ON)
set(NLOHMANN_JSON_BUILD_MODULES ON CACHE BOOL "Enable nlohmann.json module support")
set(BASE64_ENABLE_TESTING OFF)
set(SPDLOG_USE_STD_FORMAT ON)

add_subdirectory(lib/nlohmann-json)
add_subdirectory(lib/tomlplusplus)
add_subdirectory(lib/ftxui)
add_subdirectory(lib/cpp-httplib)
add_subdirectory(lib/concurrentqueue)
add_subdirectory(lib/readerwriterqueue)
add_subdirectory(lib/base64)
add_subdirectory(lib/librespot-cpp)
add_subdirectory(lib/spdlog)

add_compile_definitions(BOOST_ASIO_HAS_IO_URING)
# add_compile_definitions(BOOST_ASIO_HAS_IO_URING_AS_DEFAULT)
add_compile_definitions(BOOST_ASIO_SEPARATE_COMPILATION)

add_compile_options(
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:-ffunction-sections>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CONFIG:Release>>:-fdata-sections>
)

add_subdirectory(src)

# link
target_link_libraries(
  ${CMAKE_PROJECT_NAME} PRIVATE spo_src)

target_link_options(spo-cli PRIVATE
    $<$<CONFIG:Release>:-Wl,--gc-sections>
    $<$<CONFIG:Release>:-Wl,--as-needed>
)

# linker
find_program(MOLD_PROGRAM mold)
if (MOLD_PROGRAM)
  target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -fuse-ld=mold)
else ()
    message(STATUS "mold not found")
endif ()

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_MODULE_STD ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
   set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif()
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-flto=auto>
)

if(SPOCLI_TEST)
  include(test/test.cmake)
endif()

# install
install(TARGETS spo-cli
        RUNTIME DESTINATION bin
        COMPONENT spo-cli)
