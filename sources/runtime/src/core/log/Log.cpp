#include "Log.h"

#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

namespace Goonya {

Logger::Logger() {};

Logger::~Logger() {
    assert(!core_logger);//需要先手动调用drop释放logger
};

void Logger::inititalize() {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::level_enum::trace);
    console_sink->set_pattern("%^[%T][%n][%l] %v%$");

    spdlog::init_thread_pool(10000, 1);

    core_logger = std::make_shared<spdlog::async_logger>("core", console_sink,
                                                         spdlog::thread_pool());

    core_logger->set_level(spdlog::level::level_enum::trace);

    spdlog::register_logger(core_logger);
};

void Logger::drop(){
    core_logger->flush();
    core_logger.reset();
    spdlog::drop_all(); 
}

//全局logger
Logger logger;
} // namespace Goonya