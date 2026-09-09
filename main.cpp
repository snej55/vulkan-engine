// Created by Jens Kromdijk 05/04/2026

#include <fmt/base.h>

#include <cstdlib>

#define _DEBUG
#include "src/engine.h"

int main()
{
    Engine engine{};
    fmt::println("Running...");

    try
    {
        engine.init();
        engine.run();
        engine.free();
    }
    catch (std::exception& e)
    {
        fmt::println("{}", e.what());
        return EXIT_FAILURE;
    }

    fmt::println("We ran!");

    return EXIT_SUCCESS;
}

