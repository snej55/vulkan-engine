// Created by Jens Kromdijk 05/04/2026

#ifndef UTIL_H
#define UTIL_H

#include <cstdlib>
#include <cassert>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>

#include <fmt/base.h>
#include <fmt/format.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "engine_types.h"

// color codes for logging
#define BEGIN_ERROR "\033[41m"
#define END_ERROR "\033[m"
#define BEGIN_WARNING "\033[33m"
#define END_WARNING "\033[m"
#define BEGIN_LOG "\033[36m"
#define END_LOG "\033[m"

#define VK_CHECK(func)                                                                                                                                                                                 \
    {                                                                                                                                                                                                  \
        const VkResult result{func};                                                                                                                                                                   \
        if (result != VK_SUCCESS)                                                                                                                                                                      \
        {                                                                                                                                                                                              \
            fmt::print(stderr, "{}{}: Error calling function {} at {}:{}. Result: ({}){}\n", BEGIN_ERROR, "FATAL", #func, __FILE__, __LINE__, (int)result, END_ERROR);                                 \
            const std::string msg{fmt::format("{}{}: Error calling function {} at {}:{}. Result: ({}){}\n", BEGIN_ERROR, "FATAL", #func, __FILE__, __LINE__, (int)result, END_ERROR)};                 \
            const EngineResult error{nullptr, -1, result, nullptr};                                                                                                                                    \
            throw EngineException(error, msg);                                                                                                                                                         \
        }                                                                                                                                                                                              \
    }

#define CHECK(func)                                                                                                                                                                                    \
    {                                                                                                                                                                                                  \
        if (!func)                                                                                                                                                                                     \
        {                                                                                                                                                                                              \
            fmt::print(stderr, "{}Error calling function {} at {}:{}{}\n", BEGIN_ERROR, #func, __FILE__, __LINE__, END_ERROR);                                                                         \
            const std::string msg{fmt::format("{}Error calling function {} at {}:{}{}\n", BEGIN_ERROR, #func, __FILE__, __LINE__, END_ERROR)};                                                         \
            throw EngineException(EngineResult{nullptr, -1, VK_SUCCESS, nullptr}, msg);                                                                                                                \
        }                                                                                                                                                                                              \
    }

#define CRASHOUT()                                                                                                                                                                                     \
    {                                                                                                                                                                                                  \
        fmt::print(stderr, "{}Crashed out at {}:{}{}\n", BEGIN_ERROR, __FILE__, __LINE__, END_ERROR);                                                                                                  \
        throw EngineException(EngineResult{nullptr, -1, VK_SUCCESS, nullptr}, fmt::format("{}Crashed out at {}:{}{}\n", BEGIN_ERROR, __FILE__, __LINE__, END_ERROR));                                  \
    }

namespace Util
{
    inline bool checkSwapchain(VkResult result, bool* updateSwapchain)
    {
        if (result < VK_SUCCESS)
        {
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                *updateSwapchain = true;
                return true;
            }

            fmt::print(stderr, "{}Vulkan call return error ({}){}\n", BEGIN_ERROR, static_cast<int>(result), END_ERROR);
            return false;
        }
        return true;
    }

    inline std::unordered_set<std::string> filterExtensions(std::vector<std::string> availableExtensions, std::vector<std::string> requestedExtensions)
    {
        std::sort(availableExtensions.begin(), availableExtensions.end());
        std::sort(requestedExtensions.begin(), requestedExtensions.end());

        std::vector<std::string> result{};
        std::set_intersection(availableExtensions.begin(), availableExtensions.end(), requestedExtensions.begin(), requestedExtensions.end(), std::back_inserter(result));
        return std::unordered_set<std::string>(result.begin(), result.end());
    }

    inline std::unordered_set<std::string> filterExtensions(std::vector<std::string> availableExtensions, const char** requestedExtensions, const uint32_t requestedExtensionsCount)
    {
        // convert to std::vector<std::string> first
        std::vector<std::string> requestedExtensionsVec(static_cast<std::size_t>(requestedExtensionsCount));
        for (std::size_t i{0}; i < static_cast<std::size_t>(requestedExtensionsCount); ++i)
        {
            requestedExtensionsVec[i] = std::string(requestedExtensions[i]);
        }

        return filterExtensions(availableExtensions, requestedExtensionsVec);
    }
} // namespace Util

#endif // UTIL_H
