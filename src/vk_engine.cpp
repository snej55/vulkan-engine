// Created by Jens Kromdijk 12/09/2026

#include "vk_engine.h"
#include "util.h"

void VkEngine::init()
{
    initWindow();
    initVulkan();
}

void VkEngine::initWindow()
{
    CHECK(SDL_Init(SDL_INIT_VIDEO));
    SDL_WindowFlags windowFlags{SDL_WINDOW_VULKAN};

    m_window = SDL_CreateWindow("Vulkan Window", 640, 480, windowFlags);
    CHECK((m_window != nullptr));
}

void VkEngine::initVulkan()
{
    volkInitialize();

    createInstance();
}

void VkEngine::createInstance() {}
