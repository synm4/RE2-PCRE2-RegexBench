#pragma once

#include <spdlog/spdlog.h>
#include "pch.h"
#include <spdlog/sinks/msvc_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger {
public:
    static void init(const std::string& moduleName, const std::string& logPath);
    static void setLevel(spdlog::level::level_enum level);
    static bool write(spdlog::level::level_enum level, const std::string& log, const std::string& func, int line);
    static bool write(spdlog::level::level_enum level, const char* log, const std::string& func, int line);

private:
    static std::shared_ptr<spdlog::logger> logger;
    static std::mutex mutex; // 클래스 내부 정적 멤버로 이동
};

