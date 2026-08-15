cmake_minimum_required(VERSION 3.20)
project(engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ----------------------------------------------------------------------------
# Platform dependencies
#
# Each branch fills ENGINE_PLATFORM_LIBS / _DEFINES / _OPTIONS, which are
# applied to the target further down.
# ----------------------------------------------------------------------------
if(WIN32)
    # Everything comes from the vcpkg manifest: glfw3, glm, vulkan-headers
    # and vulkan-loader. find_package(Vulkan) picks the latter two up off
    # vcpkg's CMAKE_PREFIX_PATH, so no LunarG SDK install is needed.
    find_package(Vulkan REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(glm CONFIG REQUIRED)

    set(ENGINE_PLATFORM_LIBS
        Vulkan::Vulkan
        glfw
        glm::glm
    )

    set(ENGINE_PLATFORM_DEFINES
        # windows.h leaks min/max macros that break std::min/std::max and glm.
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        _CRT_SECURE_NO_WARNINGS
        # GCC/Clang auto-define __SSE2__ on x64, which is how glm enables the
        # SIMD specializations that GLM_FORCE_DEFAULT_ALIGNED_GENTYPES requires.
        # MSVC never defines that macro even though SSE2 is baseline on x64, so
        # glm must be told explicitly or aligned vec types fail to compile.
        GLM_FORCE_INTRINSICS
    )

    # /bigobj: main.cpp pulls in the stb_image, tiny_obj_loader and VMA
    #          implementations, which overflows the default section limit.
    # /wd4068: MSVC does not know the `#pragma GCC system_header` in mesh.h.
    set(ENGINE_PLATFORM_OPTIONS /bigobj /utf-8 /wd4068)
else()
    # Vulkan and glm come from the system; glfw3 via pkg-config. Only shaderc
    # is pulled from vcpkg.
    find_package(Vulkan REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED IMPORTED_TARGET glfw3)

    set(ENGINE_PLATFORM_LIBS
        Vulkan::Vulkan
        PkgConfig::GLFW
        dl
        pthread
    )

    set(ENGINE_PLATFORM_DEFINES "")
    set(ENGINE_PLATFORM_OPTIONS $<$<CONFIG:Debug>:-ggdb>)
endif()

find_package(unofficial-shaderc CONFIG REQUIRED)

# ----------------------------------------------------------------------------
# Target
# ----------------------------------------------------------------------------
file(GLOB ENGINE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/source/*.cpp
    ${CMAKE_SOURCE_DIR}/src/pipeline/*.cpp
    ${CMAKE_SOURCE_DIR}/src/renderpass/*.cpp
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
    ${ENGINE_PLATFORM_DEFINES}
)

target_link_libraries(engine PRIVATE
    unofficial::shaderc::shaderc
    ${ENGINE_PLATFORM_LIBS}
)

target_compile_options(engine PRIVATE ${ENGINE_PLATFORM_OPTIONS})

set_target_properties(engine PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build
)

# ----------------------------------------------------------------------------
# Convenience targets
# ----------------------------------------------------------------------------
find_program(BASH_EXECUTABLE bash)
if(BASH_EXECUTABLE)
    add_custom_target(format
        COMMAND ${BASH_EXECUTABLE} ${CMAKE_SOURCE_DIR}/src/clang_format.sh
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-format"
    )
endif()

add_custom_target(run
    COMMAND engine
    DEPENDS engine
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    USES_TERMINAL
)
