#define _CRT_SECURE_NO_WARNINGS
#include "pch.h"
#include "Logger.h"

std::shared_ptr<spdlog::logger> Logger::logger = nullptr;
std::mutex Logger::mutex; // 클래스 내부 정적 멤버로 이동

void Logger::init(const std::string& moduleName, const std::string& logPath) {
    std::lock_guard<std::mutex> lock(mutex);

    if (logger) {
        OutputDebugStringA("Logger is already initialized!\n");
        return;
    }

    try {
        auto msvc_sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
        msvc_sink->set_pattern("[%Y-%m-%d %H:%M:%S][%l]%v");

        auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            fmt::format("{}\\{}.log", logPath, moduleName), 0, 0);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S][%l]%v");

        logger = std::make_shared<spdlog::logger>(moduleName, spdlog::sinks_init_list{ msvc_sink, file_sink });
        logger->set_level(spdlog::level::info); // 기본 로그 레벨 설정
        logger->flush_on(spdlog::level::warn);
    }
    catch (const std::exception& e) {
        OutputDebugStringA(fmt::format("Logger initialization failed: {}\n", e.what()).c_str());
    }
}

void Logger::setLevel(spdlog::level::level_enum level) {
    std::lock_guard<std::mutex> lock(mutex);
    if (logger) {
        logger->set_level(level);
    }
    else {
        OutputDebugStringA("[Logger::setLevel] Logger not initialized!\n");
    }
}

bool Logger::write(spdlog::level::level_enum level, const std::string& log, const std::string& func, int line) {

    if (!logger) {
        OutputDebugStringA("[Logger::write] Logger not initialized!\n");
        return false;
    }

    logger->log(level, "[func:{}][line:{}] {}", func, line, log);

    if (level == spdlog::level::err) {
        logger->flush();
    }

    return true;
}

bool Logger::write(spdlog::level::level_enum level, const char* log, const std::string& func, int line) {
    return write(level, std::string(log), func, line);
}

/*
std::shared_ptr<spdlog::logger> Logger::get() {
  std::lock_guard<std::mutex> lock(mutex);
  if (!logger) {
    OutputDebugStringA("[Logger::get]Logger not initialized!\n");
    return nullptr;
  }
  return logger;
}
*/
