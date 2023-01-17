 -- Demonstrates character animations

-- Load sheet set descriptions
local sophia_sheet_set = require("character_animations/sophia_sheet_set")

-- Number of sheets
local sophia_num_sheets = table.getn(sophia_sheet_set)

--  Index of the current set being shown (idle side)
local sophia_current_set = 6

-- Sprite scale factor
local scale_factor = 5.0

-- Convenience function that moves the sprite index forward
function next_sprite(sprite_obj, sprite_sheet)
    if (sprite_sheet.alpha < 0 or sprite_sheet.beta < 0) then
        sprite_sheet.alpha = 0;
        sprite_sheet.beta = 0;
    elseif (sprite_sheet.beta + 1) < sprite_sheet.cols then
        sprite_sheet.beta = sprite_sheet.beta + 1;
    else
        sprite_sheet.alpha = (sprite_sheet.alpha + 1) % sprite_sheet.rows;
        sprite_sheet.beta = 0;
    end

    surge.sheet_set_indices(sprite_obj, sprite_sheet.alpha, sprite_sheet.beta)
end

-- Convenience function to advance to the next sheet
function next_sheet()
    if (sophia_current_set + 1) <= sophia_num_sheets then
        sophia_current_set = sophia_current_set + 1
    else
        sophia_current_set = 1
    end
end

function surge.key_event(key, action, mods)
    if key == surge.keyboard_key.UP and action == surge.input_action.PRESS then
        next_sheet()
    end

    surge.sheet_set_offsets(sophia_sprite, sophia_sheet_set[sophia_current_set].x0, sophia_sheet_set[sophia_current_set].y0)
    surge.sheet_set_dimentions(sophia_sprite, sophia_sheet_set[sophia_current_set].Sw, sophia_sheet_set[sophia_current_set].Sh)
    surge.sheet_set_indices(sophia_sprite, sophia_sheet_set[sophia_current_set].alpha, sophia_sheet_set[sophia_current_set].beta)

    surge.set_sprite_geometry(
        sophia_sprite,
        
        -- Pos
        (surge.window_width - sophia_sheet_set[sophia_current_set].Sw * scale_factor) / 2.0, 
        (surge.window_height - sophia_sheet_set[sophia_current_set].Sh * scale_factor) / 2.0,
        0.0,
        
        -- Scale
        sophia_sheet_set[sophia_current_set].Sw * scale_factor,
        sophia_sheet_set[sophia_current_set].Sh * scale_factor,
        0.0
    )
end

function surge.mouse_button_event(button, action, mods)
    -- do nothing
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end

function surge.pre_loop()    
    -- Load Sprite
    sophia_sprite = surge.load_sprite("character_animations/resources/sophia_spritesheet.png", ".png")

    surge.set_sprite_geometry(
        sophia_sprite,
        
        -- Pos
        (surge.window_width - sophia_sheet_set[sophia_current_set].Sw * scale_factor) / 2.0, 
        (surge.window_height - sophia_sheet_set[sophia_current_set].Sh * scale_factor) / 2.0,
        0.0,
        
        -- Scale
        sophia_sheet_set[sophia_current_set].Sw * scale_factor,
        sophia_sheet_set[sophia_current_set].Sh * scale_factor,
        0.0
    )
    
    -- Sprite sheet selection
    surge.sheet_set_offsets(sophia_sprite, sophia_sheet_set[sophia_current_set].x0, sophia_sheet_set[sophia_current_set].y0)
    surge.sheet_set_dimentions(sophia_sprite, sophia_sheet_set[sophia_current_set].Sw, sophia_sheet_set[sophia_current_set].Sh)
    surge.sheet_set_indices(sophia_sprite, sophia_sheet_set[sophia_current_set].alpha, sophia_sheet_set[sophia_current_set].beta)
end

function surge.draw()
    surge.draw_sprite(sophia_sprite)
end

local animation_frame_time = 0.0

function surge.update(dt)
    animation_frame_time = animation_frame_time + dt
    if animation_frame_time > 0.1 then
        animation_frame_time = 0.0
        next_sprite(sophia_sprite, sophia_sheet_set[sophia_current_set])
    end
end