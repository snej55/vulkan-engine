// Created by Jens Kromdijk 28/04/2026

#include "vk_init.h"

void VkUtil::commandPoolCreateInfo(VkCommandPoolCreateInfo* commandCI, uint32_t queueFamilyIndex)
{
    commandCI->sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandCI->pNext = nullptr;
    commandCI->flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandCI->queueFamilyIndex = queueFamilyIndex;
}

void VkUtil::commandBufferAllocInfo(VkCommandBufferAllocateInfo* allocInfo, VkCommandPool commandPool, uint32_t count)
{
    allocInfo->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo->pNext = nullptr;
    allocInfo->commandPool = commandPool;
    allocInfo->commandBufferCount = count;
    allocInfo->level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
}

void VkUtil::fenceCreateInfo(VkFenceCreateInfo* fenceCI, VkFenceCreateFlags flags)
{
    fenceCI->sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCI->pNext = nullptr;
    fenceCI->flags = flags;
}

void VkUtil::semaphoreCreateInfo(VkSemaphoreCreateInfo* semaphoreCI, VkSemaphoreCreateFlags flags)
{
    semaphoreCI->sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreCI->pNext = nullptr;
    semaphoreCI->flags = flags;
}

void VkUtil::commandBufferBeginInfo(VkCommandBufferBeginInfo* cmdBufferCI, VkCommandBufferUsageFlags flags)
{
    cmdBufferCI->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdBufferCI->pNext = nullptr;

    cmdBufferCI->pInheritanceInfo = nullptr;
    cmdBufferCI->flags = flags;
}

void VkUtil::semaphoreSubmitInfo(VkSemaphoreSubmitInfo* info, VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
{
    info->sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    info->pNext = nullptr;
    info->semaphore = semaphore;
    info->stageMask = stageMask;
    info->deviceIndex = 0;
    info->value = 1;
}

void VkUtil::commandBufferSubmitInfo(VkCommandBufferSubmitInfo* info, VkCommandBuffer cmd)
{
    info->sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info->pNext = nullptr;
    info->commandBuffer = cmd;
    info->deviceMask = 0;
}

void VkUtil::submitInfo(
    VkSubmitInfo2* info,
    VkCommandBufferSubmitInfo* cmd,
    VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    info->sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info->pNext = nullptr;

    info->waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info->pWaitSemaphoreInfos = waitSemaphoreInfo;

    info->signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info->pSignalSemaphoreInfos = signalSemaphoreInfo;

    info->commandBufferInfoCount = 1;
    info->pCommandBufferInfos = cmd;
}

VkImageSubresourceRange VkUtil::imageSubresourceRange(VkImageAspectFlags aspectMask)
{
    VkImageSubresourceRange subImage{};
    subImage.aspectMask = aspectMask;
    subImage.baseMipLevel = 0;
    subImage.levelCount = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
}

void VkUtil::imageCreateInfo(VkImageCreateInfo* info, VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent)
{
    info->sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info->pNext = nullptr;
    info->imageType = VK_IMAGE_TYPE_2D;
    info->format = format;
    info->extent = extent;
    info->mipLevels = 1;
    info->arrayLayers = 1;
    info->samples = VK_SAMPLE_COUNT_1_BIT;
    info->tiling = VK_IMAGE_TILING_OPTIMAL;
    info->usage = usageFlags;
}

void VkUtil::imageViewCreateInfo(VkImageViewCreateInfo* info, VkFormat format, VkImage image, VkImageAspectFlags flags)
{
    info->sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info->pNext = nullptr;
    info->viewType = VK_IMAGE_VIEW_TYPE_2D;
    info->image = image;
    info->format = format;
    info->subresourceRange.baseMipLevel = 0;
    info->subresourceRange.levelCount = 1;
    info->subresourceRange.baseArrayLayer = 0;
    info->subresourceRange.layerCount = 1;
    info->subresourceRange.aspectMask = flags;
}
