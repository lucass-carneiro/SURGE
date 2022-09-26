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
#include <fmt/color.h>
#include <fmt/core.h>
// clang-format on

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <gsl/gsl-lite.hpp>
#include <iostream>
#include <memory>

#ifdef SURGE_ENABLE_THREADS
#  include <mutex>
#endif

#include <string>

namespace surge {

/**
 * @brief The type of event to be logged
 *
 */
enum class log_event : std::uint8_t { logo, warning, error, message, memory, count };

/**
 * @brief Type of a static hash map of events and colors.
 *
 */
using log_color_map_t
    = static_map<log_event, fmt::text_style, static_cast<std::size_t>(log_event::count)>;

/**
 * @brief Type of a static hash map of events and banners.
 *
 */
using log_banner_map_t
    = static_map<log_event, const char *, static_cast<std::size_t>(log_event::count) - 1>;

/**
 * @brief Event-color hash map
 *
 */
constexpr const log_color_map_t log_color_map{
    {{{log_event::logo, fmt::emphasis::bold | fg(fmt::color::crimson)},
      {log_event::warning, fmt::fg(fmt::color::yellow)},
      {log_event::error, fmt::emphasis::bold | fg(fmt::color::red)},
      {log_event::message, fg(fmt::color::steel_blue)},
      {log_event::memory, fg(fmt::color::aquamarine)}}}};

/**
 * @brief Event-banner hash map
 *
 */
constexpr const log_banner_map_t log_banner_map{{{{log_event::warning, "warning"},
                                                  {log_event::error, "error"},
                                                  {log_event::message, "message"},
                                                  {log_event::memory, "memory event"}}}};

/**
 * Handles logging output to files and stdout.
 */
class log_manager {
public:
  log_manager() noexcept : output_file(nullptr, std::fclose) {}

  void init(const std::filesystem::path &path) noexcept { output_file.reset(open_file(path)); }

#ifdef SURGE_USE_LOG
  /**
   * @brief Log an event.
   *
   * This function logs an event using fmt print syntax.
   *
   * @tparam e The type of event to log.
   * @tparam Args The types of the format arguments. This is automatically
   * deduced by the compiler. and should not be specified.
   * @param str A fmt-like format string.
   * @param args The arguments of the fmt-like format string.
   */
  template <log_event e, typename... Args>
  inline void log(fmt::string_view str, Args &&...args) noexcept {
#  ifdef SURGE_ENABLE_THREADS
    const std::lock_guard lock(log_mutex);
#  endif
    vlog<e>(stdout, str, fmt::make_format_args(args...));
    vlog<e>(output_file.get(), str, fmt::make_format_args(args...));
  }
#else
  /**
   * @brief Log an event. This implementation is a no-op since SURGE_USE_LOG is disabled
   *
   * This function logs an event using fmt print syntax.
   *
   * @tparam e The type of event to log.
   * @tparam Args The types of the format arguments. This is automatically
   * deduced by the compiler. and should not be specified.
   * @param str A fmt-like format string.
   * @param args The arguments of the fmt-like format string.
   */
  template <log_event e, typename... Args>
  inline void log(fmt::string_view, Args &&...) const noexcept {
    return;
  }
#endif

private:
  std::unique_ptr<std::FILE, int (*)(std::FILE *)> output_file;

#ifdef SURGE_ENABLE_THREADS
  // Protects log file writes
  std::mutex log_mutex;
#endif

  /**
   * @brief Invokes fopen and sets output_file either to the oppened file or stderr
   * if fopen fails
   *
   * @param path Path to the file to open
   */
  [[nodiscard]] auto open_file(const std::filesystem::path &path) const noexcept -> std::FILE * {
    auto *temp_file = gsl::owner<std::FILE *>{fopen(path.c_str(), "w")};

    if (temp_file == nullptr) {
      temp_file = stderr;
    }

    return temp_file;
  }

  /**
   * @brief Prints a string with a colored banner
   *
   * @param file The file to print to.
   * @param banner The banner of the log message.
   * @param style The style of the log message.
   * @param str The message content
   * @param args Arguments to print in the message.
   */
  void colored_print(std::FILE *file, fmt::string_view banner, const fmt::text_style &style,
                     fmt::string_view str, fmt::format_args args) const noexcept {
    try {
      // clang-format off
      fmt::print(file,
                #ifdef SURGE_USE_LOG_COLOR
                 style,
                #endif
                "SURGE {}: ",
                banner
      );
      // clang-format on

      fmt::vprint(file, str, args);
      fmt::print(file, "\n");

    } catch (const std::exception &e) {
      std::cout << "Error while invonking fmt: " << e.what() << std::endl;
    }
  }

  /**
   * @brief Print a log entry to the screen.
   *
   * @tparam e The type of event to log.
   * @param file The file to print to.
   * @param str A string containing fmt format information.
   * @param args The arguments of the format string.
   */
  template <log_event e>
  void vlog(std::FILE *file, fmt::string_view str, fmt::format_args args) const noexcept {
    if constexpr (e == log_event::logo) {
      try {
        // clang-format off
        fmt::vprint(file,
                    #ifdef SURGE_USE_LOG_COLOR
                    log_color_map[e],
                    #endif
                    str,
                    args
        );
        // clang-format on

        fmt::print(file, "\n");

      } catch (const std::exception &ex) {
        std::cout << "Error while invonking fmt: " << ex.what() << std::endl;
      }

    } else {
      colored_print(file, log_banner_map[e], log_color_map[e], str, args);
    }
  }
};

class global_log_manager {
public:
  static auto get() noexcept -> log_manager & {
    static log_manager log;
    return log;
  }

  global_log_manager(const global_log_manager &) = delete;
  global_log_manager(global_log_manager &&) = delete;

  auto operator=(global_log_manager) -> global_log_manager & = delete;

  auto operator=(const global_log_manager &) -> global_log_manager & = delete;

  auto operator=(global_log_manager &&) -> global_log_manager & = delete;

  ~global_log_manager() = default;

private:
  global_log_manager() = default;
};

template <log_event e, typename... Args>
inline void glog(fmt::string_view str, Args &&...args) noexcept {
  global_log_manager::get().log<e>(str, args...);
}

} // namespace surge

#endif // SURGE_LOG_HPP
