-- Demonstrates the usage of a sprite set with multiple sprite sheets, each with different properties

-- Load sheet set descriptions
local sheet_set = require("background_test/sheet_set")

-- Number of sheets
local num_sheets = table.getn(sheet_set)

--  Index of the current set being shown
local current_sheet_idx = 1

-- Convenience function that moves the sprite index forward
function next_sprite(sprite_sheet)
    if (sprite_sheet.alpha < 0 or sprite_sheet.beta < 0) then
        sprite_sheet.alpha = 0;
        sprite_sheet.beta = 0;
    elseif (sprite_sheet.beta + 1) < sprite_sheet.cols then
        sprite_sheet.beta = sprite_sheet.beta + 1;
    else
        sprite_sheet.alpha = (sprite_sheet.alpha + 1) % sprite_sheet.rows;
        sprite_sheet.beta = 0;
    end
end

-- Convenience function to advance to the next sheet
function next_sheet()
    if (current_sheet_idx + 1) <= num_sheets then
        current_sheet_idx = current_sheet_idx + 1
    else
        current_sheet_idx = 1
    end
end

function surge.key_event(key, action, mods)
    if key == surge.keyboard_key.RIGHT and action == surge.input_action.PRESS then
        next_sprite(sheet_set[current_sheet_idx])
    elseif key == surge.keyboard_key.UP and action == surge.input_action.PRESS then
        next_sheet()
    end

    -- We can update the state here or in the update function
    surge.sheet_set_offsets(sprite, sheet_set[current_sheet_idx].x0, sheet_set[current_sheet_idx].y0)
    surge.sheet_set_dimentions(sprite, sheet_set[current_sheet_idx].Sw, sheet_set[current_sheet_idx].Sh)
    surge.sheet_set_indices(sprite, sheet_set[current_sheet_idx].alpha, sheet_set[current_sheet_idx].beta)
end

function surge.pre_loop()    
    -- Load Sprite
    sprite = surge.load_sprite("background_test/resources/test_sheet_set.png", ".png")
    
    -- Sprite sheet selection
    surge.sheet_set_offsets(sprite, sheet_set[current_sheet_idx].x0, sheet_set[current_sheet_idx].y0)
    surge.sheet_set_dimentions(sprite, sheet_set[current_sheet_idx].Sw, sheet_set[current_sheet_idx].Sh)
    surge.sheet_set_indices(sprite, sheet_set[current_sheet_idx].alpha, sheet_set[current_sheet_idx].beta)
    
    -- Sprite position and scale
    surge.move_sprite(sprite, surge.window_width / 4.0, surge.window_height / 4.0, 0.0)
    surge.scale_sprite(sprite, surge.window_width / 2.0, surge.window_height / 2.0, 0.0)
end

function surge.draw()
    surge.draw_sprite(sprite)
end

function surge.update(dt)
    -- Do nothing
end

function surge.mouse_button_event(button, action, mods)
    -- do nothing
end

function surge.mouse_scroll_event(xoffset, yoffset)
    -- do nothing
end