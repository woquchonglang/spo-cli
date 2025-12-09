cmake_minimum_required(VERSION 3.24)

set(CMAKE_PROJECT_NAME spo-cli)

project(${CMAKE_PROJECT_NAME})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g3")
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_executable(${CMAKE_PROJECT_NAME})

file(GLOB_RECURSE SRCFILES "src/*.cpp")
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${SRCFILES})

# lib
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(
  CPPify
  GIT_REPOSITORY https://github.com/KielanT/CPPify.git
  GIT_TAG main
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/lib/CPPify")

FetchContent_Declare(
  ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v6.1.9
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/lib/ftxui")

FetchContent_Declare(
  tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.4.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/lib/tomlplusplus")

FetchContent_Declare(
  cpp-httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.28.0
  GIT_SHALLOW TRUE
  GIT_PROGRESS TRUE
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/lib/cpp-httplib")

FetchContent_MakeAvailable(CPPify)
FetchContent_MakeAvailable(ftxui)
FetchContent_MakeAvailable(tomlplusplus)
FetchContent_MakeAvailable(cpp-httplib)

target_link_libraries(
  ${CMAKE_PROJECT_NAME} PRIVATE ftxui::component ftxui::dom ftxui::screen
                                tomlplusplus::tomlplusplus)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    lib/json
    lib/cpp-httplib
)
