local M = {
    forest = 378,
    shrine = 379,
    shalowen = {npc = 378059, spawn = 104960},
    orgaf = {npc = 378168, spawn = 105177},
    entrance = {door = 16, db = 16765, x = -297.642, y = -1462.02, z = -84.0911},
    shrine_landing = {0, 0, -6, 0},
    forest_return = {-298.01, -1475.55, -96, 256},
    nalderin_aura = 9017,
    nalderin_floors = {
        {-7,104,225,295}, {-54,61,181.375,429}, {12,67,120.25,187},
        {-9,150,76.625,62}, {0,94,-3.5,264},
    },
    brazlin_pad = {-43.936,65.993,282.487,0},
    guardian_pad = {0.544,82.863,282.487,0},
    drekish_pad = {28,100,282.487,0},
    brazlin_lap = {
        {-55,90}, {-40,128}, {0,144}, {40,128}, {55,90},
        {40,52}, {0,36}, {-40,52}, {-43.936,65.993},
    },
    core_pads = {
        {-28,91,282.487,0}, {-30,84,282.487,0}, {-27,77,282.487,0},
        {28,75,282.487,0}, {30,83,282.487,0}, {29,91,282.487,0},
        {-8,112,282.487,0}, {0,113,282.487,0}, {8,112,282.487,0},
    },
    items = {
        figurine = 85076, stone = 85077, restored = 85078, key = 85079,
        scroll = 85080, woods = {85081, 85082, 85083, 85084},
        chalice = 85085, augmentation = 85670,
    },
    achievement = {id = 500990210, exchange = 59902100, stages = {59902101, 59902102, 59902103, 59902104}},
    treants = {
        {npc = 378206, spawn = 105247, item = 85081},
        {npc = 378089, spawn = 105007, item = 85082},
        {npc = 378147, spawn = 105137, item = 85083},
        {npc = 378208, spawn = 105249, item = 85084},
    },
}

-- Shared tasks and the reserved versioned Shrine entrance and upper doors.
M.stages = {
    {id = 920346, level = 65, version = 20, entry = 304090400},
    {id = 920347, level = 70, version = 21, entry = 304090402},
    {id = 920348, level = 70},
    {id = 920349, level = 70, version = 22, entry = 304090404, upper = 304090405, raid = true},
}
return M
