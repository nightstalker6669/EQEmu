local work = {
    [703038] = { { 920406, 920407, 920408 }, "The beds are telling us three different stories. Help me work out which one is true." },
    [705003] = { { 920409, 920410, 920411 }, "If anyone trapped down here is still listening, I want to know they can reach the way out." },
    [704000] = { { 920412, 920413, 920414 }, "A catalogue helps only if someone can walk the shelves and prove it right." },
    [706010] = { { 920432, 920433 }, "Count the people we can still reach. Then we will know which reports to trust." },
    [706047] = { { 920434, 920435 }, "A fire cannot hold a street by itself. I need someone to find out what keeps bringing it back." },
    [706048] = { { 920415 }, "The port has given me four accounts. Bring me something they agree on." },
    [702012] = { { 920416, 920417, 920418, 920419, 920420 }, "I have five accounts of the same night and none agrees with the next. Help me sort the evidence." },
    [709002] = { { 920421 }, "I can still draw the lanes from memory. What I need is someone to walk them today." },
    [709036] = { { 920422 }, "A trail marked in snow lasts until the next wind. Let us mark ours with something stronger." },
    [709076] = { { 920423 }, "The boundary has moved again. The trees are no use as witnesses, so I am asking you." },
    [710047] = { { 920424 }, "Steel has a sound of its own. There is a noise in the lower garden that is almost like it." },
    [710017] = { { 920425 }, "I can see the garden from here. I cannot tell what is growing in it anymore." },
    [710068] = { { 920426 }, "The guards move when the upper bell rings. If you can keep them busy, I can make myself heard." },
    [710035] = { { 920427 }, "Someone keeps moving my trail markers. I am out of jokes about it." },
    [710071] = { { 920428 }, "A voice on the far side asks you to test the threshold before you step across it." },
    [711012] = { { 920429 }, "Something has learned to hunt beside the road. I would like to learn faster." },
    [711009] = { { 920430, 920431 }, "Nobody left a map for the living. Bring me landmarks that will still be there tomorrow." },
}

local M = {}

function M.say(e)
    local entry = work[e.self:GetNPCTypeID()]
    if not entry or not e.other.valid then
        return false
    end

    local message = e.message:lower()
    if message ~= "fieldwork" and not message:match("^hail") then
        return false
    end

    local available = {}
    for _, task in ipairs(entry[1]) do
        if e.other:CanRequestHouseOfThulePrerequisite(task, e.self:GetID()) then
            available[#available + 1] = task
        end
    end

    if message == "fieldwork" then
        if #available == 0 then
            e.self:Say("I have no fieldwork for you right now.")
        else
            eq.task_selector(available)
        end
        return true
    end

    if #available > 0 then
        e.self:Say(entry[2] .. " Ask about " .. eq.say_link("fieldwork", false, "fieldwork") .. ".")
    end
    return false
end

return M
