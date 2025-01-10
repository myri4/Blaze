cmake_minimum_required(VERSION 3.8)

set(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}")

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
    "vendor/msdfgen/atlas-gen/bitmap-blit.cpp"
    "vendor/msdfgen/atlas-gen/charset-parser.cpp"
    "vendor/msdfgen/atlas-gen/Charset.cpp"
    "vendor/msdfgen/atlas-gen/csv-export.cpp"
    "vendor/msdfgen/atlas-gen/FontGeometry.cpp"
    "vendor/msdfgen/atlas-gen/glyph-generators.cpp"
    "vendor/msdfgen/atlas-gen/GlyphGeometry.cpp"
    "vendor/msdfgen/atlas-gen/image-encode.cpp"
    "vendor/msdfgen/atlas-gen/json-export.cpp"
    "vendor/msdfgen/atlas-gen/RectanglePacker.cpp"
    "vendor/msdfgen/atlas-gen/shadron-preview-generator.cpp"
    "vendor/msdfgen/atlas-gen/size-selectors.cpp"
    "vendor/msdfgen/atlas-gen/TightAtlasPacker.cpp"
    "vendor/msdfgen/atlas-gen/utf8.cpp"
    "vendor/msdfgen/atlas-gen/Workload.cpp"
    "vendor/msdfgen/core/contour-combiners.cpp"
    "vendor/msdfgen/core/Contour.cpp"
    "vendor/msdfgen/core/edge-coloring.cpp"
    "vendor/msdfgen/core/edge-segments.cpp"
    "vendor/msdfgen/core/edge-selectors.cpp"
    "vendor/msdfgen/core/EdgeHolder.cpp"
    "vendor/msdfgen/core/equation-solver.cpp"
    "vendor/msdfgen/core/msdf-error-correction.cpp"
    "vendor/msdfgen/core/MSDFErrorCorrection.cpp"
    "vendor/msdfgen/core/msdfgen.cpp"
    "vendor/msdfgen/core/Projection.cpp"
    "vendor/msdfgen/core/rasterization.cpp"
    "vendor/msdfgen/core/render-sdf.cpp"
    "vendor/msdfgen/core/save-bmp.cpp"
    "vendor/msdfgen/core/save-tiff.cpp"
    "vendor/msdfgen/core/Scanline.cpp"
    "vendor/msdfgen/core/sdf-error-estimation.cpp"
    "vendor/msdfgen/core/shape-description.cpp"
    "vendor/msdfgen/core/Shape.cpp"
    "vendor/msdfgen/core/SignedDistance.cpp"
    "vendor/msdfgen/core/Vector2.cpp"
    "vendor/msdfgen/ext/import-font.cpp"
    "vendor/msdfgen/ext/import-svg.cpp"
    "vendor/msdfgen/ext/resolve-shape-geometry.cpp"
    "vendor/msdfgen/ext/save-png.cpp"
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


target_link_libraries(${PROJECT_NAME} 
Vulkan::Vulkan
glfw 
spdlog::spdlog 
yaml-cpp 
glm::glm 
freetype
box2d::box2d
)
target_include_directories(${PROJECT_NAME} PUBLIC 
"vendor/"
"vendor/box2d/include" # mad funny box2d
"vendor/imgui"
)
target_link_directories(${PROJECT_NAME} PUBLIC ${VULKAN_LIBRARY_DIRS})