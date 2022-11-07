-- Window geometry
surge.window_width  = 1000
surge.window_height = 800

-- Window name
surge.window_name   = "SURGE OpenGL Window"

-- Window borders and output monitor
surge.windowed = true          -- true for windowed, false for fullscreen
surge.window_monitor_index = 1

-- The color to use while clearing the framebuffer.
-- The color currently set is Horseshit green.
surge.clear_color[1] = 0.2
surge.clear_color[2] = 0.3
surge.clear_color[3] = 0.3
surge.clear_color[4] = 1.0

-- Folder in which the engine files are located
surge.engine_dir = "/home/lucas/SURGE/"

-- Projection matrix setup
surge.z_near = -1.0
surge.z_far = 1.0
surge.field_of_view = 45.0 -- degrees
surge.perspective_projection = false -- If false, uses ortographic projection If true, the field of view value must be set.