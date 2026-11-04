-- Researched quest supplies only. Completion/rewards remain native task-owned.
local D=require('achievement_quest_xorbb_data')
local I=require('achievement_quest_mesa_inventory')
local M={}
local task=3040475
local function yes(v)return v==true or v==1 end
local function active(c,a)
 return D.enabled==true and D.ready[task]==true and eq.get_zone_id()==753 and c and c.valid
  and yes(c:IsTaskActive(task))and yes(c:IsTaskActivityActive(task,a))
end
local function give(c,id)
 if I.count(c,id)>0 then return true end
 return I.give(c,id)
end
function M.accepted(c,id)
 if id==task and active(c,0)then give(c,66513)end
end
function M.say(e)
 local c=e.other
 if not e.self or not e.self.valid or e.self:GetNPCTypeID()~=D.giver then return end
 local text=tostring(e.message or ''):lower()
 if not(text:find('hail',1,true)or text:find('supplies',1,true))then return end
 if active(c,0)then give(c,66513)
 elseif active(c,1)then
  -- Dated Dec13,2012 report: poultice moved from initial assignment to the
  -- post-shelter Thistlebrush hail. Do not supply it at task acceptance.
  give(c,66515)
 elseif active(c,8)then give(c,66518)
 elseif active(c,10)then require('achievement_quest_xorbb_exchange').hail(e)end
end
function M.before_update(e)
 if tonumber(e.task_id)~=task then return 0 end
 local a=tonumber(e.activity_id)
 if a==10 then
  if not e.self or not e.self.valid then return 1 end
  return require('achievement_quest_xorbb_exchange').ready(e.self)and 0 or 1
 end
 local item=({[1]=66515,[8]=66518})[a]
 if not item then return 0 end
 if not e.self or not e.self.valid then return 1 end
 if not active(e.self,a)or I.count(e.self,item)==0 then
  e.self:Message(13,'Empty your cursor and hail Thistlebrush again to receive the quest supply.')
  return 1
 end
 return 0
end
return M
