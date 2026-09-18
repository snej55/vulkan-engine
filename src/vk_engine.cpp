// Created by Jens Kromdijk 12/09/2026

#include "vk_engine.h"
#include "util.h"
#include "constants.h"

#include <vector>
#include <map>
#include <set>

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
    fmt::println("Selecting physical device...");
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

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        score += static_cast<int>(deviceProperties.limits.maxImageDimension2D);
        if (!deviceFeatures.geometryShader) // we need geometry shader support
            continue;

        if (!(deviceProperties.apiVersion >= VK_API_VERSION_1_3)) // we need at least 1.3
            continue;

        options.insert(std::make_pair(score, device));
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

    if (!deviceFeatures.geometryShader)
        return false;

    if (!(deviceProperties.apiVersion >= VK_API_VERSION_1_3))
        return false;

    if (!checkDeviceExtensionsSupport(device))
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
