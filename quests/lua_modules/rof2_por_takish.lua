local por=require("rof2_por_progression")
local M={}
local key_task, prince_task, message_task=920307,920308,920309
local prince_seen
local tablet_kills=0

local function on_tak_route(client)
    if client:CountItem(84168)>0 then return true end
    for task=key_task,message_task do
        if client:IsTaskActive(task) or client:IsTaskCompleted(task) then return true end
    end
    return false
end

function M.stay_in_modern_ro(client, zone)
    if not por.current(zone,0,0) then return false end
    if on_tak_route(client) then return true end

    -- The group missions do not require every participant to carry a chime.
    -- An arriving companion can join a qualified group member already here.
    local group=client:GetGroup()
    if group.valid then
        for i=0,5 do
            local member=group:GetMember(i)
            if member.valid and member:IsClient() and on_tak_route(member:CastToClient()) then
                return true
            end
        end
    end
    return false
end

function M.used_public_tree(e)
    if por.current(376,0,0) and e.door:GetDoorDBID()==16844 and e.door:GetDoorID()==1 and
        not e.self:IsTaskActivityActive(key_task,0) then
        if por.enter(e.self,key_task,377,10,18,-138,-29,0) then return 1 end
    end
    return 0
end

function M.arrive(client, task)
    local zone,version=377,10
    if task==message_task then zone,version=376,11 end
    if por.inside(client,task,zone,version) then por.advance(client,task,1) end
end

function M.key_death(e)
    local npc,corpse=e.killed,e.corpse
    local client=por.player(e.other)
    if not client or not corpse or not corpse.valid or not npc.valid or not por.inside(client,key_task,377,10) then return end
    local npc_id=npc:GetNPCTypeID()
    if npc_id<377000 or npc_id>377015 or npc:GetSpawnPointID()==0 then return end
    tablet_kills=tablet_kills+1
    if tablet_kills%2==0 then
        local item=36135+(math.floor(tablet_kills/2)-1)%4
        if client:IsTaskActivityActive(key_task,item-36135+2) and corpse:CountItem(item)==0 then
            corpse:AddItem(item,1)
            -- The empty corpse timer was selected before this callback.
            if corpse:CountItem(item)==1 then corpse:ResetDecayTimer() end
        end
    end
end

function M.prince_tick(client)
    if prince_seen or not client:IsTaskActivityActive(prince_task,2) or
        not por.inside(client,prince_task,376,10) then return end
    local prince=eq.spawn2(376961,0,0,-310,18,3,0)
    if not prince.valid then return end
    prince_seen=true
    for _,point in ipairs({{-303,25},{-318,10}}) do
        eq.spawn2(376965,0,0,point[1],point[2],3,0)
    end
end

function M.private_exit(e,task)
    local client=e.self
    local egress=e.door:GetDoorID()
    if task==message_task and egress==3 and por.inside(client,task,376,11) then
        client:MovePC(378,-174,200,88,0)
        return 1
    end
    if task==prince_task and egress==3 and por.inside(client,task,376,10) then
        client:MovePC(392,-2073,5946,35.125,0)
        return 1
    end
    if task==key_task and egress==1 and por.inside(client,task,377,10) then
        client:MovePC(376,-553,735,225,0)
        return 1
    end
    return 0
end
return M
