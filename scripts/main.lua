-- SURGE entry point script
surge.log_message("Configuration summary:\n",
"  VM index: ", surge.vm_index, "\n",
"  Window width:  " , surge.window_width, "\n",
"  Window height: ", surge.window_height, "\n",
"  Window name: ", surge.window_name, "\n",
"  Windowed: ", surge.windowed, "\n",
"  Monitor index: ", surge.window_monitor_index, "\n",
"  Renderer clear color (R,G,B,A): (", surge.clear_color[1], ",",
                                       surge.clear_color[2], ",",
                                       surge.clear_color[3], ",",
                                       surge.clear_color[4], ")"
)

-- Example: Load image
-- local img = surge.load_image("/home/lucas/SURGE/resources/images/awesomeface.png", ".png")
-- img = surge.drop_image(img)

local root_dir = "/home/lucas/SURGE/"

function surge.load()
    surge.log_message("surge.load called in VM ", surge.vm_index)
    
    -- Compile and load shaders
    -- vs_dir = root_dir.."shaders/default.vert"
    -- fs_dir = root_dir.."shaders/default.frag"

    -- surge.log_message("Compiling and linking ", vs_dir, " + ", fs_dir)

    -- surge.current_shader_program = surge.create_program(vs_dir, fs_dir)
end