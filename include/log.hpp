/**
 * log.hpp
 *
 * Handles logging operations
 */

#ifndef SURGE_LOG_HPP
#define SURGE_LOG_HPP

#include "options.hpp"

#define SPDLOG_LEVEL_NAMES                                                                         \
  { "MY TRACE", "MY DEBUG", "SURGE Info", "SURGE Warning", "SURGE Error", "MY CRITICAL", "OFF" }
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#ifdef SURGE_USE_LOG_COLOR
#  include "spdlog/sinks/stdout_color_sinks.h"
#else
#  include "spdlog/sinks/stdout_sinks.h"
#endif

namespace surge {

class global_stdout_log_manager {
public:
  static auto get() -> global_stdout_log_manager & {
    static global_stdout_log_manager log;
    return log;
  }

  [[nodiscard]] auto get_logger() -> std::shared_ptr<spdlog::logger> & { return logger; }

  global_stdout_log_manager(const global_stdout_log_manager &) = delete;
  global_stdout_log_manager(global_stdout_log_manager &&) = delete;

  auto operator=(global_stdout_log_manager) -> global_stdout_log_manager & = delete;

  auto operator=(const global_stdout_log_manager &) -> global_stdout_log_manager & = delete;

  auto operator=(global_stdout_log_manager &&) -> global_stdout_log_manager & = delete;

  ~global_stdout_log_manager() = default;

private:
  std::shared_ptr<spdlog::logger> logger;

#ifdef SURGE_USE_LOG_COLOR
  global_stdout_log_manager() : logger{spdlog::stdout_color_mt("surge_stdout_logger")} {
    logger->set_pattern("\033[38;2;70;130;180m[%m-%d-%Y %H:%M:%S] "
                        "\033[38;2;127;255;212m[thread %t] "
                        "\033[1m%^%l:%$ "
                        "\033[0m%v");
  }
#else
  global_stdout_log_manager() : logger{spdlog::stdout_logger_mt("surge_stdout_logger")} {
    logger->set_pattern("[%m-%d-%Y %H:%M:%S] [thread %t] %^%l:%$ %v");
  }
#endif
};

#ifdef SURGE_SYSTEM_Windows

template <typename... Args> void log_info(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  global_stdout_log_manager::get().get_logger()->log(spdlog::level::info, fmt,
                                                     std::forward<Args>(args)...);
}

template <typename... Args> void log_error(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  global_stdout_log_manager::get().get_logger()->log(spdlog::level::err, fmt,
                                                     std::forward<Args>(args)...);
}

template <typename... Args> void log_warn(spdlog::wformat_string_t<Args...> fmt, Args &&...args) {
  global_stdout_log_manager::get().get_logger()->log(spdlog::level::warn, fmt,
                                                     std::forward<Args>(args)...);
}

#endif

template <typename... Args> void log_info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  global_stdout_log_manager::get().get_logger()->log(spdlog::level::info, fmt,
                                                     std::forward<Args>(args)...);
}

template <typename... Args> void log_error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  global_stdout_log_manager::get().get_logger()->log(spdlog::level::err, fmt,
                                                     std::forward<Args>(args)...);
}

template <typename... Args> void log_warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
  global_stdout_log_manager::get().get_logger()->log(spdlog::level::warn, fmt,
                                                     std::forward<Args>(args)...);
}

} // namespace surge

#endif // SURGE_LOG_HPP
