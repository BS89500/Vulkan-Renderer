#include "Window.h"

#include <iostream>
#include <stdexcept>
#include <vector>

Window::Window(int width, int height, const char* title)
        : m_width(width), m_height(height), m_title(title), m_window(nullptr)
{
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);

    if (!m_window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    InitVulkan();

    // WARNING: The Surface MUST be created BEFORE picking the physical device
    // in a complete engine,
    CreateSurface();

    PickPhysicalDevice();
    FindQueueFamilies();
    CreateLogicalDevice();
    CreateSwapChain();

    CreateImageViews();

    CreateCommandPool();
    CreateCommandBuffer();

    CreateSyncObjects();
}

Window::~Window() {
    if (m_imageAvailableSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
    }
    if (m_renderFinishedSemaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
    }
    if (m_inFlightFence != VK_NULL_HANDLE) {
        vkDestroyFence(m_device, m_inFlightFence, nullptr);
    }
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
    }

    for (auto imageView : m_swapChainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    if (m_swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    }

    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_vulkanInstance, m_surface, nullptr);
    }

    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
    }

    if (m_vulkanInstance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_vulkanInstance, nullptr);
    }
    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

bool Window::ShouldClose() const {
        return glfwWindowShouldClose(m_window);
    }

void Window::PollEvents() const {
        glfwPollEvents();
    }

GLFWwindow* Window::GetNativeWindow() const {
        return m_window;
    }

std::string Window::GetPhysicalDevice() {
    return deviceProperties.deviceName;
}

void Window::PickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_vulkanInstance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        std::cout << "Found GPU: " << deviceProperties.deviceName << std::endl;

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            m_physicalDevice = device;
            std::cout << "Selected Discrete GPU: " << deviceProperties.deviceName << std::endl;
            break;
        }
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            std::cout << "Integrated GPU: " << deviceProperties.deviceName << std::endl;
            m_physicalDevice = device;
            break;
        }
    }

    // Absolute fallback: Just grab the first thing in the list
    if (m_physicalDevice == VK_NULL_HANDLE && deviceCount > 0) {
        m_physicalDevice = devices[0];
        std::cout << "Fell back to default GPU.\n";
    }

    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU!");
    }
}

void Window::InitVulkan() {
    VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Audio Visualizer",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "Engine",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_4
    };


    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    VkInstanceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = 0,
        .enabledExtensionCount = glfwExtensionCount,
        .ppEnabledExtensionNames = glfwExtensions,
    };


    if (vkCreateInstance(&createInfo, nullptr, &m_vulkanInstance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance!");
    }

    std::cout << "Successfully created Vulkan Instance!" << std::endl;
}
void Window::FindQueueFamilies() {
    uint32_t queueFamilyCount = 0;
    std::cout << m_physicalDevice << std::endl;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {

        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsQueueFamily = i;
            std::cout << "Found Graphics Queue Family at index: " << i << std::endl;
            break;
        }

        i++;
    }

    if (!m_graphicsQueueFamily.has_value()) {
        throw std::runtime_error("Failed to find a Vulkan queue family that supports graphics!");
    }
}

void Window::CreateLogicalDevice() {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_graphicsQueueFamily.value();
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .pEnabledFeatures = &deviceFeatures
    };

    // The most important one is VK_KHR_SWAPCHAIN_EXTENSION_NAME.
    // We NEED this extension to be able to present images to a screen window.
    const std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    // 5. Create the logical device!
    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device!");
    }

    // 6. Retrieve the handle to the queue we just created
    // We ask for queue index 0 from our graphics queue family
    vkGetDeviceQueue(m_device, m_graphicsQueueFamily.value(), 0, &m_graphicsQueue);

    std::cout << "Successfully created Logical Device and retrieved Graphics Queue.\n" << std::endl;
}

void Window::CreateSurface() {
    // GLFW magically handles the OS-specific Win32/X11 code to bridge the window to Vulkan!
    if (glfwCreateWindowSurface(m_vulkanInstance, m_window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
    std::cout << "Successfully created Window Surface.\n" << std::endl;
}

void Window::CreateSwapChain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    if (formatCount == 0) {
        throw std::runtime_error("Failed to find any supported surface formats!");
    }

    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    m_swapChainImageFormat = surfaceFormat.format;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    m_swapChainExtent = { static_cast<uint32_t>(m_width), static_cast<uint32_t>(m_height) };

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = m_surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = m_swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    std::cout << "Successfully created Swap Chain with format: " << surfaceFormat.format << std::endl;
}
void Window::CreateImageViews() {
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);

    m_swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
    m_swapChainImageViews.resize(imageCount);

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = m_swapChainImages[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = m_swapChainImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };

        if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }

    std::cout << "Successfully created " << imageCount << " Image Views." << std::endl;
}

void Window::CreateCommandPool() {
    // We must tell the command pool which Queue Family it is allocating for.
    // We also use the RESET flag so we can clear out old drawing commands every frame
    // and record new ones without constantly reallocating memory.
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = m_graphicsQueueFamily.value()
    };

    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool!");
    }

    std::cout << "Successfully created Command Pool." << std::endl;
}

void Window::CreateCommandBuffer() {
    // We allocate the buffer from the pool we just created.
    // PRIMARY level buffers can be submitted directly to a queue.
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = m_commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if (vkAllocateCommandBuffers(m_device, &allocInfo, &m_commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer!");
    }

    std::cout << "Successfully allocated Command Buffer." << std::endl;
}

void Window::CreateSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) != VK_SUCCESS ||
        vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence) != VK_SUCCESS) {

        throw std::runtime_error("Failed to create synchronization objects!");
        }

    std::cout << "Successfully created Synchronization Objects." << std::endl;
}

void Window::DrawFrame() {
    // 1. Wait for the previous frame to finish
    vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_device, 1, &m_inFlightFence);

    // 2. Ask the Swap Chain for the index of the next available image
    uint32_t imageIndex;
    vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // 3. Reset the command buffer to start recording fresh commands
    vkResetCommandBuffer(m_commandBuffer, 0);

    // 4. Begin recording
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(m_commandBuffer, &beginInfo);

    // --- DYNAMIC RENDERING: TRANSITION THE IMAGE TO "DRAWABLE" STATE ---
    // Before we can draw to a swap chain image, it must be in the ATTACHMENT_OPTIMAL layout.
    VkImageMemoryBarrier imageBarrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = m_swapChainImages[imageIndex],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };
    vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

    // --- DYNAMIC RENDERING: BEGIN RENDERING AND CLEAR COLOR ---
    // Pick your background color! (R, G, B, Alpha)
    VkClearValue clearColor = {{{0.1f, 0.2f, 0.4f, 1.0f}}};

    VkRenderingAttachmentInfo colorAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_swapChainImageViews[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // CLEAR the screen at the start!
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clearColor
    };

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = { .offset = {0, 0}, .extent = m_swapChainExtent },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment
    };

    // Start rendering (this replaces Render Passes)
    vkCmdBeginRendering(m_commandBuffer, &renderingInfo);

    // (Later, this is where vkCmdDraw will go)

    vkCmdEndRendering(m_commandBuffer);

    // --- TRANSITION THE IMAGE TO "PRESENTABLE" STATE ---
    // Before we can show it on screen, the OS requires a specific layout.
    imageBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    imageBarrier.dstAccessMask = 0;
    imageBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

    // 5. Finish recording
    vkEndCommandBuffer(m_commandBuffer);

    // 6. Submit the commands to the GPU Queue
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_imageAvailableSemaphore,
        .pWaitDstStageMask = waitStages,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_renderFinishedSemaphore
    };

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer!");
    }

    // 7. Tell the OS to display the image on the screen
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderFinishedSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &m_swapChain,
        .pImageIndices = &imageIndex
    };

    vkQueuePresentKHR(m_graphicsQueue, &presentInfo);
}

void Window::WaitIdle() {
    vkDeviceWaitIdle(m_device);
}