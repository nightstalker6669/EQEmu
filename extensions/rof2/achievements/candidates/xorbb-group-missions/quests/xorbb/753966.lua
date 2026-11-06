-- Reserved mission-only Chrom753966; no generic spawn/reload initialization.
-- Native kill, corpse heart loot and task rewards remain native producers.
local Chrom=require('achievement_quest_xorbb_chrom_events')
function event_timer(e)Chrom.timer(e)end
function event_combat(e)Chrom.combat(e)end
function event_hp(e)Chrom.hp(e)end
