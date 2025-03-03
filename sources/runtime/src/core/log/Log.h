#pragma once
#include <memory>

#include <spdlog/spdlog.h>

namespace Goonya {

class Logger {
public:
    Logger();
    ~Logger();

    void inititalize();
    void drop();

    std::shared_ptr<spdlog::logger> core_logger;
};

extern Logger logger;

} // namespace Goonya

#define LOG_TRACE(...) ::Goonya::logger.core_logger->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::Goonya::logger.core_logger->debug(__VA_ARGS__)
#define LOG_INFO(...) ::Goonya::logger.core_logger->info(__VA_ARGS__)
#define LOG_WARN(...) ::Goonya::logger.core_logger->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Goonya::logger.core_logger->error(__VA_ARGS__)