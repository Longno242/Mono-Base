cmake_minimum_required(VERSION 3.20)
project(MonoBase LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

if(NOT EXISTS "${VENDOR_DIR}/minhook/CMakeLists.txt")
    message(FATAL_ERROR "MinHook not found. Run: scripts/setup_deps.ps1")
endif()

if(NOT EXISTS "${VENDOR_DIR}/imgui/imgui.h")
    message(FATAL_ERROR "Dear ImGui not found. Run: scripts/setup_deps.ps1")
endif()

add_subdirectory("${VENDOR_DIR}/minhook" EXCLUDE_FROM_ALL)

set(IMGUI_DIR "${VENDOR_DIR}/imgui")
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_win32.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx11.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_dx12.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    list(APPEND IMGUI_SOURCES "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp")
endif()

add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)

file(GLOB_RECURSE MOD_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_library(MonoBase SHARED ${MOD_SOURCES})
target_include_directories(MonoBase PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(MonoBase PRIVATE minhook imgui d3d11 d3d12 dxgi opengl32)

if(Vulkan_FOUND)
    target_compile_definitions(MonoBase PRIVATE MONO_BASE_HAS_VULKAN=1)
    target_include_directories(MonoBase PRIVATE ${Vulkan_INCLUDE_DIRS})
    target_link_libraries(MonoBase PRIVATE Vulkan::Vulkan)
    message(STATUS "Vulkan SDK found — Vulkan renderer enabled")
else()
    message(STATUS "Vulkan SDK not found — Vulkan renderer will log a build-time warning")
endif()

target_compile_definitions(MonoBase PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

if(MSVC)
    target_compile_options(MonoBase PRIVATE /W4 /permissive-)
endif()

set_target_properties(MonoBase PROPERTIES
    OUTPUT_NAME "MonoBase"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
