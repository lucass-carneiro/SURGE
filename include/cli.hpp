#ifndef SURGE_CLI_HPP
#define SURGE_CLI_HPP

#include <filesystem>
#include <optional>
#include <tuple>

namespace surge {

using cmd_args_t = std::optional<std::tuple<const char *, const char *>>;

void draw_logo() noexcept;

auto parse_arguments(int argc, char **argv) noexcept -> cmd_args_t;

} // namespace surge

#endif // SURGE_CLI_HPP