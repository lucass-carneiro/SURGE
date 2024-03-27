#ifndef SURGE_ERROR_TYPES_HPP
#define SURGE_ERROR_TYPES_HPP

namespace surge {

enum error : int {
  // Normal exit signal
  normal_exit = 1,

  // Cli errors
  config_file_load,
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
  texture_handle_creation,

  // Static Image errors
  static_image_load_error,
  static_image_stbi_error,
  static_image_shader_creation,

  // Text errors
  freetype_init,
  freetype_deinit,
  freetype_face_load,
  freetype_face_unload,
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
  glfw_hint_resize,
  glfw_hint_noapi,
  glfw_ext_querry,
  glfw_window_creation,
  glfw_window_input_mode,
  glfw_make_ctx,
  glfw_vsync,
  glad_loading,

  // Vulkan errors
  vk_instance_create,
  vk_validation_layers_not_available,
  vk_debug_msg,
  vk_ext_not_found,

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