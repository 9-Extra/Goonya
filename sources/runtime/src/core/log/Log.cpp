#include "Log.h"
#include "spdlog/common.h"

#include <memory>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Goonya {

std::shared_ptr<spdlog::logger> core_logger;

Logger logger; // main 函数执行前就初始化
std::vector<std::shared_ptr<spdlog::sinks::sink>> Logger::sinks;

Logger::Logger() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    console_sink->set_pattern("%^[%T][%n][%l] %v%$");
    sinks.emplace_back(console_sink);

    spdlog::init_thread_pool(10000, 1);

    core_logger = std::make_shared<spdlog::async_logger>("core", sinks.begin(), sinks.end(), spdlog::thread_pool());
#ifdef DEBUG
    core_logger->set_level(spdlog::level::trace);
#else
    core_logger->set_level(spdlog::level::warn);
#endif
    spdlog::register_logger(core_logger);
};

Logger::~Logger() {
    shutdown();
    core_logger.reset();
}

void Logger::shutdown() noexcept {
    try {
        if (core_logger) {
            core_logger->flush();
        }
        // 销毁线程池：队列里剩下的日志也会写完
        spdlog::shutdown();
    } catch (...) {
        // 退出路径上不能再抛异常
    }
}

} // namespace Goonya