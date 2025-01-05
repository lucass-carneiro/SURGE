#ifndef SURGE_CORE_ERROR_TYPES_HPP
#define SURGE_CORE_ERROR_TYPES_HPP

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
  texture_handle_creation,

  // Static Image errors
  image_load_error,
  image_stbi_error,
  image_shader_creation,
  openEXR_exception,

  // Text errors
  freetype_init,
  freetype_deinit,
  freetype_face_load,
  freetype_face_unload,
  freetype_set_face_size,
  freetype_character_load,
  freetype_charmap,
  freetype_null_face,

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
  glfw_window_hint_msaa,
  glfw_window_hint_api,
  glfw_window_hint_resize,
  glfw_window_creation,
  glfw_window_input_mode,
  glfw_set_usr_ptr,
  glfw_make_ctx,
  glfw_vsync,
  glfw_resize_callback,
  glfw_vk_ext_retrive,

  // OpenGL errors
  glad_loading,
  opengl_feature_missing,
  sdb_instance_alloc,
  sdb_fenc_alloc,
  sdb_bad_capacity,
  gc_inconsistent_creation_size,
  gc_instance_alloc,

  // Vulkan errors
  vk_ctx_alloc,
  vk_api_version_query,
  vk_instance_init,
  vk_val_layer_query,
  vk_val_layer_missing,
  vk_dbg_msg_ext_func_ptr,
  vk_dbg_msg_create,
  vk_phys_dev_enum,
  vk_phys_dev_ext_enum,
  vk_phys_dev_ext_missing,
  vk_phys_dev_no_suitable,
  vk_log_dev_create,
  vk_surface_present_query,
  vk_surface_present_unable,
  vk_surface_init,
  vk_swapchain_query,
  vk_init_swapchain,
  vk_swapchain_imgs,
  vk_swapchain_imgs_views,
  vk_init_draw_img,
  vk_graphics_queue_retrieve,
  vk_cmd_pool_creation,
  vk_cmd_buffer_creation,
  vk_fence_creation,
  vk_semaphore_creation,
  vk_allocator_creation,
  vk_fence_wait,
  vk_get_swpc_img,
  vk_cmd_buff_reset,
  vk_cmd_buff_rec_start,
  vk_cmd_buff_rec_end,
  vk_cmd_buff_submit,
  vk_present,
  vk_descriptor_set_layout_build,
  vk_descriptor_pool_init,
  vk_descriptor_pool_reset,
  vk_descriptor_set_alloc,
  vk_shader_module_create,
  vk_shader_obj_ext_func_ptr,
  vk_shader_object_create,

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

#endif // SURGE_CORE_ERROR_TYPES_HPP