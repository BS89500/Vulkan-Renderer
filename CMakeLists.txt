cmake_minimum_required(VERSION 4.0)

set(CMAKE_CXX_STANDARD 20)
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/glfw")


project(Audio_Visualizer_Project)

include_directories("${GLFW_DIR}/include")
find_library(GLFW_LIBRARY
        NAMES glfw3 glfw3dll
        PATHS "${GLFW_DIR}/lib-mingw-w64"
        NO_DEFAULT_PATH
        REQUIRED
)

add_executable(Audio_Visualizer_Project
        main.cpp
        Window.cpp
)
# 1. Ask CMake to find the Vulkan SDK on your computer
find_package(Vulkan REQUIRED)

# 2. Add the Vulkan headers to your include path
target_include_directories(Audio_Visualizer_Project PRIVATE ${Vulkan_INCLUDE_DIRS})

# 3. Link the Vulkan library to your executable
target_link_libraries(Audio_Visualizer_Project PRIVATE Vulkan::Vulkan)
target_link_libraries(Audio_Visualizer_Project PRIVATE ${GLFW_LIBRARY})
if (WIN32)
    target_link_libraries(Audio_Visualizer_Project PRIVATE opengl32 gdi32 user32 shell32)
endif()
