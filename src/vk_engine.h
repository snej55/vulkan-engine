// Created by Jens Kromdijk 12/09/2026

#ifndef VK_ENGINE_H
#define VK_ENGINE_H

#include <vulkan/vulkan.h>
#include <volk/volk.h>

#include <vma/vk_mem_alloc.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <fmt/base.h>

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

    void initWindow();
    void initVulkan();

    void createInstance();
};

#endif // VK_ENGINE_H
