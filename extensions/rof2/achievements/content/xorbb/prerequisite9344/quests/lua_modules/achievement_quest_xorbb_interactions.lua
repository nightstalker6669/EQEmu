-- Native door interactions and physical ham sources. No area-only item credit.
local D=require('achievement_quest_xorbb_data')
local G=require('achievement_quest_xorbb_ground')
local I=require('achievement_quest_mesa_inventory')
local M={}
local timer='rof2_xorbb_ground_refresh'
local failure='rof2.xorbb.ground.failure'
local function yes(v)return v==true or v==1 end
local function enabled(c)
 return D.enabled==true and D.ready[3040474]==true and eq.get_zone_id()==753 and c and c.valid
  and yes(c:IsTaskActive(3040474))
end
local function active(c,a)return enabled(c)and yes(c:IsTaskActivityActive(3040474,a))end
local hams={{'filthy_ham',10},{'dirty_ham',11},{'dusty_ham',12},{'fresh_ham',13}}
local function place(c,key)
 if c:GetEntityVariable(failure)~=''then return false end
 local id,why=G.place(c,key)
 if not id then
  c:SetEntityVariable(failure,tostring(why))
  c:Message(13,'The Xorbb quest object could not be verified. Please report this before retrying.')
  return false
 end
 return true
end
function M.refresh(e)
 local c=e.self
 if not c or not c.valid then return end
 c:StopTimer(timer)
 if not enabled(c)or c:GetEntityVariable(failure)~=''then return end
 for _,rule in ipairs(hams)do
  if active(c,rule[2])then
   -- Each source is an actual ground item. A timer merely restores an expired
   -- source; only native successful pickup can update its Loot objective.
   if place(c,rule[1])then c:SetTimer(timer,D.ground_refresh_seconds)end
   return
  end
 end
end
function M.timer(e)
 if e.timer~=timer then return false end
 M.refresh(e);return true
end
function M.accepted(e)
 if tonumber(e.task_id)~=3040474 or not e.self or not e.self.valid then return end
 e.self:SetEntityVariable(failure,'')
 M.refresh(e)
end
function M.stage(e)
 if tonumber(e.task_id)==3040474 then M.refresh(e)end
end
function M.closed(e)
 if tonumber(e.task_id)==3040474 and e.self and e.self.valid then e.self:StopTimer(timer)end
end
local doors={
 [36447]={native=4,model='OBJ_CAVE_ROCK_A',stages={1},credit=1},
 [36451]={native=8,model='OBJ_LADDER',stages={6,7},item=66508,credit=6},
 [36452]={native=9,model='OBJ_CAVE_ROCK_A',stages={9},ground='waterfall_note'},
 [36453]={native=10,model='OBJ_POTC',stages={16},item=66507,credit=16}
}
function M.door(e)
 local c,o=e.self,e.door
 if not enabled(c)or not o or not o.valid then return 0 end
 local r=doors[o:GetDoorDBID()]
 if not r or o:GetDoorID()~=r.native or o:GetDoorName()~=r.model then return 0 end
 local stage
 for _,a in ipairs(r.stages)do if active(c,a)then stage=a;break end end
 if not stage then return 0 end
 local dx,dy,dz=c:GetX()-o:GetX(),c:GetY()-o:GetY(),c:GetZ()-o:GetZ()
 if not(dx*dx+dy*dy<=D.door_interaction_xy^2 and math.abs(dz)<=D.door_interaction_z)then return 1 end
 if r.ground then place(c,r.ground);return 1 end
 if not r.item then
  -- Actual rock investigation, not a radius timer or invented minotaur summon.
  c:Message(15,'The hidden note is missing. Search for the minotaur who took it.')
  c:UpdateTaskActivity(3040474,r.credit,1)
  return 1
 end
 -- The ordinary clicked ladder/pot supplies the researched item. Empty cursor
 -- and native readback are required before crediting the actual door action.
 if I.count(c,r.item)==0 and not I.give(c,r.item)then return 1 end
 if I.count(c,r.item)>0 and stage==r.credit then
  c:UpdateTaskActivity(3040474,r.credit,1)
  if r.item==66508 then require('achievement_quest_xorbb_group').ladder(c,o)end
 end
 return 1 -- Do not animate/move the shared decorative source for another player.
end
return M
