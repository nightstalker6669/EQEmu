-- Required quest implements only. Never grant collected materials, results,
-- fabricated encounter loot, objectives, rewards or prior-stage completion.
local D=require('achievement_quest_xorbb_mercenary_data')
local I=require('achievement_quest_mesa_inventory')
local M={}
local function yes(v)return v==true or v==1 end
local rules={
 {task=3040483,giver=753001,activities={0,1,2},items={{66525,-1}}},
 {task=3040484,giver=753114,activities={0},items={{68576,1}}},
 {task=3040484,giver=753114,activities={4},items={{68573,1}}},
 {task=3040484,giver=753114,activities={7},items={{68577,1}}},
 {task=3040484,giver=753114,activities={11},items={{68574,1}}},
 {task=3040484,giver=753114,activities={14},items={{68578,1}}},
 {task=3040484,giver=753114,activities={17},items={{68575,1},{68579,1}}},
 {task=3040484,giver=753114,activities={18},items={{68579,1}}},
 {task=3040485,giver=753030,activities={2},items={{68486,1}}},
 {task=3040485,giver=753030,activities={8},coffin_gate=true,items={{68480,1}}},
 {task=3040485,giver=753030,activities={11},items={{68487,1}}},
 {task=3040487,giver=753034,activities={8},items={{17750,1}}}
}
local function allowed(c,r,giver)
 if not D.enabled or D.ready[r.task]~=true or eq.get_zone_id()~=753
  or not c or not c.valid or giver~=r.giver or not yes(c:IsTaskActive(r.task))then return false end
 if r.coffin_gate then
  local run=c:GetBucket('rof2.xorbb.mercenary.run.'..r.task)
  if not tostring(run):match('^[1-9]%d*$')or c:GetBucket('rof2.xorbb.mercenary.coffin_run')~=run then return false end
 end
 for _,a in ipairs(r.activities)do if yes(c:IsTaskActivityActive(r.task,a))then return true end end
 return false
end
function M.recover(c,giver)
 for _,r in ipairs(rules)do if allowed(c,r,giver)then
  for _,item in ipairs(r.items)do
   if I.count(c,item[1])==0 then
    -- Existing native Lore/cursor/charge restrictions are retained. No queued
    -- cursor manipulation or bank deletion to manufacture a replacement.
    if not I.give(c,item[1],item[2])then return false end
   end
  end
 end end
 return true
end
function M.accepted(e)
 if e.self and e.self.valid and e.other and e.other.valid then
  return M.recover(e.other,e.self:GetNPCTypeID())
 end
 return false
end
function M.say(e)
 if not e.self or not e.self.valid or not e.other or not e.other.valid then return false end
 local text=tostring(e.message or ''):lower()
 if text:find('hail',1,true)or text:find('%f[%a]supplies%f[%A]')then
  return M.recover(e.other,e.self:GetNPCTypeID())
 end
 return false
end
return M
