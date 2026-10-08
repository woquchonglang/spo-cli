add_library(spo_src)

set_target_properties(spo_src PROPERTIES
    CXX_MODULE_STD ON
)

find_package(sdbus-c++ REQUIRED)

file(GLOB_RECURSE SRCFILES "*.cpp")
file(GLOB_RECURSE MODULE_FILES "*.cppm")

list(REMOVE_ITEM SRCFILES "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

target_sources(spo_src
  PUBLIC FILE_SET CXX_MODULES FILES ${MODULE_FILES}
  PRIVATE ${SRCFILES}
)

target_include_directories(spo_src PRIVATE ${CMAKE_SOURCE_DIR}/lib/asio/include ${CMAKE_SOURCE_DIR}/lib/beast/include)

add_library(caches INTERFACE)
target_include_directories(caches INTERFACE ${CMAKE_SOURCE_DIR}/lib/caches/include)

add_library(stb INTERFACE)
add_compile_definitions(STB_IMAGE_IMPLEMENTATION)

add_library(cava STATIC ${CMAKE_SOURCE_DIR}/lib/cava/cavacore.c)
target_include_directories(cava PUBLIC ${CMAKE_SOURCE_DIR}/lib/cava)

target_compile_options(skyr-url INTERFACE -Wno-deprecated-literal-operator -Wno-unused-result -Wno-unused-value)

target_include_directories(caches INTERFACE ${CMAKE_SOURCE_DIR}/lib/stb)

target_compile_definitions(tomlplusplus_modules PRIVATE TOML_EXCEPTIONS=0)
target_compile_options(tomlplusplus_modules PRIVATE -fno-exceptions)

target_link_libraries(
  spo_src PUBLIC ftxui::modules SDBusCpp::sdbus-c++ nlohmann_json_modules httplib
                 tomlplusplus_modules curl ssl crypto concurrentqueue caches
                 skyr-url base64 uring librespot-cpp readerwriterqueue
                 fmt spdlog fftw3 cava)

# version
execute_process(
  COMMAND git describe --tags --abbrev=0
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT GIT_VERSION)
  set(GIT_VERSION "0.0.0")
else()
  string(REGEX REPLACE "^v" "" GIT_VERSION "${GIT_VERSION}")
endif()
target_compile_definitions(spo_src PRIVATE
  GIT_VERSION="${GIT_VERSION}")

