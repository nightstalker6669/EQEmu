-- Authored quest routing and actual-kill supply production, disabled pending
-- complete scenes, native acceptance and coordinated transactional installation.
local D=require('achievement_quest_xorbb_mercenary_data')
local I=require('achievement_quest_mesa_inventory')
local M={}
local function yes(v)return v==true or v==1 end
local function enabled(c,t)
 return D.enabled==true and D.ready[t]==true and eq.get_zone_id()==753
  and c and c.valid and c:IsClient()
end
local function active(c,t,a)
 return enabled(c,t)and yes(c:IsTaskActive(t))and yes(c:IsTaskActivityActive(t,a))
end
local function token(message,phrase)
 return (' '..tostring(message or ''):lower()..' '):find('%f[%a]'..phrase..'%f[%A]')~=nil
end
function M.say(e)
 local n,c=e.self,e.other
 if not n or not n.valid or not c or not c.valid or not D.enabled or eq.get_zone_id()~=753 then return false end
 if require('achievement_quest_xorbb_golem').say(e)or require('achievement_quest_xorbb_gronk').say(e)then return true end
 if require('achievement_quest_xorbb_mercenary_dialogue').say(e)then return true end
 require('achievement_quest_xorbb_mercenary_supplies').say(e)
 local choices={};local requested=false
 for _,t in ipairs(D.tasks)do
  if t.giver==n:GetNPCTypeID()then
   if token(e.message,t.phrase)then requested=true end
   if enabled(c,t.id)and c:GetLevel()>=D.minimum_level and not yes(c:IsTaskActive(t.id))
    and yes(eq.is_task_appropriate(t.id))then choices[#choices+1]=t.id end
  end
 end
 if requested then if #choices>0 then eq.task_selector(choices)end;return true end
 if token(e.message,'hail')and #choices>0 then
  local links={};local seen={}
  for _,t in ipairs(D.tasks)do if t.giver==n:GetNPCTypeID()and D.ready[t.id]and not seen[t.phrase]then
   seen[t.phrase]=true;links[#links+1]=eq.say_link(t.phrase,false,t.phrase)
  end end
  n:Say('There is work to do in the valley. Ask me about '..table.concat(links,', ')..'.')
 end
 return false
end
function M.merit(e)
 -- Called only by native EVENT_KILLED_MERIT on the actual eligible corpse.
 -- Kill/autogrant activities use manual goalmethod2 so an inventory failure
 -- cannot first advance the native kill counter without delivering its proof.
 local n,c=e.self,e.other
 if not n or not n.valid or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 for _,t in ipairs(D.autogrants)do
  if active(c,t.id,0)and t.npcs[n:GetNPCTypeID()]then
   local key='rof2.xorbb.mercenary.proof.'..t.id..'.'..c:CharacterID()
   -- One corpse/character/quest can supply one proof. Pending is sticky when a
   -- partial mutation becomes ambiguous; no automatic replay or invented credit.
   if n:GetEntityVariable(key)==''then
    local before=I.count(c,t.item)
    if c:GetInventory():GetItem(Slot.Cursor).valid then
     c:Message(13,'No quest proof was awarded because your cursor was occupied.');return
    end
    n:SetEntityVariable(key,'pending')
    if not I.give(c,t.item,1)or I.count(c,t.item)~=before+1 then
     c:Message(13,'Quest proof could not be verified; no progress was granted.');return
    end
    n:SetEntityVariable(key,'awarded')
    c:UpdateTaskActivity(t.id,0,1)
   end
  end
 end
end
function M.before_update(e)
 local t=tonumber(e.task_id)
 if not D.owned[t]then return 0 end
 -- No accidental progress even if a GM assigns an unfinished disabled quest.
 if not enabled(e.self,t)then return 1 end
 return require('achievement_quest_xorbb_mercenary_dialogue').before_update(e)
end
function M.accepted(e)
 local id=tonumber(e.task_id);local c=e.other
 if not id or not D.owned[id]or not enabled(c,id)or not yes(c:IsTaskActive(id))then return false end
 local giver
 for _,t in ipairs(D.tasks)do if t.id==id then giver=t.giver;break end end
 if not e.self or not e.self.valid or e.self:GetNPCTypeID()~=giver then return false end
 local key='rof2.xorbb.mercenary.run.'..id
 local raw=c:GetBucket(key)
 if raw~=''and not tostring(raw):match('^[1-9]%d*$')then return false end
 local prior=tonumber(raw)or 0
 if prior<0 or prior~=math.floor(prior)or prior>=9007199254740990 then return false end
 local value=string.format('%.0f',prior+1);c:SetBucket(key,value)
 if c:GetBucket(key)~=value then return false end
 -- Retain the monotonically increasing run counter across completion/cancel;
 -- repeat quests must not reuse a prior shared-actor duplicate guard.
 return require('achievement_quest_xorbb_mercenary_supplies').accepted(e)
end
return M
