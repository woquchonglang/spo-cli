cmake_minimum_required(VERSION 4.3)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD 451f2fe2-a8a2-47c3-bc32-94786d8fc91b)

set(CMAKE_PROJECT_NAME spo-cli)

project(${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g3")
set(CMAKE_BUILD_TYPE "Debug")
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

add_executable(${CMAKE_PROJECT_NAME})

# lib

find_program(CCACHE_PROGRAM ccache)

if(CCACHE_PROGRAM)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif()

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(FTXUI_BUILD_MODULES ON)
set(TOMLPLUSPLUS_BUILD_MODULES ON)
set(HTTPLIB_BUILD_MODULES ON)
set(HTTPLIB_COMPILE ON)
set(NLOHMANN_JSON_BUILD_MODULES ON CACHE BOOL "Enable nlohmann.json module support")

add_subdirectory(lib/nlohmann-json)
add_subdirectory(lib/tomlplusplus)
add_subdirectory(lib/sdbus-cpp)
add_subdirectory(lib/ftxui)
add_subdirectory(lib/cpp-httplib)

target_link_libraries(
  ${CMAKE_PROJECT_NAME} PRIVATE ftxui::modules SDBusCpp::sdbus-c++ nlohmann_json_modules httplib
                                 tomlplusplus_modules curl ssl crypto)

file(GLOB_RECURSE SRCFILES "src/*.cpp")
file(GLOB_RECURSE MODULE_FILES "src/*.cppm")

target_sources(${CMAKE_PROJECT_NAME}
  PUBLIC FILE_SET CXX_MODULES FILES ${MODULE_FILES}
  PUBLIC ${SRCFILES}
)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES CXX_MODULE_STD ON)
