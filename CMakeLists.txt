cmake_minimum_required(VERSION 4.0)

set(CMAKE_CXX_STANDARD 20)
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/glfw")

project(Audio_Visualizer_Project)

find_package(Vulkan REQUIRED COMPONENTS volk shaderc_combined)
set(CMAKE_CXX_STANDARD 20)
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/glfw")

include_directories("${GLFW_DIR}/include")
find_library(GLFW_LIBRARY
        NAMES glfw3 glfw3dll
        PATHS "${GLFW_DIR}/lib-vc2022"
        NO_DEFAULT_PATH
        REQUIRED
)

add_executable(Audio_Visualizer_Project
        main.cpp
        Window.cpp
)

target_include_directories(Audio_Visualizer_Project PRIVATE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(Audio_Visualizer_Project PRIVATE
        Vulkan::Vulkan
        Vulkan::volk
        Vulkan::shaderc_combined
        ${GLFW_LIBRARY}
)
if (WIN32)
target_link_libraries(Audio_Visualizer_Project PRIVATE opengl32 gdi32 user32 shell32)
endif()
