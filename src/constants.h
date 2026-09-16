// Created by Jens Kromdijk 15/09/2026

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <array>

#include <vulkan/vulkan.h>

namespace CST
{
    inline constexpr std::array<const char*, 1> validationLayers{"VK_LAYER_KHRONOS_validation"};
    inline constexpr std::array<const char*, 1> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
} // namespace CST

#endif
