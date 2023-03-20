-- Window geometry
surge.config.window_width  = 1000
surge.config.window_height = 800

-- Window name
surge.config.window_name   = "SURGE OpenGL Window"

-- Window borders and output monitor
surge.config.windowed = true          -- true for windowed, false for fullscreen
surge.config.window_monitor_index = 1

-- Show/hide window cursor
surge.config.show_cursor = true

-- Key for showing/hiding debug objects (not implemented)
surge.config.show_debug_objects = surge.input.keyboard.key.F11

-- The color to use while clearing the framebuffer.
-- The color currently set is Horseshit green.
surge.config.clear_color[1] = 0.2
surge.config.clear_color[2] = 0.3
surge.config.clear_color[3] = 0.3
surge.config.clear_color[4] = 1.0

-- Folder structure
surge.config.engine_root_dir = "/home/lucas/SURGE/"