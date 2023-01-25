-- Convenience sprite set descriptor for sophia animations
local sophia_sheet_set = {}

-- sophia_back_walk
sophia_sheet_set[1] = {
    x0 = 0,
    y0 = 100,

    Sw = 33,
    Sh = 50,

    alpha = 0,
    beta = 0,

    rows = 1,
    cols = 5
}

-- sophia_front_walk
sophia_sheet_set[2] = {
    x0 = 0,
    y0 = 50,

    Sw = 33,
    Sh = 50,

    alpha = 0,
    beta = 0,

    rows = 1,
    cols = 6
}

-- sophia_idle_back
sophia_sheet_set[3] = {
    x0 = 198,
    y0 = 50,

    Sw = 33,
    Sh = 50,

    alpha = 0,
    beta = 0,

    rows = 1,
    cols = 2
}

-- sophia_idle_front
sophia_sheet_set[4] = {
    x0 = 165,
    y0 = 100,

    Sw = 33,
    Sh = 50,

    alpha = 0,
    beta = 0,

    rows = 1,
    cols = 3
}

-- sophia_idle_side
sophia_sheet_set[5] = {
    x0 = 0,
    y0 = 150,

    Sw = 33,
    Sh = 50,

    alpha = 0,
    beta = 0,

    rows = 1,
    cols = 1
}

-- sophia_side_walk
sophia_sheet_set[6] = {
    x0 = 0,
    y0 = 0,

    Sw = 33,
    Sh = 50,

    alpha = 0,
    beta = 0,

    rows = 1,
    cols = 8
}

return sophia_sheet_set