// Created by Jens Kromdijk 05/04/2026

#include <fmt/base.h>

#include <cstdlib>
#include <exception>
#include <memory>

#define _DEBUG
#include "src/vk_engine.h"

class App
{
public:
    App() = default;

    void run()
    {
        init();
        mainLoop();
        free();
    }

private:
    // initialize vulkan context and other stuff
    void init()
    {
        m_engine = std::make_unique<VkEngine>(VkEngine{});
        m_engine->init();
    }

    // main rendering loop
    void mainLoop() {}

    // free resources
    void free() { m_engine->free(); }

    std::unique_ptr<VkEngine> m_engine{nullptr};
};

int main()
{
    fmt::println("Running...");

    try
    {
        App app{};
        app.run();
    }
    catch (const std::exception& e)
    {
        fmt::println("{}", e.what());
        return EXIT_FAILURE;
    }

    fmt::println("We ran!");

    return EXIT_SUCCESS;
}
