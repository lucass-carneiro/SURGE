/**
 * log.hpp
 *
 * Handles logging operations
 */

#ifndef SURGE_LOG_HPP
#define SURGE_LOG_HPP

#include "options.hpp"
#include "static_map.hpp"

// clang-format off
#include <bits/types/FILE.h>
#include <fmt/color.h>
#include <fmt/core.h>
// clang-format on

#include <array>
#include <chrono>
#include <cstdint>
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
 * The type of event to be logged.
 *
 * These types serve as tags that are passed to the general log function.
 */
enum class log_event : std::uint8_t {
  logo,    // bold crimson
  warning, // yellow
  error,   // bold red
  message, // steel blue
  memory,  // aquamarine
  count
};

using log_color_map_t = static_map<log_event, fmt::text_style,
                                   static_cast<std::size_t>(log_event::count)>;

using log_banner_map_t =
    static_map<log_event, const char *,
               static_cast<std::size_t>(log_event::count) - 1>;

/**
 * Static map of event-color assossiations
 */
constexpr const log_color_map_t log_color_map{
    {{{log_event::logo, fmt::emphasis::bold | fg(fmt::color::crimson)},
      {log_event::warning, fmt::fg(fmt::color::yellow)},
      {log_event::error, fmt::emphasis::bold | fg(fmt::color::red)},
      {log_event::message, fg(fmt::color::steel_blue)},
      {log_event::memory, fg(fmt::color::aquamarine)}}}};

/**
 * Static map of event-baner assossiations
 */
constexpr const log_banner_map_t log_banner_map{
    {{{log_event::warning, "warning"},
      {log_event::error, "error"},
      {log_event::message, "message"},
      {log_event::memory, "memory event"}}}};

/**
 * Handles logging output to files and stdout.
 */
class log_manager {
public:
  log_manager(const std::filesystem::path &path)
      : output_file(open_file(path), std::fclose) {}

  log_manager() : output_file(stdout, [](std::FILE *) -> int { return 0; }) {}

#ifdef SURGE_USE_LOG
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
    const std::lock_guard lock(log_mutex);
    vlog<e>(str, fmt::make_format_args(args...));
  }
#else
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
  inline void log(fmt::string_view, Args &&...) noexcept {
    return;
  }
#endif

private:
  // The file pointer is released explicitly using the shutdown function
  // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
  std::unique_ptr<std::FILE, int (*)(std::FILE *)> output_file;

  // Protects file reads and writes
  std::mutex log_mutex;

  /**
   * Invokes fopen and returns either a pointer to the oppened file or stderr
   * if fopen files
   *
   * @param path Path to the file to open
   * @return Pointer to oppened file or stderr
   */
  auto open_file(const std::filesystem::path &path) noexcept -> std::FILE * {
    // This pointer owns the file only temporarilly, and after that it returns
    // itself to a unique pointer, thus relinquinshing it's ownership over the
    // file
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    std::FILE *file = fopen(path.c_str(), "w");

    if (file == nullptr) {
      // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
      file = stderr;

      log<log_event::warning>("Unable to open {} for outputting. Reason: {}. "
                              "Using stderr for log output",
                              path.c_str(), std::strerror(errno));
    }
    return file;
  }

  /**
   * Prints a colored string
   */
  void colored_print(fmt::string_view banner, const fmt::text_style &style,
                     fmt::string_view str, fmt::format_args args) noexcept {
    try {
      // clang-format off
      fmt::print(output_file.get(),
                #ifdef SURGE_USE_LOG_COLOR
                 style,
                #endif
                "SURGE {}: ",
                banner
      );
      // clang-format on

      fmt::vprint(output_file.get(), str, args);
      fmt::print(output_file.get(), "\n");

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
    if constexpr (e == log_event::logo) {
      try {
        // clang-format off
        fmt::vprint(output_file.get(),
                    #ifdef SURGE_USE_LOG_COLOR
                    log_color_map[e],
                    #endif
                    str,
                    args
        );
        // clang-format on

        fmt::print(output_file.get(), "\n");

      } catch (const std::exception &ex) {
        std::cout << "Error while invonking fmt: " << ex.what() << std::endl;
      }

    } else {
      colored_print(log_banner_map[e], log_color_map[e], str, args);
    }
  }
};

class global_stdout_log_manager {
public:
  static auto get() noexcept -> log_manager & {
    static log_manager log;
    return log;
  }

  global_stdout_log_manager(const global_stdout_log_manager &) = delete;
  global_stdout_log_manager(global_stdout_log_manager &&) = delete;

  auto operator=(global_stdout_log_manager)
      -> global_stdout_log_manager & = delete;

  auto operator=(const global_stdout_log_manager &)
      -> global_stdout_log_manager & = delete;

  auto operator=(global_stdout_log_manager &&)
      -> global_stdout_log_manager & = delete;

  ~global_stdout_log_manager() = default;

private:
  global_stdout_log_manager() = default;
};

class global_file_log_manager {
public:
  static auto get() noexcept -> log_manager & {
    static log_manager log(file_path);
    return log;
  }

  static const std::filesystem::path file_path;

  global_file_log_manager(const global_file_log_manager &) = delete;
  global_file_log_manager(global_file_log_manager &&) = delete;

  auto operator=(global_file_log_manager) -> global_file_log_manager & = delete;

  auto operator=(const global_file_log_manager &)
      -> global_file_log_manager & = delete;

  auto operator=(global_file_log_manager &&)
      -> global_file_log_manager & = delete;

  ~global_file_log_manager() = default;

private:
  global_file_log_manager() = default;
};

template <log_event e, typename... Args>
inline void log_all(fmt::string_view str, Args &&...args) noexcept {
  global_stdout_log_manager::get().log<e>(str, args...);
  global_file_log_manager::get().log<e>(str, args...);
}

} // namespace surge

#endif // SURGE_LOG_HPP
