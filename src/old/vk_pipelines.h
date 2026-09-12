// Created by Jens Kromdijk 03/05/2026

#ifndef VK_PIPELINE_H
#define VK_PIPELINE_H

#include <volk/volk.h>

namespace VkUtil
{
    [[nodiscard]] bool loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule);
}

#endif
