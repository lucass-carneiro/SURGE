#ifndef SURGE_PLAYER_HPP
#define SURGE_PLAYER_HPP

#include "surge_core.hpp"

namespace player {

using opt_mod_handle = tl::expected<surge::module::handle_t, surge::error>;
using opt_mod_api = tl::expected<surge::module::api, surge::error>;
using opt_error = std::optional<surge::error>;

void gl_main_loop(opt_mod_handle &mod, opt_mod_api &mod_api, int &on_load_result,
                  opt_error &input_bind_result, const surge::config::clear_color &w_ccl,
                  const surge::config::renderer_attrs &r_attrs) noexcept;

} // namespace player

#endif // SURGE_PLAYER_HPP