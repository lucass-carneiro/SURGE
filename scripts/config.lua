-- Window geometry
surge.window_width  = 1000
surge.window_height = 800

-- Window name
surge.window_name   = "SURGE OpenGL Window"

-- Window borders and output monitor
surge.windowed = true          -- true for windowed, false for fullscreen
surge.window_monitor_index = 1

-- Show/hide window cursor
surge.show_cursor = false -- Not working

-- The color to use while clearing the framebuffer.
-- The color currently set is Horseshit green.
surge.clear_color[1] = 0.2
surge.clear_color[2] = 0.3
surge.clear_color[3] = 0.3
surge.clear_color[4] = 1.0

-- Folder structure
surge.engine_root_dir = "/home/lucas/SURGE/"

-- Load external libraries
surge.serialize = require("scripts/libs/ser")