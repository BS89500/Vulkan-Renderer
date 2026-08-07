#include <iostream>
// Include your custom Window header
#include "Window.h"

int main() {
    Window appWindow(1024, 768, "Audio Visualizer");
    std::cout << "Engine successfully started." << std::endl;

    while (!appWindow.ShouldClose()) {
        appWindow.PollEvents();
        appWindow.DrawFrame();
    }
    appWindow.WaitIdle();
}