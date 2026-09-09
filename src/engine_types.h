// Created by Jens Kromdijk 26/04/2026

#ifndef ENGINE_TYPES_H
#define ENGINE_TYPES_H

#include <stdexcept>
#include <deque>
#include <functional>

#include <vulkan/vulkan.h>

struct DeletionQueue
{
    std::deque<std::function<void()>> m_deletors;

    void push_function(std::function<void()>&& function)
    {
        m_deletors.push_back(function);
    }

    void flush()
    {
        for (auto it {m_deletors.begin()}; it != m_deletors.end(); ++it)
        {
            (*it)();
        }
        m_deletors.clear();
    }
};

struct EngineResult
{
    void* m_data;
    int m_code;
    VkResult m_vkResult;
    const char* m_msg;
};

class EngineException : public std::runtime_error
{
public:
    EngineResult m_result;

    EngineException(EngineResult result, const std::string& msg) : std::runtime_error(msg), m_result(result) {}
};

#endif // ENGINE_TYPES_H
