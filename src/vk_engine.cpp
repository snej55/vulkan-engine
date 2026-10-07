// Created by Jens Kromdijk 12/09/2026

#include "vk_engine.h"
#include "util.h"
#include "constants.h"

#include <vector>
#include <map>
#include <set>

#include <SDL3/SDL_vulkan.h>

void VkEngine::init()
{
    initWindow();
    initVulkan();
}

void VkEngine::free()
{
    // vkDeviceWaitIdle(m_device);
#ifdef _DEBUG
    if (m_debugMessenger != VK_NULL_HANDLE)
    {
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
    }
#endif
    /*
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
        */
    vkDestroyInstance(m_instance, nullptr);

    SDL_DestroyWindow(m_window);

    fmt::println("Cleaned up!");
}

void VkEngine::initWindow()
{
    CHECK(SDL_Init(SDL_INIT_VIDEO));
    SDL_WindowFlags windowFlags{SDL_WINDOW_VULKAN};

    m_window = SDL_CreateWindow("Vulkan Window", 640, 480, windowFlags);
    CHECK((m_window != nullptr));
}

void VkEngine::initVulkan()
{
    volkInitialize();

    createInstance();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
}

void VkEngine::createInstance()
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

    // m_enabledInstanceLayers = Util::filterExtensions(e)
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

[[nodiscard]] std::vector<std::string> VkEngine::enumerateInstanceLayers()
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

[[nodiscard]] std::vector<std::string> VkEngine::enumerateInstanceExtensions()
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

void VkEngine::selectPhysicalDevice()
{
    uint32_t physicalDeviceCount{0};
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr));
    CHECK((physicalDeviceCount != 0));
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    VK_CHECK(vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data()));

    std::multimap<int, VkPhysicalDevice> options{};
    for (const VkPhysicalDevice& device : physicalDevices)
    {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        int score{0};

        switch (deviceProperties.deviceType)
        {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            score += 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            score += 900;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            score += 800;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            score += 700;
            break;
        default:
            break;
        }

        score += static_cast<int>(deviceProperties.limits.maxImageDimension2D);
        if (!deviceSuitable(device))
            continue;

        options.insert(std::make_pair(score, device));
        // fmt::println("\t{}", deviceProperties.deviceName);
    }

    CHECK((!options.empty() && options.rbegin()->first > 0))
    m_physicalDevice = options.rbegin()->second;

    CHECK((m_physicalDevice != VK_NULL_HANDLE));

#ifdef _DEBUG
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(m_physicalDevice, &deviceProperties);
    fmt::println("Selected physical device: ");
    fmt::println("\t{}", deviceProperties.deviceName);
#endif
}

[[nodiscard]] bool VkEngine::deviceSuitable(VkPhysicalDevice device) const
{
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (!deviceFeatures.geometryShader) // application requires geometry shaders support
        return false;

    if (!(deviceProperties.apiVersion >= VK_API_VERSION_1_3)) // we need at least 1.3
        return false;

    if (!checkDeviceExtensionsSupport(device))
        return false;

    QueueFamilyIndices indices{findQueueFamilies(device)};
    if (!indices.complete())
        return false;

    return true;
}

[[nodiscard]] bool VkEngine::checkDeviceExtensionsSupport(VkPhysicalDevice device) const
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

[[nodiscard]] QueueFamilyIndices VkEngine::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i{0};
    for (const VkQueueFamilyProperties& queueFamily : queueFamilies)
    {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            indices.graphicsFamily = i;
        }

        // NOTE: Gives seg fault if surface hasn't been created yet
        VkBool32 presentSupport{false};
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
        if (presentSupport)
        {
            indices.presentFamily = i;
        }

        if (indices.complete())
        {
            break;
        }
        ++i;
    }

    return indices;
}

void VkEngine::createLogicalDevice()
{
    QueueFamilyIndices indices{findQueueFamilies(m_physicalDevice)};
    m_queueFamilyIndices = indices;

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
    std::set<uint32_t> uniqueQueueFamilies{indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority{1.0f};
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCI{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority};
    }

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .extendedDynamicState = VK_TRUE};
    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &dynamicStateFeatures,
        .dynamicRendering = VK_TRUE};

    VkPhysicalDeviceVulkan11Features features11{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
        .pNext = &features13,
        .shaderDrawParameters = VK_TRUE};

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features11,
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

void VkEngine::createSurface() { CHECK(SDL_Vulkan_CreateSurface(m_window, m_instance, nullptr, &m_surface)); }

void VkEngine::createSwapchain()
{
    SwapchainSupportDetails details{checkSwapchainSupport(m_physicalDevice)};
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
    // createInfo.compositeAlpha = VK_COMPO
}

[[nodiscard]] SwapchainSupportDetails VkEngine::checkSwapchainSupport(VkPhysicalDevice device) const
{
    SwapchainSupportDetails details{};
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
VkEngine::selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
{
    for (const VkSurfaceFormatKHR& format : formats)
    {
        if (format.format == VK_FORMAT_R8G8B8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    return formats[0];
}

[[nodiscard]] VkPresentModeKHR
VkEngine::selectSwapchainPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
{
    for (const auto& pm : presentModes)
    {
        if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return pm;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

[[nodiscard]] VkExtent2D VkEngine::selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
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

VKAPI_ATTR VkBool32 VKAPI_CALL VkEngine::debugCallback(
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

void VkEngine::setupDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
}
