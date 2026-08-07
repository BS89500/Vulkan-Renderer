cmake_minimum_required(VERSION 4.0)

set(CMAKE_CXX_STANDARD 20)

project(Vulkan_Renderer)

find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

set(CMAKE_CXX_STANDARD 20)
set(GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/glfw")
set(VMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/vma")
set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/volk")

include_directories("${GLFW_DIR}/include")
include_directories("${VMA_DIR}") 
include_directories("${VOLK_DIR}")

find_library(GLFW_LIBRARY
        NAMES glfw3 glfw3dll
        PATHS "${GLFW_DIR}/lib-vc2022"
        NO_DEFAULT_PATH
        REQUIRED
)

add_executable(Vulkan_Renderer
        main.cpp
        Window.cpp
        "${VMA_DIR}/vma_implementation.cpp"
        "${VOLK_DIR}/volk.c"
)

target_include_directories(Vulkan_Renderer PRIVATE ${Vulkan_INCLUDE_DIRS})

target_link_libraries(Vulkan_Renderer PRIVATE
        Vulkan::Vulkan
        Vulkan::shaderc_combined
        ${GLFW_LIBRARY}
)
if (WIN32)
target_link_libraries(Vulkan_Renderer PRIVATE opengl32 gdi32 user32 shell32)
endif()
