-- SURGE entry point script
surge.log_message("Configuration summary:\n",
"  VM index: ", surge.vm_index, "\n",
"  Window width:  " , surge.window_width, "\n",
"  Window height: ", surge.window_height, "\n",
"  Window name: ", surge.window_name, "\n",
"  Windowed: ", surge.windowed, "\n",
"  Monitor index: ", surge.window_monitor_index, "\n",
"  Renderer clear color (R,G,B,A): (", surge.clear_color[0], ",",
                                       surge.clear_color[1], ",",
                                       surge.clear_color[2], ",",
                                       surge.clear_color[3], ")"
)

local img = surge.load_image("/home/lucas/SURGE/resources/images/awesomeface.png", ".png")
print("value in img: ", img)
img = surge.drop_image(img)
print("value in img: ", img)