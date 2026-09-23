// Created by Jens Kromdijk 12/09/2026

#ifndef VK_ENGINE_H
#define VK_ENGINE_H

#define _DEBUG

#include <vulkan/vulkan.h>
#include <volk/volk.h>

#include <vma/vk_mem_alloc.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <fmt/base.h>

#include <unordered_set>
#include <string>
#include <vector>
#include <optional>

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    // std::optional<uint32_t> presentFamily;

    bool complete() const { return graphicsFamily.has_value(); } //  && presentFamily.has_value(); }
};

class VkEngine
{
public:
    VkEngine() = default;
    ~VkEngine() = default;

    void init();
    void run();
    void free();

private:
    SDL_Window* m_window{nullptr};

    // --------- Vulkan components --------- //
    std::unordered_set<std::string> m_enabledInstanceLayers{};
    std::unordered_set<std::string> m_enabledInstanceExtensions{};

    VkDebugUtilsMessengerEXT m_debugMessenger{VK_NULL_HANDLE};

    VkInstance m_instance{VK_NULL_HANDLE};
    VkPhysicalDevice m_physicalDevice{VK_NULL_HANDLE};

    void initWindow();
    void initVulkan();

    void createInstance();
    void selectPhysicalDevice();
    [[nodiscard]] bool deviceSuitable(VkPhysicalDevice device) const;
    [[nodiscard]] bool checkDeviceExtensionsSupport(VkPhysicalDevice device) const;
    [[nodiscard]] QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    [[nodiscard]] std::vector<std::string> enumerateInstanceLayers();
    [[nodiscard]] std::vector<std::string> enumerateInstanceExtensions();

    // ----------- validation layers ----------- //
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    void setupDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
};

#endif // VK_ENGINE_H
