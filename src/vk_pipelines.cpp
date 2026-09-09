// Created by Jens Kromdijk 03/05/2026

#include "vk_pipelines.h"

#include <fstream>
#include <vector>

bool VkUtil::loadShaderModule(const char* filePath, VkDevice device, VkShaderModule* outShaderModule)
{
    // std::ios::ate starts seek at end of file
    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    // tellg gets position of seek, so gets file size
    std::size_t fileSize{static_cast<std::size_t>(file.tellg())};

    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put cursor back at start and read file into buffer
    file.seekg(0);
    file.read((char*)buffer.data(), fileSize);
    file.close();

    // create the shader module
    VkShaderModuleCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    createInfo.pNext = nullptr;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}
