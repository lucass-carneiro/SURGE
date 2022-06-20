/**
 * log.hpp
 *
 * Handles logging operations
 */

#ifndef SURGE_LOG_HPP
#define SURGE_LOG_HPP

#include "options.hpp"

// clang-format off
#include <fmt/color.h>
#include <fmt/core.h>
#include <gsl/gsl-lite.hpp>
// clang-format on

#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>

namespace surge {

/**
 * Returns a formatted string containing the current (local) date and time
 *
 * This function uses ctime instead of the chrono machinary, but it is
 * thread safe. It uses a local stack allocated buffer to store the
 * date string, which can be adjusted at compile time.
 *
 * @param n The size of the internal buffer used to store the date and time.
 * @return A C++ string containing the current local date and time.
 */
template <std::size_t max_datetimestring_size = 20>
[[nodiscard]] auto get_datetime_string() -> std::string {
  std::time_t t =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::array<char, max_datetimestring_size> mbstr{};
  auto bytes_written = std::strftime(mbstr.data(), max_datetimestring_size,
                                     "%F %T", std::localtime(&t));

  if (bytes_written != 0)
    return std::string(mbstr.data());
  else
    return std::string{"YYYY-MM-DD HH:MM:SS"};
}

/**
 * The type of event to be logged.
 *
 * The type of event determines the string that is output to the screen as
 * well as the color of the log entry. These types serve as tags that are
 * passed to the general log function. Based on type information, another
 * function, specific to the type of the event being logged is called and
 * executed. This dispatch happens at compile time.
 */
enum class log_event {
  warning,         // yellow
  error,           // bold red
  message,         // steel blue
  missing_feature, // bold yellow
  logo,            // bold crimson
  pre_engine_init, // gold
  rendering,       // aquamarine
};

/**
 * Handles logging output to files and stdout.
 */
class log_manager {
public:
  log_manager() = default;

  auto startup(const std::filesystem::path &path) noexcept {
    output_file = fopen(path.c_str(), "w");

    if (output_file == nullptr) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      output_file = stderr;

      log<log_event::warning>("Unable to open {} for outputting. Reason: {}. "
                              "Using stderr for log output",
                              path.c_str(), std::strerror(errno));
    }
  }

  auto startup() noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    output_file = stdout;
  }

  auto shutdown() noexcept {
    if (output_file != nullptr && output_file != stdout &&
        output_file != stderr) {
      fclose(output_file);
      output_file = nullptr;
    }
  }

  /**
   * Log an event.
   *
   * This function logs an event using fmt print syntax. @see vlog()
   *
   * @param e The type of event to log.
   * @param Args The types of the format arguments. This is automatically
   * deduced by the compiler. and should not be specified.
   * @param str A fmt-like format string.
   * @param args The arguments of the fmt-like format string.
   */
  template <log_event e, typename... Args>
  inline void log(fmt::string_view str, Args &&...args) noexcept {
#ifdef SURGE_USE_LOG
    const std::lock_guard lock(log_mutex);
    vlog<e>(str, fmt::make_format_args(args...));
#else
    return;
#endif
  }

private:
  // The file pointer is released explicitly using the shutdown function
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  gsl::owner<FILE *> output_file = nullptr;

  // Protects file reads and writes
  std::mutex log_mutex;

  /**
   * Prints a colored string
   */
  void colored_print(fmt::string_view banner, const fmt::text_style &style,
                     fmt::string_view str, fmt::format_args args) noexcept {
    try {
      // clang-format off
      fmt::print(output_file,
                #ifdef SURGE_USE_LOG_COLOR
                 style,
                #endif
                "{:s} - SURGE {}: ",
                get_datetime_string(),
                banner
      );
      // clang-format on

      fmt::vprint(output_file, str, args);
      fmt::print(output_file, "\n");

    } catch (const std::exception &e) {
      std::cout << "Error while invonking fmt: " << e.what() << std::endl;
    }
  }

  /**
   * Print a log entry to the screen - non variadic version used
   * internally.
   *
   * @tparam e The type of event to log.
   * @param str A string containing fmt format information.
   * @param args The arguments of the format string.
   */
  template <log_event e>
  void vlog(fmt::string_view str, fmt::format_args args) noexcept {
    if constexpr (e == log_event::message) {
      colored_print("message", fg(fmt::color::steel_blue), str, args);

    } else if constexpr (e == log_event::warning) {
      colored_print("warning", fg(fmt::color::yellow), str, args);

    } else if constexpr (e == log_event::missing_feature) {
      colored_print("missing feature", fg(fmt::color::yellow), str, args);

    } else if constexpr (e == log_event::error) {
      colored_print("error", fmt::emphasis::bold | fg(fmt::color::red), str,
                    args);

    } else if constexpr (e == log_event::logo) {

      try {
        // clang-format off
        fmt::vprint(output_file,
                    #ifdef SURGE_USE_LOG_COLOR
                    fmt::emphasis::bold | fg(fmt::color::crimson),
                    #endif
                    str,
                    args
        );
        // clang-format on

        fmt::print(output_file, "\n");

      } catch (const std::exception &ex) {
        std::cout << "Error while invonking fmt: " << ex.what() << std::endl;
      }

    } else if constexpr (e == log_event::pre_engine_init) {
      colored_print("pre engine initialization bin", fg(fmt::color::gold), str,
                    args);
    }
  }
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern log_manager global_stdout_log_manager;

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
extern log_manager global_file_log_manager;

template <log_event e, typename... Args>
inline void log_all(fmt::string_view str, Args &&...args) noexcept {
  global_stdout_log_manager.log<e>(str, args...);
  global_file_log_manager.log<e>(str, args...);
}

} // namespace surge

#endif // SURGE_LOG_HPP
