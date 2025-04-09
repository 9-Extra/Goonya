#pragma once
#include <memory>

#include <spdlog/spdlog.h>
#include <vector>

namespace Goonya {

class Logger {
public:
    Logger() = delete;

    static void inititalize();
    static void drop();

    static const std::vector<std::shared_ptr<spdlog::sinks::sink>> get_sinks() noexcept { return sinks; }

private:
    static std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;
};

extern std::shared_ptr<spdlog::logger> core_logger;

} // namespace Goonya

#define LOG_TRACE(...) ::Goonya::core_logger->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::Goonya::core_logger->debug(__VA_ARGS__)
#define LOG_INFO(...) ::Goonya::core_logger->info(__VA_ARGS__)
#define LOG_WARN(...) ::Goonya::core_logger->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Goonya::core_logger->error(__VA_ARGS__)