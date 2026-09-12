// Created by Jens Kromdijk 24/04/2026

#include "engine.h"

#include <cassert>
#include <chrono>
#include <thread>

void Engine::init()
{
    assert(!m_init);
    m_context = std::make_unique<Context>();
    m_context->init();
}

void Engine::run()
{
    SDL_Event e;
    bool quit{false};
    bool minimized{false};
    while (!quit)
    {
        while (SDL_PollEvent(&e) != 0)
        {
            if (e.type == SDL_EVENT_QUIT)
            {
                quit = true;
            }

            if (e.type == SDL_EVENT_WINDOW_MINIMIZED)
            {
                minimized = true;
            }
            if (e.type == SDL_EVENT_WINDOW_RESTORED)
            {
                minimized = false;
            }
        }

        if (minimized)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));

        draw();
    }
}

void Engine::free() { m_context->free(); }

void Engine::draw() { m_context->draw(); }
