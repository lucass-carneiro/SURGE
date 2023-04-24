-- Demonstrates character animations

local keyboard_motion_actor = require("character_animations/keyboard_motion_actor")

local z_level_0 = 0.0
local z_level_1 = 0.1
local z_level_2 = 0.2

local z_level_0_list = {}
local z_level_1_list = {}
local z_level_2_list = {}

-- Draws entity based on it's type (call appropriate draw function)
function draw_entity(entity)
    local name = getmetatable(entity).__name

    if name == "surge::image" then
        surge.image.draw(entity)
    elseif name == "surge::KMA" then
        entity:draw()
    end
end

function surge.pre_loop()
    -- Load background
    local bkg_sx = surge.config.window_width / 500.0
    local bkg_sy = surge.config.window_height / 288.0
    
    new_york_0 = surge.image.new(
        "character_animations/resources/background/background-0.png",
        0.0   , 0.0   , z_level_0,
        bkg_sx, bkg_sy, 1.0
    )

    new_york_1 = surge.image.new(
        "character_animations/resources/background/background-1.png",
        388.0 * bkg_sx, 153.0 * bkg_sy, z_level_2,
        bkg_sx        , bkg_sy        , 1.0
    )

    -- Load actor
    sophia = keyboard_motion_actor:new(
        "character_animations/resources/sophia/sophia.png", -- Sprite sheet image file
        "character_animations/resources/sophia/sophia.sad", -- Animations file
        4,                                                  -- First animation index
        16.0,  48.0,  0.0      ,                            -- Anchor
        600.0, 705.0, z_level_1,                            -- Position
        5.0,   5.0,   1.0                                   -- Scale
    )

    -- Add entities to z level lists
    table.insert(z_level_0_list, new_york_0)
    table.insert(z_level_1_list, sophia)
    table.insert(z_level_2_list, new_york_1)
end

function surge.key_event(key, action, mods)
    sophia:detect_arrows_released(key, action, scale)
end

function surge.mouse_button_event(button, action, mods)
    if button == surge.input.mouse.button.RIGHT and action == surge.input.action.PRESS then
        local click_x, click_y, click_z = surge.input.mouse.get_cursor_position()
        surge.log.message("(", click_x, ",", click_y, ",", click_z, ")")
    end
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end

function surge.draw()
    for _, entity in ipairs(z_level_0_list) do
        draw_entity(entity)
    end

    for _, entity in ipairs(z_level_1_list) do
        draw_entity(entity)
    end

    for _, entity in ipairs(z_level_2_list) do
        draw_entity(entity)
    end
end

function surge.update(dt)
    sophia:update(dt, 0.08)
end