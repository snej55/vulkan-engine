// Created by Jens Kromdijk 15/04/2026

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <array>

#include <vulkan/vulkan.h>

namespace CST
{
    inline constexpr int WIDTH{640};
    inline constexpr int HEIGHT{480};

    inline constexpr std::array<const char*, 1> validationLayers{"VK_LAYER_KHRONOS_validation"};
    inline constexpr std::array<const char*, 1> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
} // namespace CST

#endif // CONSTANTS_H
