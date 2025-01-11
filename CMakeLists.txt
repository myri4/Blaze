cmake_minimum_required(VERSION 3.8)

set(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}")
set(VENDOR "vendor/")

project("WC-Engine")
add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN)

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/WC Engine/*.cpp" "${CMAKE_SOURCE_DIR}/WC Engine/*.h")
file(GLOB IMGUI_SOURCES 
"vendor/imgui/*.cpp" "vendor/imgui/*.h"
"vendor/imgui/misc/cpp/*.cpp" "vendor/imgui/misc/cpp/*.h"
)
    
add_executable("WC-Engine" 
    ${SOURCES}
    ${IMGUI_SOURCES}
    "vendor/wc/imgui_backend/imgui_impl_glfw.cpp"
    "vendor/wc/imgui_backend/imgui_impl_vulkan.cpp"
    "vendor/imguizmo/ImGuizmo.cpp"
    "vendor/flecs/flecs.c"
    "vendor/tinyxml2.cpp"
    "vendor/lodepng.cpp"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIRECTORY}"
)

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

set(FREETYPE_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/vendor/freetype/include")
set(FREETYPE_LIBRARY ${CMAKE_BINARY_DIR}/vendor/freetype/freetype.lib)
add_subdirectory("vendor/msdf-atlas-gen")


target_link_libraries(${PROJECT_NAME} 
Vulkan::Vulkan
glfw 
spdlog::spdlog 
yaml-cpp 
glm::glm 
box2d::box2d
freetype
freetype-interface
msdf-atlas-gen::msdf-atlas-gen
)
target_include_directories(${PROJECT_NAME} PUBLIC 
"vendor/"
"vendor/box2d/include" # mad funny box2d
"vendor/imgui"
)
target_link_directories(${PROJECT_NAME} PUBLIC ${VULKAN_LIBRARY_DIRS})