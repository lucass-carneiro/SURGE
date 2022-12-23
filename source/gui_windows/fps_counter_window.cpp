#include "gui_windows/gui_windows.hpp"
#include "window.hpp"

#include <cmath>
#include <limits>
#include <numeric>

template <typename T, std::size_t samples> class rolling_data {
public:
  rolling_data() {
    const auto dt0{1.0 / surge::global_engine_window::get().get_frame_dt()};
    std::fill(x_data.begin(), x_data.end(), 0.0);
    std::fill(y_data.begin(), y_data.end(), dt0);

    for (T i = T{0}; auto &x : x_data) {
      x = i;
      i += T{1};
    }
  }

  void insert_point(T x, T y) noexcept {
    last_idx %= samples;
    x_data[last_idx] = x;
    y_data[last_idx] = y;
    last_idx++;

    if (y < min) {
      min = y;
    }

    if (y > max) {
      max = y;
    }

    avg = std::accumulate(y_data.begin(), y_data.end(), T{0}) / T{samples};
  }

  [[nodiscard]] auto get_x_data() const noexcept -> const std::array<T, samples> & {
    return x_data;
  }

  [[nodiscard]] auto get_y_data() const noexcept -> const std::array<T, samples> & {
    return y_data;
  }

  [[nodiscard]] auto get_last_idx() const noexcept -> std::size_t { return last_idx; }

  [[nodiscard]] auto get_min() const noexcept -> T { return min; }

  [[nodiscard]] auto get_max() const noexcept -> T { return max; }

  [[nodiscard]] auto get_avg() const noexcept -> T { return avg; }

private:
  std::array<T, samples> x_data{samples};
  std::array<T, samples> y_data{samples};

  std::size_t last_idx{0};

  T min{std::numeric_limits<T>::max()}, max{std::numeric_limits<T>::min()}, avg{0};
};

void surge::show_fps_counter_window(bool *open) noexcept {

  ImGui::SetNextWindowSize(ImVec2(430, 430), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("FPS counter", open, ImGuiWindowFlags_NoResize)) {

    static constexpr const std::size_t max_samples{SURGE_FPS_COUNTER_SAMPLE_SIZE};
    static constexpr const double x_max{max_samples};

    static rolling_data<double, max_samples> plot_data{};
    static double x{0};

    const auto frame_rate{1.0 / global_engine_window::get().get_frame_dt()};

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    ImGui::Text("Avg. : %.0f FPS", plot_data.get_avg());

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    ImGui::Text("Min. : %.0f FPS", plot_data.get_min());

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    ImGui::Text("Max. : %.0f FPS", plot_data.get_max());

    x = std::fmod(x, x_max);

    plot_data.insert_point(x, frame_rate);
    x += 1.0;

    if (ImPlot::BeginPlot("##Rolling")) {
      ImPlot::SetupAxes("Time", "Frame Rate (FPS)", ImPlotAxisFlags_NoTickLabels);
      ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, x_max - 1.0, ImGuiCond_Always);
      ImPlot::SetupAxisLimits(ImAxis_Y1, 50.0, 70.0);
      ImPlot::PlotLine("Frame rate", plot_data.get_x_data().data(), plot_data.get_y_data().data(),
                       plot_data.get_last_idx());
      ImPlot::EndPlot();
    }

    ImGui::End();
  }
}