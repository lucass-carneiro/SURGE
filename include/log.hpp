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
template <std::size_t max_datetimestring_size = 20> [[nodiscard]] auto get_datetime_string()
    -> std::string {
  std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::array<char, max_datetimestring_size> mbstr{};
  auto bytes_written
      = std::strftime(mbstr.data(), max_datetimestring_size, "%F %T", std::localtime(&t));

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
 * Closes a file if it is not stdout or stderror. Inteded to be used in the unique ptr managing the
 * log's underlying file.
 */
inline void file_closer(std::FILE *f) {
  if (f != stdout || f != stderr) {
    std::fclose(f); // NOLINT(cppcoreguidelines-owning-memory)
    f = nullptr;
  }
}

/**
 * Handles logging output to files and stdout.
 */
class logger {
public:
  /**
   * Constructs a logger object that outputs to a file given by a path.
   *
   * If the file indicated by the path cannot be opened, the logger outputs data to the stderr
   * stream. This is to not pollute the stdout output in case it is also being used. If a regular
   * file is being used, it is closed when the logger destructor is called, otherwise if the output
   * is set to stderr no close operation is performed.
   *
   * @param path The path to the log file
   */
  logger(const std::filesystem::path &path) : output_file(fopen(path.c_str(), "w"), file_closer) {
    if (output_file == nullptr) {

      output_file.release(); // NOLINT(bugprone-unused-return-value)
      output_file.reset(stderr);

      log<log_event::warning>(
          "Unable to open {} for outputting. Reason: {}. Using stderr for log output", path.c_str(),
          std::strerror(errno));
    }
  }

  /**
   * Constructs a logger object that outputs to stdout
   *
   * When the destructor is called, stdout is not closed.
   *
   */
  logger() : output_file(stdout, file_closer) {}

  /**
   * Log an event.
   *
   * This function logs an event using fmt print syntax. @see vlog()
   *
   * @param e The type of event to log.
   * @param Args The types of the format arguments. This is automatically deduced by the compiler.
   * and should not be specified.
   * @param str A fmt-like format string.
   * @param args The arguments of the fmt-like format string.
   */
  template <log_event e, typename... Args> inline void log(fmt::string_view str, Args &&...args) {
#ifdef SURGE_USE_LOG
    const std::lock_guard lock(log_mutex);
    vlog<e>(str, fmt::make_format_args(args...));
#else
    return;
#endif
  }

private:
  std::unique_ptr<std::FILE, void (*)(std::FILE *)> output_file;
  std::mutex log_mutex;

  void colored_print(fmt::string_view banner, const fmt::text_style &style, fmt::string_view str,
                     fmt::format_args args) {
    fmt::print(output_file.get(),
#ifdef SURGE_USE_LOG_COLOR
               style,
#endif
               "{:s} - SURGE {}: ", get_datetime_string(), banner);
    // clang-format on

    fmt::vprint(output_file.get(), str, args);
    fmt::print(output_file.get(), "\n");
  }

  /**
   * Print a log entry to the screen - non variadic version used
   * internally.
   *
   * @tparam e The type of event to log.
   * @param str A string containing fmt format information.
   * @param args The arguments of the format string.
   */
  template <log_event e> void vlog(fmt::string_view str, fmt::format_args args) {
    if constexpr (e == log_event::message) {
      colored_print("message", fg(fmt::color::steel_blue), str, args);

    } else if constexpr (e == log_event::warning) {
      colored_print("warning", fg(fmt::color::yellow), str, args);

    } else if constexpr (e == log_event::missing_feature) {
      colored_print("missing feature", fg(fmt::color::yellow), str, args);

    } else if constexpr (e == log_event::error) {
      colored_print("error", fmt::emphasis::bold | fg(fmt::color::red), str, args);

    } else if constexpr (e == log_event::logo) {
      fmt::vprint(output_file.get(),
#ifdef SURGE_USE_LOG_COLOR
                  fmt::emphasis::bold | fg(fmt::color::crimson),
#endif
                  str, args);

      fmt::print(output_file.get(), "\n");

    } else if constexpr (e == log_event::pre_engine_init) {
      colored_print("pre engine initialization bin", fg(fmt::color::gold), str, args);
    }
  }
};

class global_stdout_log {
public:
  static auto instance() -> logger & {
    static logger l;
    return l;
  }

  global_stdout_log(const global_stdout_log &) = delete;
  global_stdout_log(global_stdout_log &&) = delete;
  auto operator=(global_stdout_log &) -> global_stdout_log & = delete;
  auto operator=(global_stdout_log &&) -> global_stdout_log & = delete;

private:
  global_stdout_log() = default;
  ~global_stdout_log() = default;
};

class global_file_log {
public:
  static auto instance() -> logger & {
    static logger l("log.txt");
    return l;
  }

  global_file_log(const global_file_log &) = delete;
  global_file_log(global_file_log &&) = delete;
  auto operator=(global_file_log &) -> global_file_log & = delete;
  auto operator=(global_file_log &&) -> global_file_log & = delete;

private:
  global_file_log() = default;
  ~global_file_log() = default;
};

template <log_event e, typename... Args>
inline void log_all(fmt::string_view str, Args &&...args) noexcept {
  try {
    global_stdout_log::instance().log<e>(str, args...);
    global_file_log::instance().log<e>(str, args...);
  } catch (const std::exception &exc) {
    std::cout << "Exception ocurred while attempting to log information: " << exc.what()
              << std::endl;
  }
}

} // namespace surge

#endif // SURGE_LOG_HPP
