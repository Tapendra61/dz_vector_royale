#pragma once

#include <spdlog/spdlog.h>

#include <string>

namespace vector::core {

void init_logging(const std::string& app_name = "vector",
                  spdlog::level::level_enum level = spdlog::level::info);

void shutdown_logging();

}  // namespace vector::core

#define VECTOR_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define VECTOR_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define VECTOR_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define VECTOR_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define VECTOR_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define VECTOR_CRIT(...)  SPDLOG_CRITICAL(__VA_ARGS__)
