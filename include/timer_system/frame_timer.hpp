#ifndef SURGE_FRAME_TIMER_HPP
#define SURGE_FRAME_TIMER_HPP

#include <chrono>

namespace surge {

class frame_timer {
public:
  using clock_t = std::chrono::steady_clock;
  using duration_t = std::chrono::duration<double>;
  using time_pt_t = std::chrono::time_point<clock_t>;

  static inline auto get() noexcept -> frame_timer & {
    static frame_timer ft;
    return ft;
  }

  inline void begin_frame() noexcept { begin = std::chrono::steady_clock::now(); }
  inline void end_frame() noexcept {
    const duration_t dt{std::chrono::steady_clock::now() - begin};
    previous_dt = dt.count();
  }

  [[nodiscard]] inline auto dt() const noexcept -> double { return previous_dt; }

  frame_timer(const frame_timer &) = delete;
  frame_timer(frame_timer &&) = delete;

  auto operator=(frame_timer) -> frame_timer & = delete;
  auto operator=(const frame_timer &) -> frame_timer & = delete;
  auto operator=(frame_timer &&) -> frame_timer & = delete;

  ~frame_timer() = default;

private:
  time_pt_t begin;
  double previous_dt{0};

  frame_timer() = default;
};

} // namespace surge

#endif // SURGE_FRAME_TIMER_HPP