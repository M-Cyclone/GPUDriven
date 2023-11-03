#pragma once
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

class Log
{
public:
    static void init();

    static spdlog::logger& get() { return *s_core_logger; }

private:
    static std::shared_ptr<spdlog::logger> s_core_logger;
};

// Core log macros
#define LogTrace(...)    ::Log::get().trace(__VA_ARGS__)
#define LogInfo(...)     ::Log::get().info(__VA_ARGS__)
#define LogWarn(...)     ::Log::get().warn(__VA_ARGS__)
#define LogError(...)    ::Log::get().error(__VA_ARGS__)
#define LogCritical(...) ::Log::get().critical(__VA_ARGS__)
