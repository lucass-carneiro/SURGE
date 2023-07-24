#ifndef SURGE_CLI_HPP
#define SURGE_CLI_HPP

namespace surge::cli {

void draw_logo() noexcept;

auto parse_arguments(int argc, char **argv) noexcept -> bool;

} // namespace surge::cli

#endif // SURGE_CLI_HPP