#include "core/logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>

namespace vector::core {

void init_logging(const std::string& app_name, spdlog::level::level_enum level) {
    if (spdlog::get(app_name)) {
        return;
    }
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto logger = std::make_shared<spdlog::logger>(app_name, sink);
    logger->set_level(level);
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] [%n] %v");
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(1));
}

void shutdown_logging() {
    spdlog::shutdown();
}

}  // namespace vector::core
