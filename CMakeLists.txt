cmake_minimum_required(VERSION 3.20)

project(Genesis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Dependencies ---

# 1. Raylib
add_subdirectory(external/raylib)

# 2. Creating ImGui Library
# Include all core ImGui source files
file(GLOB IMGUI_SOURCES
    "external/imgui/imgui.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_tables.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/imgui_demo.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC external/imgui)

# 3. Creating rlImGui Library
add_library(rlImGui STATIC "external/rlImGui/rlImGui.cpp")
target_include_directories(rlImGui PUBLIC external/rlImGui)
target_link_libraries(rlImGui PUBLIC raylib imgui)

# --- Genesis Application ---

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(Genesis ${SOURCES} ${HEADERS})

target_include_directories(Genesis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Link Dependencies
target_link_libraries(Genesis PRIVATE raylib imgui rlImGui)

if(APPLE)
    # macOS specific framework requirements (handled by Raylib usually, but good to ensure)
    target_link_libraries(Genesis PRIVATE "-framework IOKit" "-framework Cocoa" "-framework OpenGL")
endif()
