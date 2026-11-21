local por = require("rof2_por_progression")
local scene = require("rof2_por_scene_state")
local M = {}
local task_meta = {
    [10]={ task=920300, name="library", arrival={-912,-136.3,8,510}, door=4 },
    [11]={ task=920301, name="oubliette", arrival={0,-649,5,0}, door=1 },
    [12]={ task=920302, name="laboratory", arrival={544,-49,5,0}, door=3 },
}

-- The shipped room exits ATP_1, ATP_9 and ATP_8 return to the lobby.
-- Use the floor side of each portal so client collision cannot hide the trigger.
local return_portals = {
    [10]={ bounds={-919,-905,-145,-137.5,0,15}, landing={-58,33,122,172} },
    [11]={ bounds={-8,8,-647,-642,0,15}, landing={83,0,4,385} },
    [12]={ bounds={536,552,-59,-54.5,0,15}, landing={0,-65,170,506} },
}
local return_area, return_timer = 920312, "por_skylance_return"

local steps = {
    {20, "Remove the necrotic tissue."},
    {8, "Infuse the egg with the Trans-Injector."},
    {17, "Call to the willing spirits."},
    {9, "Apply the ethereal goo."},
    {19, "Use the Dispenser."},
    {11, "Align the proteins with the Hyper-Manipulator."},
    {18, "Distill the magical spirits."},
    {10, "Confer sight with the Gaze-O-Rama."},
}
local egg_table = { client_id=12, x=544.208,y=-0.098119 }
local incubated_egg, prototype_egg = 36145, 36143
-- Sorted v11 spawn2 and NPC type pairs in the r2 layout. A quest reload
-- can happen while these NPCs are already alive, so spawn callbacks cannot
-- be used to build the candidate roster.
local eggsacks = {
    {304080528, 371013},
    {304080530, 371014},
    {304080533, 371014},
    {304080534, 371013},
    {304080535, 371016},
    {304080536, 371016},
    {304080553, 371014},
    {304080554, 371013},
    {304080555, 371013},
    {304080557, 371016},
    {304080558, 371013},
    {304080559, 371016},
    {304080565, 371016},
    {304080566, 371016},
    {304080567, 371013},
    {304080568, 371013},
    {304080569, 371014},
    {304080591, 371016},
    {304080592, 371016},
    {304080595, 371016},
    {304080597, 371014},
    {304080598, 371014},
    {304080609, 371016},
    {304080610, 371014},
    {304080611, 371013},
    {304080612, 371013},
    {304080613, 371013},
}
local eggsack_types = {}
for _, egg in ipairs(eggsacks) do eggsack_types[egg[1]] = egg[2] end

local function whole_room(message)
    for client in eq.get_entity_list():GetClientList().entries do
        if por.inside(client,920302,371,12) then client:Message(13,message) end
    end
end

local function mob(npc_type, x, y, z, heading, enemy)
    local spawned = eq.spawn2(npc_type,0,0,x,y,z,heading)
    if spawned and spawned.valid and spawned:IsNPC() then
        local npc = spawned:CastToNPC()
        if enemy then npc:AddToHateList(enemy,1) end
        return npc
    end
end

function M.arrive(client, version)
    local meta = task_meta[version]
    if por.current(371,version) and eq.get_zone():GetInstanceID() > 0 then
        eq.add_area(return_area,1,unpack(return_portals[version].bounds))
    end
    if por.inside(client,meta.task,371,version) then
        por.advance(client,meta.task,0)
    end
end

function M.return_area(event, version)
    if event.area_id == return_area and por.current(371,version) and
        eq.get_zone():GetInstanceID() > 0 then
        -- Finish processing the client's movement update before moving within the instance.
        eq.set_timer(return_timer,100)
    end
end

function M.return_timer(event, version)
    if event.timer ~= return_timer then return end
    eq.stop_timer(return_timer)
    local instance = eq.get_zone():GetInstanceID()
    if instance > 0 and por.current(371,version) then
        local x,y,z,h = unpack(return_portals[version].landing)
        event.self:MovePCInstance(371,instance,x,y,z,h)
    end
end

function M.arkahn(event, version)
    local meta = task_meta[version]
    if event.self:GetNPCTypeID() ~= 371002 or not por.current(371,version) then return end
    local phrase = por.words(event)
    if phrase == "leave" then
        event.other:MovePC(369,-957,241,409,0)
        return
    end
    if not por.inside(event.other,meta.task,371,version) then return end
    local client=event.other
    if version==11 and phrase=="recover the prototype egg" and client:IsTaskActivityActive(920301,3) then
        local issued,character=scene.read(client,920301,371,11,0)
        if issued==1 and character==client:CharacterID() and client:CountItem(prototype_egg)==0 then
            client:SummonItem(prototype_egg)
        end
        return
    end
    if phrase == "hail" or phrase == meta.name or phrase == "the "..meta.name then
        event.self:Say("The portal to the " .. meta.name .. " is ahead. Use it to continue.")
    end
end

local function lab_boss()
    for npc in eq.get_entity_list():GetNPCList().entries do
        if npc.valid and npc:GetNPCTypeID()==371963 and npc:GetHP()>0 then return npc end
    end
    return nil
end

function M.replay(client,version)
    local task=task_meta[version].task
    if not por.inside(client,task,371,version) then return end
    if version==11 and client:IsTaskActivityActive(task,2) then
        if scene.read(client,task,371,11,0)==1 then por.advance(client,task,2) end
    elseif version==12 then
        if not (client:IsTaskActivityActive(task,2) or client:IsTaskActivityActive(task,3) or
                 client:IsTaskActivityActive(task,4)) then return end
        local durable=scene.read(client,task,371,12,0)
        if not durable or durable==0 then return end
        if client:IsTaskActivityActive(task,2) then
            por.advance(client,task,2)
        elseif durable==9 then
            if lab_boss() or mob(371963,egg_table.x,egg_table.y,5,0,client) then
                por.advance(client,task,3)
            end
        end
    end
end

local function experiment_door(event,id)
    local client=event.self
    local task=920302
    if id==egg_table.client_id and event.door:GetDoorDBID()==304081291 then
        local durable,character=scene.read(client,task,371,12,0)
        if durable==false and client:IsTaskActivityActive(task,2) and client:GetItemIDAt(Slot.Cursor)==incubated_egg then
            if not scene.start(client,task,371,12,0,0) then return 1 end
            durable,character=scene.read(client,task,371,12,0)
        end
        if durable==0 and character==client:CharacterID() then
            if client:GetItemIDAt(Slot.Cursor)==incubated_egg then
                client:DeleteItemInInventory(Slot.Cursor,1,true)
            end
            if client:CountItem(incubated_egg)>0 or not scene.egg_debited(client) then
                client:Message(15,"Move the incubated egg onto your cursor before continuing.")
                return 1
            end
            if scene.advance(client,task,371,12,0,1) then
                por.advance(client,task,2)
                for _,point in ipairs({{524,-16},{563,-16},{544,20}}) do
                    mob(371961,point[1],point[2],5,0,client)
                end
                whole_room(steps[1][2])
            end
        elseif durable and steps[durable] then
            whole_room(steps[durable][2])
        end
        return 1
    end
    if id<8 or id>20 then return 0 end
    if not client:IsTaskActivityActive(task,3) then return 1 end
    local durable=scene.read(client,task,371,12,0)
    local expected=durable and steps[durable]
    if not expected then return 1 end
    if id~=expected[1] then
        if durable==1 or scene.advance(client,task,371,12,durable,1) then
            mob(371962,client:GetX(),client:GetY(),client:GetZ(),0,client)
            whole_room("The equipment hisses. Start again. " .. steps[1][2])
        end
        return 1
    end
    if scene.advance(client,task,371,12,durable,durable+1) then
        mob(371961,client:GetX(),client:GetY(),client:GetZ(),0,client)
        if steps[durable+1] then whole_room(steps[durable+1][2])
        else M.replay(client,12) end
    end
    return 1
end

function M.door(event, version)
    local meta=task_meta[version]
    local client,door=event.self,event.door
    if not por.inside(client,meta.task,371,version) then return 0 end
    local num=door:GetDoorID()
    if version==10 and num==1 then
        client:MovePCInstance(371,eq.get_zone():GetInstanceID(),-464,0,5,0)
        return 1
    end
    if num==meta.door then
        local x,y,z,h=unpack(meta.arrival)
        client:MovePCInstance(371,eq.get_zone():GetInstanceID(),x,y,z,h)
        por.advance(client,meta.task,1)
        return 1
    end
    if version==12 then return experiment_door(event,num) end
    return 0
end

function M.joban(e)
    if por.inside(e.other,920300,371,10) and por.words(e)=="hail" then
        if por.advance(e.other,920300,2) then e.self:Say("The assistants each hold half of the Skylance token.") end
    end
end

function M.librarian(e)
    if por.inside(e.other,920300,371,10) then
        if e.other:IsTaskActivityActive(920300,5) and e.other:CountItem(36142)==0 and
            por.take(e,{[36140]=1,[36141]=1}) then
            e.other:SummonItem(36142)
            por.advance(e.other,920300,5,2)
        end
    end
    e.self:ReturnHandinItems(e.other)
end

function M.egg_death(e)
    local client = por.player(e.other)
    local npc = e.killed
    if not client or not npc or not npc.valid or not por.inside(client,920301,371,11) or
        not client:IsTaskActivityActive(920301,2) then return end

    local placement = npc:GetSpawnPointID()
    if eggsack_types[placement] ~= npc:GetNPCTypeID() then return end
    local identity = client:GetSharedTaskIdentity()
    if not identity:match("^[1-9]%d*$") then return end
    local index = 0
    for digit in identity:gmatch("%d") do
        index = (index * 10 + tonumber(digit)) % #eggsacks
    end

    if eggsacks[index+1][1] ~= placement then
        mob(371960,npc:GetX(),npc:GetY(),npc:GetZ(),npc:GetHeading(),client)
        return
    end
    if scene.start(client,920301,371,11,0,1) then
        if client:CountItem(prototype_egg)==0 then client:SummonItem(prototype_egg) end
        por.advance(client,920301,2)
    elseif scene.read(client,920301,371,11,0) ~= 1 then
        eq.debug("Skylance prototype eggsack death reached the scene writer, but state could not be recorded for shared task "..identity)
    end
end

function M.assistant_combat(e)
    if not e.joined then eq.stop_timer("por_library_add"); return end
    eq.set_timer("por_library_add",20000)
end

function M.assistant_add(e,npc_type)
    if e.timer~="por_library_add" then return end
    local npc=e.self
    local target=npc:GetTarget()
    if target and target.valid then mob(npc_type,npc:GetX()+4,npc:GetY(),npc:GetZ(),0,target) end
end

return M
