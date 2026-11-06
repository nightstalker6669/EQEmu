-- NPC request routing and researched quest-supply dispatch. This module never
-- manufactures objective credit, pays scripted rewards or replaces missing scenes.
local D=require('achievement_quest_xorbb_data')
local phases=require('achievement_quest_xorbb_phase_data')
local M={}
local function yes(v)return v==true or v==1 end
local function requestable(c,id)
 if not require('achievement_quest_xorbb_exchange').requestable(c,id)then return false end
 if not require('achievement_quest_xorbb_rescue').requestable(c,id)then return false end
 if D.ready[id]~=true or c:GetLevel()<D.minimum_level or yes(c:IsTaskActive(id))
  or not yes(eq.is_task_appropriate(id))then return false end
 for _,prior in ipairs(D.tasks[id].prior)do if not yes(c:IsTaskCompleted(prior))then return false end end
 return true
end
function M.say(e)
 if not D.enabled or eq.get_zone_id()~=753 or e.self:GetNPCTypeID()~=D.giver then return false end
 local c=e.other
 if not c or not c.valid then return false end
 require('achievement_quest_xorbb_rescue').recover(e)
 local text=tostring(e.message or ''):lower()
 require('achievement_quest_xorbb_supplies').say(e)
 local selected
 for id=3040474,3040476 do
  local t=D.tasks[id]
  if text:find('%f[%a]'..t.keyword..'%f[%A]')then selected=id;break end
 end
 if not selected then
  if text:find('hail',1,true)then
   local choices={}
   for id=3040474,3040476 do
    if requestable(c,id)then
     local t=D.tasks[id];choices[#choices+1]=eq.say_link(t.keyword,false,t.title)
    end
   end
   if #choices>0 then
    -- Authored concise guidance; original quest prose is not copied.
    e.self:Say('Brantun and Barleyburr need help. Ask me about '..table.concat(choices,', ')..'.')
    return true
   end
  end
  return false
 end
 if not D.ready[selected] then
  c:Message(15,'This quest reconstruction is not enabled yet.')
  return true
 end
 if not requestable(c,selected)then return true end
 eq.task_selector({selected})
 return true
end
function M.accepted(e)
 local id=tonumber(e.task_id)
 if eq.get_zone_id()==753 and e.self and e.self.valid and e.self:GetNPCTypeID()==D.giver and id and D.tasks[id]and e.other and e.other.valid then
  require('achievement_quest_xorbb_ground').clear(e.other,id)
  require('achievement_quest_xorbb_ladder').accepted(e.other,id)
  require('achievement_quest_xorbb_bats').accepted(e.other,id)
  require('achievement_quest_xorbb_rescue').accepted(e.other,id)
  require('achievement_quest_xorbb_supplies').accepted(e.other,id)
  require('achievement_quest_xorbb_exchange').accepted(e.other,id)
  require('achievement_quest_xorbb_cooks').accepted(e.other,id)
 end
end
function M.closed(e)
 local id=tonumber(e.task_id)
 if id and D.tasks[id]then
  require('achievement_quest_xorbb_ground').clear(e.self,id)
  require('achievement_quest_xorbb_ladder').closed(e)
  require('achievement_quest_xorbb_bats').closed(e)
  require('achievement_quest_xorbb_crystals').closed(e)
  require('achievement_quest_xorbb_spirit').closed(e)
  require('achievement_quest_xorbb_shelters').closed(e)
  require('achievement_quest_xorbb_interactions').closed(e)
 end
end
function M.before_update(e)
 local id=tonumber(e.task_id)
 if not id or not D.tasks[id]then return 0 end
 -- A GM-forced disabled task must not advance on broad/unwired targets.
 if not D.enabled or D.ready[id]~=true then return 1 end
 local rescue=require('achievement_quest_xorbb_rescue').before_update(e)
 if rescue~=0 then return rescue end
 -- Camp/prison actors share NPC type IDs. Native NPC matching still checks
 -- identity; this additional player-position guard rejects the alternate scene
 -- even when TaskSystem:EnableTaskProximity is disabled.
 local area=phases[id]and phases[id][tonumber(e.activity_id)]
 if area then
  local c=e.self
  if eq.get_zone_id()~=753 or not c or not c.valid then return 1 end
  local x,y,z=c:GetX(),c:GetY(),c:GetZ()
  if not(x>=area.min_x and x<=area.max_x and y>=area.min_y and y<=area.max_y
   and z>=area.min_z and z<=area.max_z)then return 1 end
 end
 return require('achievement_quest_xorbb_supplies').before_update(e)
end
return M
