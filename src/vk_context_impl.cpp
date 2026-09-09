// Created by Jens Kromdijk 23/04/2026
// Contains actual implementation of context class

#include "vk_context.h"
#include "vk_init.h"
#include "constants.h"
#include "util.h"
#include "vk_image.h"
#include "vk_pipelines.h"

#include <cstdlib>
#include <functional>
#include <algorithm>
#include <set>
#include <limits>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

void Context::init()
{
    assert(!m_init);
    initWindow();
    initVulkan();

    m_init = true;
}

void Context::draw()
{
    // make sure everything is finished
    // timeout is in nanoseconds
    VK_CHECK(vkWaitForFences(m_device, 1, &getCurrentFrame().m_renderFence, true, 1e+9));
    VK_CHECK(vkResetFences(m_device, 1, &getCurrentFrame().m_renderFence));

    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(
        m_device, m_swapchain, 1e+9, getCurrentFrame().m_swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd{getCurrentFrame().m_commandBuffer};
    // reset the command buffer
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // begin recording
    VkCommandBufferBeginInfo cmdBeginInfo{};
    VkUtil::commandBufferBeginInfo(&cmdBeginInfo, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    m_drawExtent.width = m_drawImage.m_extent.width;
    m_drawExtent.height = m_drawImage.m_extent.height;

    // start the actual recording :)
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    VkUtil::transitionImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // clear image
    drawBackground(cmd);

    // make it into presentable mode
    VkUtil::transitionImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkUtil::transitionImage(
        cmd,
        m_swapchainImages[swapchainImageIndex].m_image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkUtil::copyImage2Image(
        cmd, m_drawImage.m_image, m_swapchainImages[swapchainImageIndex].m_image, m_drawExtent, m_swapchainExtent);

    VkUtil::transitionImage(
        cmd,
        m_swapchainImages[swapchainImageIndex].m_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // finish command buffer (can be executed now)
    VK_CHECK(vkEndCommandBuffer(cmd));

    // prepare queue submission
    VkCommandBufferSubmitInfo cmdInfo{};
    VkUtil::commandBufferSubmitInfo(&cmdInfo, cmd);

    VkSemaphoreSubmitInfo waitInfo{};
    VkUtil::semaphoreSubmitInfo(
        &waitInfo, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, getCurrentFrame().m_swapchainSemaphore);

    VkSemaphoreSubmitInfo signalInfo{};
    VkUtil::semaphoreSubmitInfo(
        &signalInfo, VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_swapchainImages[swapchainImageIndex].m_semaphore);

    VkSubmitInfo2 submit{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2, .pNext = nullptr, .flags = 0};
    VkUtil::submitInfo(&submit, &cmdInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, getCurrentFrame().m_renderFence));

    // present the actual image
    VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &m_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &m_swapchainImages[swapchainImageIndex].m_semaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;

    VK_CHECK(vkQueuePresentKHR(m_graphicsQueue, &presentInfo));
    tickFrame();
}

void Context::immediateSubmit(std::function<void(VkCommandBuffer)>&& function)
{
    VK_CHECK(vkResetFences(m_device, 1, &m_immFence));
    VK_CHECK(vkResetCommandBuffer(m_immCommandBuffer, 0));

    VkCommandBuffer cmd{m_immCommandBuffer};
    VkCommandBufferBeginInfo cmdBeginInfo{};
    VkUtil::commandBufferBeginInfo(&cmdBeginInfo, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo{};
    VkUtil::commandBufferSubmitInfo(&cmdInfo, cmd);
    VkSubmitInfo2 submit{};
    VkUtil::submitInfo(&submit, &cmdInfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(m_graphicsQueue, 1, &submit, m_immFence));
    VK_CHECK(vkWaitForFences(m_device, 1, &m_immFence, true, 9999999999));
}

void Context::drawBackground(VkCommandBuffer cmd)
{
    // VkClearColorValue clearValue;
    // const float flash{std::abs(std::sin(static_cast<float>(getFrameNumber() / 2400.f)))};
    // clearValue = {{0.0f, 0.0f, flash, 1.0f}};

    // VkImageSubresourceRange clearRange{VkUtil::imageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT)};
    // vkCmdClearColorImage(cmd, m_drawImage.m_image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradientPipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_gradientPipelineLayout, 0, 1, &m_drawImageDescriptors, 0, nullptr);
    vkCmdDispatch(cmd, std::ceil(m_drawExtent.width / 16.0), std::ceil(m_drawExtent.height / 16.0), 1);
}

void Context::initWindow()
{
    CHECK(SDL_Init(SDL_INIT_VIDEO));
    SDL_WindowFlags windowFlags{SDL_WINDOW_VULKAN};

    m_window = SDL_CreateWindow("Vulkan Window", CST::WIDTH, CST::HEIGHT, windowFlags);
    CHECK((m_window != nullptr));
}

void Context::initVulkan()
{
    volkInitialize();

    createInstance();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();

    createAllocator();

    createSwapchain();
    createImageViews();

    initCommands();
    initSyncStructures();
    initDescriptors();
    initPipelines();
}

void Context::createInstance()
{

    // get validation layers and extensions
    std::vector<std::string> validationLayers{};
#ifdef _DEBUG
    for (std::size_t i{0}; i < CST::validationLayers.size(); ++i)
    {
        validationLayers.emplace_back(CST::validationLayers[i]);
    }
#endif

    uint32_t sdlExtensionCount{0};
    char const* const* sdlExtensions{SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount)};

    std::vector<std::string> requiredExtensions(static_cast<std::size_t>(sdlExtensionCount));
    for (std::size_t i{0}; i < static_cast<std::size_t>(sdlExtensionCount); ++i)
    {
        requiredExtensions[i] = std::string(sdlExtensions[i]);
    }

#ifdef _DEBUG
    requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    m_enabledInstanceLayers = Util::filterExtensions(enumerateInstanceLayers(), validationLayers);
    m_enabledInstanceExtensions = Util::filterExtensions(enumerateInstanceExtensions(), requiredExtensions);

#ifdef _DEBUG
    fmt::print("Enabled layers: \n");
    for (const std::string& layer : m_enabledInstanceLayers)
    {
        fmt::print("\t{}\n", layer);
    }

    fmt::print("Enabled extensions: \n");
    for (const std::string& extension : m_enabledInstanceExtensions)
    {
        fmt::println("\t{}", extension);
    }
#endif

    std::vector<const char*> instanceLayers(m_enabledInstanceLayers.size());
    std::transform(
        m_enabledInstanceLayers.begin(),
        m_enabledInstanceLayers.end(),
        instanceLayers.begin(),
        std::mem_fn(&std::string::c_str));

    std::vector<const char*> instanceExtensions(m_enabledInstanceExtensions.size());
    std::transform(
        m_enabledInstanceExtensions.begin(),
        m_enabledInstanceExtensions.end(),
        instanceExtensions.begin(),
        std::mem_fn(&std::string::c_str));

    const VkApplicationInfo appInfo{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan Window",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3};

    VkInstanceCreateInfo instanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(instanceLayers.size()),
        .ppEnabledLayerNames = instanceLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size()),
        .ppEnabledExtensionNames = instanceExtensions.data()};

    VkDebugUtilsMessengerCreateInfoEXT debugCI{};
#ifdef _DEBUG
    setupDebugMessenger(debugCI);
    instanceCI.pNext = &debugCI;
#else
    instanceCI.pNext = nullptr;
#endif

    instanceCI.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    VK_CHECK(vkCreateInstance(&instanceCI, nullptr, &m_instance));

    volkLoadInstance(m_instance);

#ifdef _DEBUG
    VK_CHECK(vkCreateDebugUtilsMessengerEXT(m_instance, &debugCI, nullptr, &m_debugMessenger));
#endif
}

void Context::createSurface()
{
    CHECK(SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface));
    fmt::println("Created surface");
}

void Context::selectPhysicalDevice()
{

    fmt::println("Selecting physical device...");
    uint32_t physicalDeviceCount{0};
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr));
    CHECK((physicalDeviceCount != 0));
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data()));

    for (const VkPhysicalDevice& device : physicalDevices)
    {
        if (deviceSuitable(device))
        {
            m_physicalDevice = device;
            break;
        }
    }

    CHECK((m_physicalDevice != VK_NULL_HANDLE));

#ifdef _DEBUG
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    fmt::println("Selected physical device: ");
    fmt::println("\t{}", deviceProperties.deviceName);
#endif
}

void Context::createLogicalDevice()
{
    fmt::println("Creating logical device...");
    QueueFamilyIndices indices{findQueueFamilies(m_physicalDevice)};
    m_queueFamilyIndices = indices;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    std::set<uint32_t> uniqueQueueFamiles{indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority{1.0f};
    for (uint32_t queueFamily : uniqueQueueFamiles)
    {
        VkDeviceQueueCreateInfo queueCI{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority};
        queueCreateInfos.push_back(queueCI);
    }

    // TODO: Validate feature support
    VkPhysicalDeviceVulkan13Features features13{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    features13.dynamicRendering = VK_TRUE;
    features13.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    features12.bufferDeviceAddress = VK_TRUE;
    features12.descriptorIndexing = VK_TRUE;
    features12.pNext = &features13;

    VkPhysicalDeviceFeatures deviceFeatures{};

    // actual logical device
    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features12,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .pEnabledFeatures = &deviceFeatures};

    createInfo.enabledExtensionCount = static_cast<uint32_t>(CST::deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = CST::deviceExtensions.data();
#ifdef _DEBUG
    std::vector<const char*> instanceLayers(m_enabledInstanceLayers.size());
    std::transform(
        m_enabledInstanceLayers.begin(),
        m_enabledInstanceLayers.end(),
        instanceLayers.begin(),
        std::mem_fn(&std::string::c_str));
    createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
    createInfo.ppEnabledLayerNames = instanceLayers.data();
#else
    createInfo.enabledLayerCount = 0;
#endif

    VK_CHECK(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device));
    volkLoadDevice(m_device);

    vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
}

void Context::createSwapchain()
{
    SwapChainSupportDetails details{checkSwapchainSupport(m_physicalDevice)};

    VkSurfaceFormatKHR surfaceFormat{selectSwapchainSurfaceFormat(details.formats)};
    VkPresentModeKHR presentMode{selectSwapchainPresentMode(details.presentModes)};
    VkExtent2D extent{selectSwapExtent(details.capabilities)};

    uint32_t imageCount{details.capabilities.minImageCount + 1};
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
    {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_surface;
    // image details
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // amount of layers each image has
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    QueueFamilyIndices indices{findQueueFamilies(m_physicalDevice)};
    // must survive longer so not in condition scope
    uint32_t queueFamilyIndices[]{indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain));

    fmt::println("Created swap chain: {} * {}", extent.width, extent.height);

    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr));
    m_swapchainImages.resize(imageCount);
    std::vector<VkImage> tempSwapchainImages(imageCount);
    VK_CHECK(vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, tempSwapchainImages.data()));
    std::transform(
        tempSwapchainImages.begin(),
        tempSwapchainImages.end(),
        m_swapchainImages.begin(),
        [](VkImage image)
        { return SwapchainImage{.m_image = image, .m_view = VK_NULL_HANDLE, .m_semaphore = VK_NULL_HANDLE}; });

    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;

    m_drawImage.m_imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    const VkExtent3D drawImageExtent{extent.width, extent.height, 1};
    m_drawImage.m_extent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimgInfo{};
    VkUtil::imageCreateInfo(&rimgInfo, m_drawImage.m_imageFormat, drawImageUsages, drawImageExtent);

    VmaAllocationCreateInfo rimgAllocInfo{};
    rimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vmaCreateImage(m_allocator, &rimgInfo, &rimgAllocInfo, &m_drawImage.m_image, &m_drawImage.m_allocation, nullptr);

    VkImageViewCreateInfo imageViewCI{};
    VkUtil::imageViewCreateInfo(
        &imageViewCI, m_drawImage.m_imageFormat, m_drawImage.m_image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(m_device, &imageViewCI, nullptr, &m_drawImage.m_imageView));

    m_deletionQueue.push_function(
        [&]()
        {
            vkDestroyImageView(m_device, m_drawImage.m_imageView, nullptr);
            vmaDestroyImage(m_allocator, m_drawImage.m_image, m_drawImage.m_allocation);
        });
}

void Context::createAllocator()
{
    VmaAllocatorCreateInfo allocatorCI{};
    VmaVulkanFunctions vkFunctions{
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = vkGetDeviceProcAddr,
        .vkCreateImage = vkCreateImage};
    allocatorCI.physicalDevice = m_physicalDevice;
    allocatorCI.device = m_device;
    allocatorCI.instance = m_instance;
    allocatorCI.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorCI.pVulkanFunctions = &vkFunctions;
    vmaCreateAllocator(&allocatorCI, &m_allocator);

    fmt::println("Created VMA allocator!");
}

void Context::free()
{
    vkDeviceWaitIdle(m_device);
#ifdef _DEBUG
    if (m_debugMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }
#endif

    // clean up sync structures (as well as command pool ofc)
    for (std::size_t i{0}; i < FRAME_OVERLAP; ++i)
    {
        vkDestroyCommandPool(m_device, m_frames[i].m_commandPool, nullptr);

        vkDestroyFence(m_device, m_frames[i].m_renderFence, nullptr);
        vkDestroySemaphore(m_device, m_frames[i].m_swapchainSemaphore, nullptr);
        m_frames[i].m_deletionQueue.flush();
    }
    m_deletionQueue.flush();

    vmaDestroyAllocator(m_allocator);

    freeSwapchain();

    vkDestroyDevice(m_device, nullptr);
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    vkDestroyInstance(m_instance, nullptr);

    SDL_DestroyWindow(m_window);

    fmt::println("Cleaned up!");
}

[[nodiscard]] std::vector<std::string> Context::enumerateInstanceLayers()
{

    uint32_t instanceLayerCount{0};
    VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr));
    std::vector<VkLayerProperties> layers(instanceLayerCount);
    VK_CHECK(vkEnumerateInstanceLayerProperties(&instanceLayerCount, layers.data()));

    std::vector<std::string> availableLayers;
    std::transform(
        layers.begin(),
        layers.end(),
        std::back_inserter(availableLayers),
        [](const VkLayerProperties& properties) { return properties.layerName; });

#ifdef _DEBUG
    fmt::println("Found {} available layer(s):", instanceLayerCount);
    for (const std::string& layer : availableLayers)
    {
        fmt::println("\t{}", layer);
    }
#endif
    return availableLayers;
}

[[nodiscard]] std::vector<std::string> Context::enumerateInstanceExtensions()
{

    uint32_t instanceExtensionCount{0};
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr));
    std::vector<VkExtensionProperties> extensions(instanceExtensionCount);
    VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, extensions.data()));

    std::vector<std::string> availableExtensions;
    std::transform(
        extensions.begin(),
        extensions.end(),
        std::back_inserter(availableExtensions),
        [](const VkExtensionProperties& properties) { return properties.extensionName; });

#ifdef _DEBUG
    fmt::println("Found {} available extension(s):", instanceExtensionCount);
    for (const std::string& extension : availableExtensions)
    {
        fmt::println("\t{}", extension);
    }
#endif
    return availableExtensions;
}

[[nodiscard]] bool Context::deviceSuitable(VkPhysicalDevice device) const
{

    fmt::println("Checking if device is suitable...");
    QueueFamilyIndices indices{findQueueFamilies(device)};

    const bool extensionsSupported(checkDeviceExtensionsSupport(device));

    bool swapChainValid{false};
    if (extensionsSupported)
    {
        SwapChainSupportDetails details{checkSwapchainSupport(device)};
        swapChainValid = !details.formats.empty() && !details.presentModes.empty();
    }
    return indices.complete() && extensionsSupported && swapChainValid;
}

[[nodiscard]] bool Context::checkDeviceExtensionsSupport(VkPhysicalDevice device) const
{

    uint32_t extensionCount;
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr));

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    VK_CHECK(vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data()));

    std::set<std::string> requiredExtensions(CST::deviceExtensions.begin(), CST::deviceExtensions.end());

    for (const VkExtensionProperties& extension : availableExtensions)
    {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

[[nodiscard]] QueueFamilyIndices Context::findQueueFamilies(VkPhysicalDevice device) const
{
    // TODO: Check if physical device supports Vulkan 1.3
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    fmt::println("Got queue families...");

    int i{0};
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }
        fmt::println("Checked graphics bit...");

        // NOTE: Gives seg fault if surface hasn't been created yet
        VkBool32 presentSupport{false};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        fmt::println("Checked presentation support...");

        if (indices.complete())
        {
            break;
        }
        ++i;
    }

    return indices;
}

[[nodiscard]] SwapChainSupportDetails Context::checkSwapchainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities));

    uint32_t formatCount;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr));

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data()));
    }

    uint32_t presentModeCount;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr));

    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

[[nodiscard]] VkSurfaceFormatKHR
Context::selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
{
    for (const VkSurfaceFormatKHR& format : formats)
    {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // first one is usually fine
    return formats[0];
}

[[nodiscard]] VkPresentModeKHR
Context::selectSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
{
    // for (const VkPresentModeKHR& presentMode : presentModes)
    // {
    //     if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) // ooh yea we like triple buffering
    //     {
    //         return presentMode;
    //     }
    // }
    return VK_PRESENT_MODE_FIFO_KHR; // only one guaranteed to be available
}

[[nodiscard]] VkExtent2D Context::selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }
    int width, height;
    SDL_GetWindowSize(m_window, &width, &height);

    VkExtent2D extent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return extent;
}

void Context::createImageViews()
{
    for (std::size_t i{0}; i < m_swapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i].m_image;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        VK_CHECK(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImages[i].m_view));
    }
    fmt::println("Created {} image views...", m_swapchainImages.size());
}

void Context::freeSwapchain()
{
    for (SwapchainImage& image : m_swapchainImages)
    {
        vkDestroyImageView(m_device, image.m_view, nullptr);
        vkDestroySemaphore(m_device, image.m_semaphore, nullptr);
    }
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void Context::recreateSwapchain()
{
    vkDeviceWaitIdle(m_device);

    freeSwapchain();

    createSwapchain();
    createImageViews();

    VkSemaphoreCreateInfo semaphoreCI{};
    VkUtil::semaphoreCreateInfo(&semaphoreCI, 0);
    for (std::size_t i{0}; i < m_swapchainImages.size(); ++i)
    {
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_swapchainImages[i].m_semaphore));
    }
}

void Context::initCommands()
{
    VkCommandPoolCreateInfo commandCI{};
    VkUtil::commandPoolCreateInfo(&commandCI, m_queueFamilyIndices.graphicsFamily.value());

    for (std::size_t i{0}; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateCommandPool(m_device, &commandCI, nullptr, &m_frames[i].m_commandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo{};
        VkUtil::commandBufferAllocInfo(&cmdAllocInfo, m_frames[i].m_commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_frames[i].m_commandBuffer));
    }

    VK_CHECK(vkCreateCommandPool(m_device, &commandCI, nullptr, &m_immCommandPool));

    VkCommandBufferAllocateInfo cmdAllocInfo{};
    VkUtil::commandBufferAllocInfo(&cmdAllocInfo, m_immCommandPool, 1);
    VK_CHECK(vkAllocateCommandBuffers(m_device, &cmdAllocInfo, &m_immCommandBuffer));

    m_deletionQueue.push_function([&]() { vkDestroyCommandPool(m_device, m_immCommandPool, nullptr); });
}

void Context::initSyncStructures()
{
    VkFenceCreateInfo fenceCI{};
    VkUtil::fenceCreateInfo(&fenceCI, VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCI{};
    VkUtil::semaphoreCreateInfo(&semaphoreCI, 0);

    for (std::size_t i{0}; i < FRAME_OVERLAP; ++i)
    {
        VK_CHECK(vkCreateFence(m_device, &fenceCI, nullptr, &m_frames[i].m_renderFence));

        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_frames[i].m_swapchainSemaphore));
    }

    for (std::size_t i{0}; i < m_swapchainImages.size(); ++i)
    {
        VK_CHECK(vkCreateSemaphore(m_device, &semaphoreCI, nullptr, &m_swapchainImages[i].m_semaphore));
    }

    VK_CHECK(vkCreateFence(m_device, &fenceCI, nullptr, &m_immFence));
    m_deletionQueue.push_function([&]() { vkDestroyFence(m_device, m_immFence, nullptr); });
}

void Context::initDescriptors()
{
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes{{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1.f}};
    m_globalDescriptorAllocator.initPool(m_device, 10, sizes);

    // neat in new scope
    {
        DescriptorLayoutBuilder builder;
        builder.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        m_drawImageDescriptorLayout = builder.build(m_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }

    m_drawImageDescriptors = m_globalDescriptorAllocator.allocate(m_device, m_drawImageDescriptorLayout);

    VkDescriptorImageInfo imgInfo{};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = m_drawImage.m_imageView;

    VkWriteDescriptorSet drawImageWrite{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    drawImageWrite.pNext = nullptr;

    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = m_drawImageDescriptors;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(m_device, 1, &drawImageWrite, 0, nullptr);

    m_deletionQueue.push_function(
        [&]()
        {
            m_globalDescriptorAllocator.destroyPool(m_device);
            vkDestroyDescriptorSetLayout(m_device, m_drawImageDescriptorLayout, nullptr);
        });
}

void Context::initPipelines() { initBackgroundPipelines(); }

void Context::initBackgroundPipelines()
{
    VkPipelineLayoutCreateInfo computeLayout{};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = &m_drawImageDescriptorLayout;
    computeLayout.setLayoutCount = 1;

    VK_CHECK(vkCreatePipelineLayout(m_device, &computeLayout, nullptr, &m_gradientPipelineLayout));

    VkShaderModule computeDrawShader;
    if (!VkUtil::loadShaderModule("data/shaders/gradient.comp.spv", m_device, &computeDrawShader))
    {
        fmt::println("{}Error building compute shader!{}", BEGIN_ERROR, END_ERROR);
        CRASHOUT();
    }

    VkPipelineShaderStageCreateInfo stageInfo{};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.pNext = nullptr;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = computeDrawShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo{};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = m_gradientPipelineLayout;
    computePipelineCreateInfo.stage = stageInfo;

    VK_CHECK(vkCreateComputePipelines(
        m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &m_gradientPipeline));

    vkDestroyShaderModule(m_device, computeDrawShader, nullptr);
    m_deletionQueue.push_function(
        [&]()
        {
            vkDestroyPipelineLayout(m_device, m_gradientPipelineLayout, nullptr);
            vkDestroyPipeline(m_device, m_gradientPipeline, nullptr);
        });
}

void Context::tickFrame()
{
    ++m_frameNumber;
    getFrameDeletionQueue().flush();
}

VKAPI_ATTR VkBool32 VKAPI_CALL Context::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    std::string colorCode{BEGIN_LOG};
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        colorCode = BEGIN_ERROR;
    }
    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        colorCode = BEGIN_WARNING;
    }

    fmt::println(stderr, "{}Validation layer: {}{}", colorCode, pCallbackData->pMessage, END_LOG);
    return VK_FALSE;
}

void Context::setupDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}

void Context::initImGUI() {}
