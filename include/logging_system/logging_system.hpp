#ifndef SURGE_LOGGING_SYSTEM_H
#define SURGE_LOGGING_SYSTEM_H

#include "options.hpp"

#define SPDLOG_LEVEL_NAMES                                                                         \
  { "MY TRACE", "SURGE Debug", "SURGE Info", "SURGE Warning", "SURGE Error", "MY CRITICAL", "OFF" }
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#ifdef SURGE_USE_LOG_COLOR
#  include <spdlog/sinks/stdout_color_sinks.h>
#else
#  include "spdlog/sinks/stdout_sinks.h"
#endif

namespace surge {

class log_manager {
public:
  inline static auto get() -> log_manager & {
    static log_manager log;
    return log;
  }

  [[nodiscard]] inline auto get_logger() noexcept -> std::shared_ptr<spdlog::logger> & { return logger; }

  log_manager(const log_manager &) = delete;
  log_manager(log_manager &&) = delete;

  auto operator=(log_manager) -> log_manager & = delete;
  auto operator=(const log_manager &) -> log_manager & = delete;
  auto operator=(log_manager &&) -> log_manager & = delete;

  ~log_manager() = default;

private:
  std::shared_ptr<spdlog::logger> logger;

#ifdef SURGE_USE_LOG_COLOR
  inline log_manager() : logger{spdlog::stdout_color_mt("surge_stdout_logger")} {
    logger->set_pattern("\033[38;2;70;130;180m[%m-%d-%Y %H:%M:%S] "
                        "\033[38;2;127;255;212m[thread %t] "
                        "\033[1m%^%l:%$ "
                        "\033[0m%v");
  }

#else

  inline log_manager() : logger{spdlog::stdout_logger_mt("surge_stdout_logger")} {
    logger->set_pattern("[%m-%d-%Y %H:%M:%S] [thread %t] %^%l:%$ %v");
  }

#endif

};

#ifdef SURGE_SYSTEM_Windows

template <typename... Args> void log_info(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::info, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_error(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::err, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_warn(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_debug(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
}

#endif

template <typename... Args> void log_info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::info, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::err, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::warn, fmt, std::forward<Args>(args)...);
}

template <typename... Args> void log_debug(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  log_manager::get().get_logger()->log(spdlog::level::debug, fmt, std::forward<Args>(args)...);
}

} // namespace surge

#endif // SURGE_LOGGING_SYSTEM_H