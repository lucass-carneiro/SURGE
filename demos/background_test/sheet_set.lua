-- Convenience descriptor for the set
local sheet_set = {}

-- Sprite sheet for animation S_1
sheet_set[1] = {
    x0 = 0,
    y0 = 0,
    
    Sw = 480,
    Sh = 480,
    
    alpha = 0,
    beta = 0,
    
    rows = 1,
    cols = 3
}

-- Sprite sheet for animation S_2
sheet_set[2] = {
    x0 = 0,
    y0 = 480,
    
    Sw = 144,
    Sh = 480,
    
    alpha = 0,
    beta = 0,
    
    rows = 1,
    cols = 10
}

-- Sprite sheet for animation S_3
sheet_set[3] = {
    x0 = 0,
    y0 = 960,
    
    Sw = 240,
    Sh = 480,
    
    alpha = 0,
    beta = 0,
    
    rows = 1,
    cols = 3
}

-- Sprite sheet for animation S_4
sheet_set[4] = {
    x0 = 720,
    y0 = 960,
    
    Sw = 240,
    Sh = 240,
    
    alpha = 0,
    beta = 0,
    
    rows = 2,
    cols = 3
}

return sheet_set