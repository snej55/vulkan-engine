// Created by Jens Kromdijk 28/04/2026

#ifndef VK_INIT_H
#define VK_INIT_H

#include <vulkan/vulkan.h>

namespace VkUtil
{
    void commandPoolCreateInfo(VkCommandPoolCreateInfo* commandCI, uint32_t queueFamilyIndex);

    void commandBufferAllocInfo(VkCommandBufferAllocateInfo* allocInfo, VkCommandPool commandPool, uint32_t count);

    void fenceCreateInfo(VkFenceCreateInfo* fenceCI, VkFenceCreateFlags flags);

    void semaphoreCreateInfo(VkSemaphoreCreateInfo* semaphoreCI, VkSemaphoreCreateFlags flags);

    void commandBufferBeginInfo(VkCommandBufferBeginInfo* cmdBufferCI, VkCommandBufferUsageFlags flags);

    void semaphoreSubmitInfo(VkSemaphoreSubmitInfo* info, VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);

    void commandBufferSubmitInfo(VkCommandBufferSubmitInfo* info, VkCommandBuffer cmd);

    void submitInfo(VkSubmitInfo2* info, VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);

    VkImageSubresourceRange imageSubresourceRange(VkImageAspectFlags aspectMask);

    void imageCreateInfo(VkImageCreateInfo* info, VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

    void imageViewCreateInfo(VkImageViewCreateInfo* info, VkFormat format, VkImage image, VkImageAspectFlags flags);
}; // namespace VkUtil

#endif // VK_INIT_H
