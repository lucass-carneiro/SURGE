#include "allocators/global_allocators.hpp"
#include "gui_windows/gui_windows.hpp"
#include "thread_allocators.hpp"
#include "window.hpp"

#include <imgui.h>
#include <type_traits>

/**
 * @brief Draws an allocator bar.
 *
 * @param bar_color Color of the allocator bar
 * @param fill The fill proportion of the bar in the range[0, 1]
 */
static void allocator_bar(float fill) noexcept {
  ImDrawList *draw_list = ImGui::GetWindowDrawList();

  const ImU32 fill_collor{ImColor{ImVec4{1.0f, 0.0f, 0.0f, 1.0f}}};
  const ImU32 empty_collor{ImColor{ImVec4{0.0f, 0.0f, 1.0f, 1.0f}}};

  const ImVec2 fill_start{ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y + 3.0f};
  const ImVec2 fill_end{fill_start.x + ImGui::GetContentRegionAvail().x * fill,
                        fill_start.y + 8.0f};
  const ImVec2 empty_end{fill_start.x + ImGui::GetContentRegionAvail().x, fill_start.y + 8.0f};

  draw_list->AddRectFilled(fill_start, empty_end, empty_collor);
  draw_list->AddRectFilled(fill_start, fill_end, fill_collor);
}

struct allocator_entry_data {
  const char *name;
  const std::size_t requested;
  const std::size_t actual;
  const std::size_t used;
  const float percentage;
};

template <typename alloc_t>
static auto retieve_allocator_entry_data(const alloc_t &allocator) noexcept
    -> allocator_entry_data {

  if constexpr (std::is_same<surge::global_thread_allocators::stack_allocator_ptr,
                             alloc_t>::value) {

#ifdef SURGE_DEBUG_MEMORY
    const auto name{allocator->get_debug_name()};
#else
    const auto name{"Stack allocator"};
#endif

    const auto requested{allocator->get_requested_capacity()};
    const auto actual{allocator->get_actual_capacity()};
    const auto used{allocator->get_allocated_size()};
    const float percentage{static_cast<float>(used) / static_cast<float>(actual)};

    return allocator_entry_data{name, requested, actual, used, percentage};

  } else {

#ifdef SURGE_DEBUG_MEMORY
    const auto name{allocator.get_debug_name()};
#else
    const auto name{"Arena allocator"};
#endif

    const auto requested{allocator.get_requested_capacity()};
    const auto actual{allocator.get_actual_capacity()};
    const auto used{allocator.get_allocated_size()};
    const float percentage{static_cast<float>(used) / static_cast<float>(actual)};

    return allocator_entry_data{name, requested, actual, used, percentage};
  }
}

template <typename alloc_t> static inline void allocator_entry(const alloc_t &allocator) noexcept {

  const allocator_entry_data data{retieve_allocator_entry_data(allocator)};

  ImGui::TableNextRow();
  ImGui::TableNextColumn();

  ImGui::Text("%s", data.name); // NOLINT(cppcoreguidelines-pro-type-vararg)

  ImGui::TableNextColumn();
  ImGui::Text("%lu", data.requested); // NOLINT(cppcoreguidelines-pro-type-vararg)

  ImGui::TableNextColumn();
  ImGui::Text("%lu", data.actual); // NOLINT(cppcoreguidelines-pro-type-vararg)

  ImGui::TableNextColumn();
  ImGui::Text("%lu", data.used); // NOLINT(cppcoreguidelines-pro-type-vararg)

  ImGui::TableNextColumn();
  allocator_bar(data.percentage);
}

void surge::show_memory_profiler_window(bool *open) noexcept {
  static const ImVec2 window_size{600.0f, 530.0f};
  ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);

  if (ImGui::Begin("Memory profiler", open, ImGuiWindowFlags_NoResize)) {

    if (ImGui::BeginTable("Allocator table", 5, ImGuiTableFlags_SizingStretchSame)) {

      ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Requested (B)", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Actual (B)", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Used (B)", ImGuiTableColumnFlags_WidthFixed);
      ImGui::TableSetupColumn("Graph", ImGuiTableColumnFlags_WidthFixed, window_size.x / 3.3f);
      ImGui::TableHeadersRow();

      allocator_entry(global_linear_arena_allocator::get());

      for (const auto &allocator : global_thread_allocators::get().get_allocator_array()) {
        allocator_entry(allocator);
      }
    }

    ImGui::EndTable();
  }

  ImGui::End();
}