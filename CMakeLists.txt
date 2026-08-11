cmake_minimum_required(VERSION 3.20)
project(engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Vulkan REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)

find_library(SHADERC_SHARED_LIB shaderc_shared REQUIRED)

file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/source/*.cpp
    ${CMAKE_SOURCE_DIR}/src/pipeline/*.cpp
)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
file(GLOB IMGUI_SOURCES CONFIGURE_DEPENDS
    ${IMGUI_DIR}/*.cpp
)
list(APPEND IMGUI_SOURCES
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
)

add_executable(engine ${ENGINE_SOURCES} ${IMGUI_SOURCES})

target_include_directories(engine PRIVATE
    ${CMAKE_SOURCE_DIR}/thirdparty
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends
    ${CMAKE_SOURCE_DIR}/src/include
)

target_compile_definitions(engine PRIVATE
    VULKAN_HPP_NO_CONSTRUCTORS
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
)

target_link_libraries(engine PRIVATE
    Vulkan::Vulkan
    PkgConfig::GLFW
    ${SHADERC_SHARED_LIB}
    dl
    pthread
)

target_compile_options(engine PRIVATE $<$<CONFIG:Debug>:-ggdb>)

set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

add_custom_target(format
    COMMAND ${CMAKE_SOURCE_DIR}/src/clang_format.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running clang-format"
)

add_custom_target(run
    COMMAND engine
    DEPENDS engine
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
)
