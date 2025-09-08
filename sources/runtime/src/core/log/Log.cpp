#include "Log.h"

#include <memory>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Goonya {

std::shared_ptr<spdlog::logger> core_logger; 

static Logger logger; // main 函数执行前就初始化
std::vector<std::shared_ptr<spdlog::sinks::sink>> Logger::sinks;

Logger::Logger() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    console_sink->set_pattern("%^[%T][%n][%l] %v%$");
    sinks.emplace_back(console_sink);

    spdlog::init_thread_pool(10000, 1);

    core_logger = std::make_shared<spdlog::async_logger>("core", sinks.begin(), sinks.end(), spdlog::thread_pool());

    core_logger->set_level(spdlog::level::trace);

    spdlog::register_logger(core_logger);
};

Logger::~Logger() {
    core_logger.reset();
    spdlog::drop_all();
}

} // namespace Goonya