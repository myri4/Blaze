cmake_minimum_required(VERSION 3.8)

set(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}")
set(VENDOR "vendor/")

project("WC-Engine")
add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)
string(REPLACE "-Wformat" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/WC Engine/*.cpp" "${CMAKE_SOURCE_DIR}/WC Engine/*.h")
file(GLOB IMGUI_SOURCES
"vendor/imgui/*.cpp" "vendor/imgui/*.h"
"vendor/imgui/misc/cpp/*.cpp" "vendor/imgui/misc/cpp/*.h"
)

# Check if we're inside CLion
if (NOT DEFINED ENV{CLION_IDE})
    message(STATUS "Not in CLion, assuming Visual Studio or other IDE.")
    # Define MSVC macro if you're using Visual Studio
    if (MSVC)
        add_definitions(-DMSVC)
    endif()
else()
    message(STATUS "Building in CLion.")
    add_definitions(-DCLION)
endif()

add_executable("WC-Engine"
    ${SOURCES}
    ${IMGUI_SOURCES}
    "vendor/wc/imgui_backend/imgui_impl_glfw.cpp"
    "vendor/wc/imgui_backend/imgui_impl_vulkan.cpp"
    "vendor/imguizmo/ImGuizmo.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}"
)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    
    add_compile_definitions(
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=2>
        $<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>
    )
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

find_package(Vulkan REQUIRED)
set(VULKAN_SDK $ENV{VULKAN_SDK})
set(VULKAN_LIBRARY_DIRS "${VULKAN_SDK}/Lib")

add_subdirectory("vendor/glfw")
add_subdirectory("vendor/spdlog")
add_subdirectory("vendor/yaml-cpp")
add_subdirectory("vendor/glm")
add_subdirectory("vendor/freetype")
add_subdirectory("vendor/box2d")
add_subdirectory("vendor/flecs")
add_subdirectory("vendor/luau")

set(FREETYPE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/vendor/freetype/include")
#set(FREETYPE_LIBRARY ${CMAKE_BINARY_DIR}/vendor/freetype/freetyped.lib)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(FREETYPE_LIBRARY "${CMAKE_BINARY_DIR}/vendor/freetype/freetyped.lib")
else()
    set(FREETYPE_LIBRARY "${CMAKE_BINARY_DIR}/vendor/freetype/freetype.lib")
endif()
add_subdirectory("vendor/msdf-atlas-gen")

# Find all shader files
set(SHADER_IN_DIR "${PROJECT_SOURCE_DIR}/WC Engine/shaders/")
file(GLOB SHADER_SOURCES
        "${PROJECT_SOURCE_DIR}/WC Engine/shaders/*.vert"
        "${PROJECT_SOURCE_DIR}/WC Engine/shaders/*.frag"
        "${PROJECT_SOURCE_DIR}/WC Engine/shaders/*.comp")

file(GLOB SHADER_HEADERS
        "${SHADER_IN_DIR}/include/*.glsl")

# Create custom target for all shaders
add_custom_target(shaders ALL)

foreach(SHADER ${SHADER_SOURCES})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    add_custom_command(
            OUTPUT "${PROJECT_SOURCE_DIR}/WC Engine/workdir/assets/shaders/${SHADER_NAME}"
            COMMAND "$ENV{VULKAN_SDK}/Bin/glslc.exe"
            -std=450core
            -I"${SHADER_IN_DIR}/include"
            -o "${PROJECT_SOURCE_DIR}/WC Engine/workdir/assets/shaders/${SHADER_NAME}"
            -O0
            ${SHADER}
            DEPENDS ${SHADER}
            ${SHADER_HEADERS}
            COMMENT "Compiling ${SHADER_NAME}"
            VERBATIM
    )
    add_custom_target(${SHADER_NAME} DEPENDS "${PROJECT_SOURCE_DIR}/WC Engine/workdir/assets/shaders/${SHADER_NAME}")
    add_dependencies(shaders ${SHADER_NAME})
endforeach()

# Make shaders compile automatically when building the main target
add_dependencies(${PROJECT_NAME} shaders)

target_link_libraries(${PROJECT_NAME} 
Vulkan::Vulkan
glfw 
spdlog::spdlog 
yaml-cpp 
glm::glm 
box2d::box2d
freetype
freetype-interface
msdfgen
msdf-atlas-gen::msdf-atlas-gen
flecs::flecs_static
Luau.Analysis
Luau.Ast
Luau.Compiler
Luau.Config
Luau.Common
Luau.VM
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} $ENV{VULKAN_SDK}/Lib/spirv-cross-cored.lib)
else()
    target_link_libraries(${PROJECT_NAME} $ENV{VULKAN_SDK}/Lib/spirv-cross-core.lib)
endif()

target_include_directories(${PROJECT_NAME} PUBLIC 
"vendor/"
"vendor/box2d/include" # mad funny box2d
"vendor/imgui"
"vendor/luau/Analysis/include"
"vendor/luau/Ast/include"
"vendor/luau/CodeGen/include"
"vendor/luau/Common/include"
"vendor/luau/Compiler/include"
"vendor/luau/Config/include"
"vendor/luau/VM/include"
"vendor/flecs/include"
)
target_link_directories(${PROJECT_NAME} PUBLIC ${VULKAN_LIBRARY_DIRS})