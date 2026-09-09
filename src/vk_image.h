// Created by Jens Kromdijk 30/04/2026
//
#ifndef VK_IMAGE_H
#define VK_IMAGE_H

#include <vulkan/vulkan.h>

namespace VkUtil
{
    void transitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

    void copyImage2Image(VkCommandBuffer cmd, VkImage source, VkImage dest, VkExtent2D srcSize, VkExtent2D dstSize);
} // namespace VkUtil

#endif // VK_IMAGE_H
