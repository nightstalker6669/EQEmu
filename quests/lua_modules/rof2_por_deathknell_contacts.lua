local por = require("rof2_por_progression")
local deathknell = require("rof2_por_deathknell")

local M = {}
local crystals_task, bones_task = 920344, 920345
local inspiration = {52594, 52595, 52596, 52597, 52598, 52599}
local shards = {84164, 84165, 84166, 84167, 52601}

local function say_link(phrase)
    return "[" .. eq.say_link(phrase) .. "]"
end

local function carrying(client, items)
    for _, item in ipairs(items) do
        if client:CountItem(item) > 0 then return true end
    end
    return false
end

local function deliver(e, task, items)
    local handled = false
    for i, item in ipairs(items) do
        if e.other:IsTaskActivityActive(task, i-1) and por.offers(e, item) and por.take(e, {[item]=1}) then
            por.advance(e.other, task, i-1)
            handled = true
        end
    end
    return handled
end

function M.sarcrynn(e)
    if not por.public(e.self, 369, 369097, 103695) then return end
    local words = por.words(e)
    if words == "hail" then
        e.self:Say("Ao keeps us looking for spell components while he studies the " .. say_link("real magic") .. ".")
    elseif words == "real magic" or words == "what real magic" then
        por.offer(e.self, e.other, crystals_task, nil, 1)
    end
end

function M.sarcrynn_trade(e)
    if por.public(e.self, 369, 369097, 103695) then
        deliver(e, crystals_task, {85646, 85647, 85648, 85649})
    end
    e.self:ReturnHandinItems(e.other)
end

function M.orsarro(e)
    if not por.public(e.self, 369, 369085, 103667) then return end
    local words = por.words(e)
    if words == "hail" then
        por.advance(e.other, crystals_task, 4)
        if e.other:IsTaskCompleted(crystals_task) then
            e.self:Say("The crystals will unseal the path to " .. say_link("Daosheen the Firstborn") .. ".")
        end
    elseif words == "daosheen the firstborn" then
        deathknell.request(e.other, "daosheen")
    end
end

function M.maelin_say(e)
    if not por.public(e.self, 202, 202125, 58619) then return false end
    local words = por.words(e)
    local client = e.other
    if words == "hail" and client:CountItem(88099) > 0 and not client:IsTaskCompleted(bones_task) then
        e.self:Say("Bring me " .. say_link("legendary berserker bones") .. " to strengthen your charm.")
        return false
    end
    if words ~= "legendary berserker bones" then return false end
    if client:CountItem(88099) > 0 and deathknell.completed_shared(client, 920305) then
        por.offer(e.self, client, bones_task, nil, 70)
    end
    return true
end

function M.maelin_trade(e)
    if not por.public(e.self, 202, 202125, 58619) or e.other:CountItem(88099) == 0 then
        return false
    end
    return deliver(e, bones_task, {88102, 88103, 88104})
end

function M.sullon_request(e)
    local words = por.words(e)
    if words == "fix whatever is wrong" or words == "we will fix whatever is wrong" then
        deathknell.request(e.other, "sullon")
        return true
    end
    if words == "feel disconnected" then
        e.self:Say("Bring a raid to her tower if you will " .. say_link("fix whatever is wrong") .. ".")
        return true
    end
    if words == "hail" and e.other:IsTaskCompleted(bones_task) then
        e.self:Say("Since her rage changed, I " .. say_link("feel disconnected") .. " from her.")
    end
    return false
end

function M.suchun_request(e)
    local words = por.words(e)
    if words == "disturbance" or words == "what disturbance" then
        deathknell.request(e.other, "suchun")
        return true
    end
    if words == "hail" and deathknell.completed_shared(e.other, 920309) then
        e.self:Say("I found a rotting tree in the ruins. Investigate the " .. say_link("disturbance") .. " there.")
        return true
    end
    return false
end

function M.emissary(e)
    if not por.public(e.self, 383, 383166, 107559) then return end
    local words = por.words(e)
    if words == "hail" then
        e.self:Say("My mistress seeks adventurers strong enough to accept her " .. say_link("proposition") .. ".")
    elseif words == "proposition" then
        e.self:Say("If you are " .. say_link("interested") .. ", take a token to Druzzil Ro.")
    elseif words == "interested" or words == "i am interested" then
        if not e.other:HasZoneFlag(381) and not carrying(e.other, shards) then e.other:SummonItem(84164) end
        e.self:Say("Speak with the Shrine of Druzzil Ro in Arcstone when you have proved your strength.")
    end
end

local shrine_dialogue = {
    task = {"A divine power has been corrupted. I need an ", "enchantment", " to contain it."},
    enchantment = {"Rallos Zek took ", "steps", " to contain the fury of Sullon Zek."},
    steps = {"The blood of Rallos and his sons created a ward below Sullon's tower. Are you ", "willing", " to seek it?"},
    troubles = {"Ayonae Ro's realm has been taken by ", "mistmoore", "."},
    mistmoore = {"I need your ", "help", " to finish the ward and free my daughter."},
}

function M.shrine(e)
    if not por.public(e.self, 369, 369067, 103627) or not deathknell.credit(e.other, "daosheen") then return end
    local words, client, npc = por.words(e), e.other, e.self
    if words == "hail" then
        if client:CountItem(84167) > 0 then
            npc:Say("Place the Black Shard of Mana upon the shrine.")
        elseif client:CountItem(84166) > 0 then
            npc:Say("Suchun guards the blood we need. If you have his vial, place it on the shrine with the red shard.")
        elseif client:CountItem(84165) > 0 then
            if deathknell.completed_shared(client, 920305) then
                npc:Say("Place the Glowing Shard of Mana upon the shrine.")
            else
                npc:Say("Return once you can reach Sullon's tower by your own efforts.")
            end
        elseif client:CountItem(84164) > 0 then
            npc:Say("You have proved yourself strong enough for a " .. say_link("task") .. ".")
        end
        return
    end
    local line = shrine_dialogue[words:gsub("^what ", "")]
    if line then
        npc:Say(line[1] .. say_link(line[2]) .. line[3])
    elseif words == "willing" or words == "i am willing" or words == "i willing" then
        npc:Say("With the shard in your possession, stand in the ward beneath Sullon's tower. Bring the shard back to me.")
    elseif words == "help" or words == "i will continue to help" then
        npc:Say("Seek Suchun in the Root of Ro. The shard needs the Blood of Solusek.")
    end
end

local function eodue_ready(client)
    return client:CountItem(52601) > 0 and not client:HasZoneFlag(381)
        and deathknell.completed_shared(client, 920305) and client:IsTaskCompleted(920337)
end

function M.eodue(e)
    if not por.public(e.self, 380, 380000, 106958) then return end
    local words = por.words(e)
    if words == "hail" then
        e.self:Say("This place was filled with music before it fell to " .. say_link("ruin") .. ".")
    elseif words == "ruin" then
        e.self:Say("A great evil has overtaken my six " .. say_link("companions") .. ".")
    elseif words == "companions" then
        e.self:Say("The music of Jonthan, Tuyen, Rizlona, Denon, Cassindra and Ervaj gave them life. Now they suffer under a " .. say_link("curse") .. ".")
    elseif words == "curse" then
        e.self:Say("Their Inspirations may reveal the " .. say_link("source") .. " of the calamity.")
    elseif words == "source" and eodue_ready(e.other) then
        e.self:Say("Bring me an Inspiration from one of Ayonae's muses. Keep the Divine Fetters with you.")
    end
end

function M.eodue_trade(e)
    if por.public(e.self, 380, 380000, 106958) and eodue_ready(e.other) then
        for _, item in ipairs(inspiration) do
            if por.offers(e, item) and por.take(e, {[item]=1}) then
                e.other:SetZoneFlag(381)
                e.self:Say("Speak for Ayonae in Deathknell. The tower will admit you.")
                break
            end
        end
    end
    e.self:ReturnHandinItems(e.other)
end

return M
