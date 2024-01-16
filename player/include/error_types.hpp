#ifndef SURGE_ERROR_TYPES_HPP
#define SURGE_ERROR_TYPES_HPP

namespace surge {

enum error : int {
  // Cli errors
  config_file_load = 1,
  config_file_parse,

  // File errors
  invalid_path,
  read_error,
  invalid_format,
  unknow_error,

  // Module errors
  loading,
  name_retrival,
  symbol_retrival,

  // Renderer errors,
  unrecognized_shader,
  shader_load_error,
  shader_link_error,

  // Static Image errors
  static_image_load_error,
  static_image_stbi_error,
  static_image_shader_creation,

  // Text errors
  freetype_init,
  freetype_face_not_found,
  freetype_set_face_size,
  freetype_character_load,

  // Window errors
  glfw_init,
  glfw_monitor,
  glfw_monitor_size,
  glfw_monitor_scale,
  glfw_monitor_bounds,
  glfw_monitor_area,
  glfw_monitor_name,
  glfw_window_hint_major,
  glfw_window_hint_minor,
  glfw_window_hint_profile,
  glfw_window_hint_resize,
  glfw_window_creation,
  glfw_window_input_mode,
  glfw_make_ctx,
  glfw_vsync,
  glfw_resize_callback,
  glad_loading,

  // Module callback binding
  keyboard_event_binding,
  keyboard_event_unbinding,
  mouse_button_event_binding,
  mouse_button_event_unbinding,
  mouse_scroll_event_binding,
  mouse_scroll_event_unbinding,

  // Count
  count
};

} // namespace surge

#endif // SURGE_ERROR_TYPES_HPP