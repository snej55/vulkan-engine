// Created by Jens Kromdijk 02/05/2026

#include "vk_context.h"
#include "util.h"

// ---- DescriptorLayoutBuilder ---- //
void DescriptorLayoutBuilder::addBinding(const uint32_t binding, VkDescriptorType type)
{
    VkDescriptorSetLayoutBinding newbind{};
    newbind.binding = binding;
    newbind.descriptorCount = 1;
    newbind.descriptorType = type;

    m_bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() { m_bindings.clear(); }

VkDescriptorSetLayout DescriptorLayoutBuilder::build(
    VkDevice device,
    VkShaderStageFlags shaderStages,
    void* pNext,
    VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : m_bindings)
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    info.pNext = pNext;

    info.pBindings = m_bindings.data();
    info.bindingCount = static_cast<uint32_t>(m_bindings.size());
    info.flags = flags;

    VkDescriptorSetLayout set;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));
    return set;
}

// ---- DesriptorAllocator ---- //
void DescriptorAllocator::initPool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(
            VkDescriptorPoolSize{.type = ratio.type, .descriptorCount = static_cast<uint32_t>(ratio.ratio * maxSets)});
    }

    VkDescriptorPoolCreateInfo poolCI{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolCI.flags = 0;
    poolCI.maxSets = maxSets;
    poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCI.pPoolSizes = poolSizes.data();

    vkCreateDescriptorPool(device, &poolCI, nullptr, &m_pool);
}

void DescriptorAllocator::clearDescriptors(VkDevice device) { vkResetDescriptorPool(device, m_pool, 0); }

void DescriptorAllocator::destroyPool(VkDevice device) { vkDestroyDescriptorPool(device, m_pool, nullptr); }

VkDescriptorSet DescriptorAllocator::allocate(VkDevice device, VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.pNext = nullptr;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet ds;
    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds));
    return ds;
}
