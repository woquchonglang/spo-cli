cmake_minimum_required(VERSION 4.4)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD f35a9ac6-8463-4d38-8eec-5d6008153e7d)
set(CMAKE_PROJECT_NAME spo-cli)

project(${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g0")
set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_BUILD_TYPE "Release")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# linker
find_program(MOLD_PROGRAM mold)
if (MOLD_PROGRAM)
    add_link_options(-fuse-ld=mold)
else ()
    message(STATUS "mold not found")
endif ()

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
set(TOMLPLUSPLUS_BUILD_MODULES ON)
set(HTTPLIB_BUILD_MODULES ON)
set(HTTPLIB_COMPILE ON)
set(NLOHMANN_JSON_BUILD_MODULES ON CACHE BOOL "Enable nlohmann.json module support")
set(MINIAUDIO_ENABLE_ALSA ON)
set(BASE64_ENABLE_TESTING OFF)
set(skyr_BUILD_TESTS OFF)
set(skyr_ENABLE_JSON_FUNCTIONS OFF)

add_subdirectory(lib/nlohmann-json)
add_subdirectory(lib/tomlplusplus)
add_subdirectory(lib/sdbus-cpp)
add_subdirectory(lib/ftxui)
add_subdirectory(lib/cpp-httplib)
add_subdirectory(lib/miniaudio)
add_subdirectory(lib/concurrentqueue)
add_subdirectory(lib/readerwriterqueue)
add_subdirectory(lib/url)
add_subdirectory(lib/base64)

add_library(caches INTERFACE)
target_include_directories(caches INTERFACE lib/caches/include)

add_library(stb INTERFACE)
add_compile_definitions(STB_IMAGE_IMPLEMENTATION)

target_include_directories(caches INTERFACE lib/stb)

add_compile_definitions(BOOST_ASIO_HAS_IO_URING)
add_compile_definitions(BOOST_ASIO_DISABLE_EPOLL)
add_compile_definitions(BOOST_ASIO_HAS_IO_URING_AS_DEFAULT)
add_compile_definitions(BOOST_ASIO_SEPARATE_COMPILATION)


# concurrentqueue has so many static func and static value which can expose global module TU local
target_compile_options(concurrentqueue INTERFACE -Wno-expose-global-module-tu-local)

target_compile_options(skyr-url INTERFACE -Wno-deprecated-literal-operator -Wno-unused-result)

add_subdirectory(src)

# link
target_link_libraries(
  ${CMAKE_PROJECT_NAME} PRIVATE spo_src)

# add_compile_options(-Wexpose-global-module-tu-local)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_MODULE_STD ON)


target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:-flto=auto>
)


set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_MODULE_STD ON)
