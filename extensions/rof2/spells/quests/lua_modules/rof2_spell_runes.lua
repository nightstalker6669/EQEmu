-- Original direct rune hand-ins, not selectable reward tasks.
-- No saved counter: the next reward is the first spell/discipline not learned.
local item_lib = require("items")
local data = require("rof2_spell_rune_data")
local M = {}
local classes = {WAR=1,CLR=2,PAL=3,RNG=4,SHD=5,DRU=6,MNK=7,BRD=8,
                 ROG=9,SHM=10,NEC=11,WIZ=12,MAG=13,ENC=14,BST=15,BER=16}
local owners = {sof=442026, underfoot=480107}

function M.next_reward(client, choices)
    for _, entry in ipairs(choices or {}) do
        if not client:HasSpellScribed(entry[1]) and
           not client:HasDisciplineLearned(entry[1]) then
            return entry
        end
    end
end

function M.hand_in(e, era)
    if e.self:GetNPCTypeID() ~= owners[era] then
        item_lib.return_items(e.self, e.other, e.trade)
        return
    end
    -- This API reports the permanent class, including when shrouded.
    local class = classes[e.other:GetClassAbbreviation()]
    local catalogue = data[era] or {}
    local handled = false
    for slot = 1, 4 do
        local inst = e.trade["item" .. slot]
        if not handled and inst and inst.valid then
            local rune = inst:GetID()
            local choices = catalogue[rune] and catalogue[rune][class]
            local reward = M.next_reward(e.other, choices)
            if reward and item_lib.check_turn_in(e.trade, {item1=rune}) then
                -- Consume exactly one rune through the native hand-in checker.
                -- Unused stacks, extra runes, wrong items and coins are returned.
                e.other:QuestReward(e.self, 0, 0, 0, 0, reward[2], 0)
                handled = true
            end
        end
    end
    item_lib.return_items(e.self, e.other, e.trade)
end

return M
