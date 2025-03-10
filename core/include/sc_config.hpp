#ifndef SURGE_CORE_CONFIG_HPP
#define SURGE_CORE_CONFIG_HPP

#include "sc_container_types.hpp"
#include "sc_error_types.hpp"

namespace surge::config {

struct WindowResolution {
  int width{800};
  int height{600};
};

struct ClearColor {
  float r{0.0f};
  float g{0.0f};
  float b{0.0f};
  float a{1.0f};
};

struct WindowAttributes {
  using String = surge::containers::mimalloc::String;
  String name{"SURGE Window"};
  int monitor_index{0};
  bool windowed{true};
  bool cursor{true};
};

enum class RenderBackend { opengl, vulkan };

struct RendererAttributes {
  RenderBackend backend{RenderBackend::opengl};
  bool vsync{true};
  bool MSAA{true};
  bool fps_cap{true};
  int fps_cap_value{60};
};

struct ConfigData {
  using String = surge::containers::mimalloc::String;
  WindowResolution wr{};
  ClearColor ccl{};
  WindowAttributes wattrs{};
  RendererAttributes rattrs{};
  String module{};
};

auto parse_config(RenderBackend &&backend) -> Result<ConfigData>;

} // namespace surge::config

#endif // SURGE_CORE_CONFIG_HPP