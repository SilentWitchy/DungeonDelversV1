cmake_minimum_required(VERSION 3.20)
project(DungeonCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(DungeonCore
    src/main.cpp
    src/core/App.cpp
    src/core/Log.cpp
    src/gfx/Texture.cpp
    src/gfx/Renderer.cpp
    src/gfx/Font.cpp
    src/input/Input.cpp
    src/ui/Ui.cpp
)

target_include_directories(DungeonCore PRIVATE src)

# SDL2
# Works with:
# - vcpkg (preferred): find_package(SDL2 CONFIG REQUIRED)
# - system installs:  find_package(SDL2 REQUIRED)
find_package(SDL2 CONFIG QUIET)

if(SDL2_FOUND)
    message(STATUS "Found SDL2 via CONFIG.")
    target_link_libraries(DungeonCore PRIVATE SDL2::SDL2 SDL2::SDL2main)
else()
    message(STATUS "SDL2 CONFIG not found; trying MODULE mode.")
    find_package(SDL2 REQUIRED)
    target_include_directories(DungeonCore PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(DungeonCore PRIVATE ${SDL2_LIBRARIES})
endif()

# Copy assets next to the binary (simple dev workflow)
add_custom_command(TARGET DungeonCore POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:DungeonCore>/assets
)
