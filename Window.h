//
// Created by ybsxm on 7/30/2026.
//

#ifndef AUDIO_VISUALIZER_PROJECT_WINDOW_H
#define AUDIO_VISUALIZER_PROJECT_WINDOW_H
#include <optional>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
class Window {
private:
    GLFWwindow* m_window;
    int m_width, m_height;
    const char* m_title;

    VkInstance m_vulkanInstance;
    VkPhysicalDevice m_physicalDevice;
    VkPhysicalDeviceProperties deviceProperties;
    VkSurfaceKHR m_surface;
    std::optional<uint32_t> m_graphicsQueueFamily;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkSwapchainKHR m_swapChain;

    VkCommandPool m_commandPool;
    VkCommandBuffer m_commandBuffer;

    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    VkFormat m_swapChainImageFormat; // We need to save the format for later
    VkExtent2D m_swapChainExtent;    // We need to save the resolution for later

    VkSemaphore m_imageAvailableSemaphore;
    VkSemaphore m_renderFinishedSemaphore;
    VkFence m_inFlightFence;

    void InitVulkan();
    void PickPhysicalDevice();
    void FindQueueFamilies();
    void CreateLogicalDevice();
    void CreateSurface();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateSyncObjects();


public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    std::string GetPhysicalDevice();

    bool ShouldClose() const;
    void PollEvents() const;
    void CreateCommandPool();
    void CreateCommandBuffer();
    void DrawFrame();
    void WaitIdle();

    GLFWwindow* GetNativeWindow() const;

};


#endif //AUDIO_VISUALIZER_PROJECT_WINDOW_H