#ifndef SURGE_LOGGING_SYSTEM_H
#define SURGE_LOGGING_SYSTEM_H

#define SPDLOG_LEVEL_NAMES                                                                         \
  {                                                                                                \
    "Surge Trace", "SURGE Debug", "SURGE Info", "SURGE Warning", "SURGE Error", "Surge Critical",  \
        "OFF"                                                                                      \
  }
#include <spdlog/spdlog.h>

namespace surge {

namespace logger {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern std::shared_ptr<spdlog::logger> logger_ptr;

auto init() noexcept -> bool;

} // namespace logger

template <typename... Args> void log_info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::info, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::err, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_debug(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
}

#ifdef SURGE_SYSTEM_Windows

template <typename... Args> void log_info(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::info, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_error(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::err, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_warn(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_debug(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  logger::logger_ptr->log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
}

#endif

} // namespace surge

#endif // SURGE_LOGGING_SYSTEM_H