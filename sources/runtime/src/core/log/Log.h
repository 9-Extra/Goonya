#pragma once
#include <memory>

#include <spdlog/spdlog.h>
#include <vector>

namespace Goonya {

class Logger final {
private:
    static std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;
public:
    Logger(); // 主要是初始化core_logger
    ~Logger();

    static const std::vector<std::shared_ptr<spdlog::sinks::sink>>& get_sinks() noexcept { return sinks; }
};

extern std::shared_ptr<spdlog::logger> core_logger; // 其实应该放在Logger里面，但这样看起来不错

} // namespace Goonya

#define LOG_TRACE(...) ::Goonya::core_logger->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::Goonya::core_logger->debug(__VA_ARGS__)
#define LOG_INFO(...) ::Goonya::core_logger->info(__VA_ARGS__)
#define LOG_WARN(...) ::Goonya::core_logger->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Goonya::core_logger->error(__VA_ARGS__)