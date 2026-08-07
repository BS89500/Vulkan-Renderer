#include <iostream>
#include "Window.h"

int main() {
    Window appWindow(1024, 768, "Vulkan_Renderer");
    std::cout << "Engine successfully started." << std::endl;

    while (!appWindow.ShouldClose()) {
        appWindow.PollEvents();
        appWindow.DrawFrame();
    }
    appWindow.WaitIdle();
}