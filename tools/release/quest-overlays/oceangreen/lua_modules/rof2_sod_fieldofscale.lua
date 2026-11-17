-- Original requester phrases. Native admission/history and inventory writers
-- own prerequisites/turn-ins; these dialogue routes never grant victory.
local M={}
local tasks={Drake=920197,Food=920198,Illusion=920199,Raising=920200,
 SarnakSokokar=920201,SarnakBurynai=920202,SarnakKurn=920203,Supplies=920204,
 Afoot=920205,Idle=920206,Purify=920207,Skull=920208,Lost=920209,Parley=920210,
 Audience=920211,Brood=920212,Knowledge=920213,Phalanx=920214,Shield=920215}
local sets={SeverilousFood=11,SeverilousDrake=12,PharaIllusion=13,
 TrakanonSupplies=14,JaledGroup=15,MessageRaising=16,MessageSarnak=17}
M.tasks=tasks
M.tasksets=sets
local function base()
 local v=tostring(eq.get_rule('SodProgression:EnableFieldOfScaleSlice')):lower()
 local z=eq.get_zone()
 return (v=='true' or v=='1') and z:GetZoneID()==452 and
  z:GetInstanceID()==0 and z:GetInstanceVersion()==0
end
local function phrase(e,p)
 return (e.message or ''):lower():find(p,1,true)~=nil
end
local function one(e,key)
 return e.other:TaskSelectorFromNPC(e.self,{tasks[key]})
end
-- Native World sends the NPC acceptance context ONLY to the actual requester
-- (shared_task_manager.cpp SendAcceptNewSharedTaskPacket loop). Other initial
-- members and late joins receive NPC0, so this actual NPC event grants only the
-- requester's original reusable quest crystal, never all six participants.
function M.accepted(e)
 if not base() or e.self:GetNPCTypeID()~=452147 or e.task_id~=tasks.Knowledge or
  not e.other.valid or not e.other:IsTaskActive(tasks.Knowledge) then return end
 if e.other:CountItem(17345)==0 then e.other:SummonItem(17345) end
end
local function refresh_illusion(c)
 if not c:IsTaskActive(tasks.Illusion) then return end
 -- Four actual native subtask completions, not a hail/reward/counter proxy.
 -- This also retains earned component work if the grand task was abandoned
 -- and re-requested; nonrepeatable parts must never need synthetic re-grants.
 for _,part in ipairs({{tasks.Purify,0},{tasks.Idle,1},{tasks.Afoot,2},{tasks.Skull,3}}) do
  if c:IsTaskActivityActive(tasks.Illusion,part[2]) and
   c:GetSodTaskCompletionState(part[1])==1 then
   c:UpdateTaskActivity(tasks.Illusion,part[2],1)
  end
 end
end
-- Exact actor wrappers are emitted only from the NPC owner's bound roles.
-- Provider binding is an argument, never a client-supplied name-match.
function M.say(e,role,expected_id)
 if not base() or not expected_id or e.self:GetNPCTypeID()~=expected_id then return end
 if role=='Jaled' then
  if phrase(e,'still available') then
   -- This set contains five GROUP missions only. Lost and Parley retain
   -- their individual original request phrases and native shared selectors.
   eq.task_set_selector(sets.JaledGroup)
  elseif phrase(e,'investigate') then one(e,'Lost')
  elseif phrase(e,'parley') then one(e,'Parley')
  elseif phrase(e,'hail') then e.self:Say('I have enough [problems] to deal with here.')
  elseif phrase(e,'problems') then
   local lost=e.other:GetSodTaskCompletionState(tasks.Lost)
   local parley=e.other:GetSodTaskCompletionState(tasks.Parley)
   if lost==1 and parley==1 then
    e.self:Say('Is your assistance [still available] to us?')
   elseif lost==1 and parley==0 then
    e.self:Say('Will you attempt to [parley] with Ganak?')
   elseif lost==0 then
    local count=0
    for _,id in ipairs({tasks.Drake,tasks.Food,tasks.Illusion,tasks.Raising,
     tasks.SarnakSokokar,tasks.SarnakBurynai,tasks.SarnakKurn,tasks.Supplies}) do
     local credit=e.other:GetSodTaskCompletionState(id)
     if credit~=0 and credit~=1 then return end
     if credit==1 then count=count+1 end
    end
    if count>=4 then
     -- Native acceptance rechecks fresh acknowledged requester history.
     e.self:Say('One of my kin has not returned. Will you [investigate]?')
    else e.self:Say('You have not yet proven yourself to the Ring of Scale.') end
   end
  end
 elseif role=='Severilous' then
  if phrase(e,'feed') then eq.task_set_selector(sets.SeverilousFood)
  elseif phrase(e,'kill some') then eq.task_set_selector(sets.SeverilousDrake)
  elseif phrase(e,'hail') or phrase(e,'offer') then
   e.self:Say('Go [kill some] of those fool Iksar, or help [feed] my racnar.')
  end
 elseif role=='Trakanon' then
  if phrase(e,'illusion') then require('rof2_sod_fieldofscale_public').illusion(e)
  elseif phrase(e,'useful') then eq.task_set_selector(sets.TrakanonSupplies)
  elseif phrase(e,'hail') or phrase(e,'kurn') then
   e.self:Say('The mighty Kurn hides inside his tower. You might prove [useful] to me.')
  end
 elseif role=='Ganak' then
  require('rof2_sod_fieldofscale_public').ganak(e)
 elseif role=='Talendor' then
  -- Original spoken keyword, not the generic native hail matcher.
  if phrase(e,'ribcage') and e.other:IsTaskActivityActive(tasks.Purify,2) then
   e.other:UpdateTaskActivity(tasks.Purify,2,1)
  elseif phrase(e,'hail') and e.other:IsTaskActive(tasks.Purify) then
   e.self:Say('Tell me about the [impure ribcage].')
  end
 elseif role=='Phara' then
  if phrase(e,'hail') then refresh_illusion(e.other) end
  if phrase(e,'see this through') then eq.task_set_selector(sets.PharaIllusion)
  elseif phrase(e,'ribcage') then one(e,'Purify')
  elseif phrase(e,'foot') then one(e,'Afoot')
  elseif phrase(e,'hand') then one(e,'Idle')
  elseif phrase(e,'skull') then one(e,'Skull')
  elseif phrase(e,'hail') or phrase(e,'illusion') then
   e.self:Say('Can you [see this through]? I will need a [ribcage], a [foot], a [hand], and a [skull].')
  end
 end
end
-- The original physical entry door chooses the already-owned native shared
-- mission. No dialogue teleport, invented location, or new private instance.
function M.door(e)
 if not base() or not e.self.valid or not e.door.valid then return 0 end
 local d=require('rof2_sod_fieldofscale_data')
 if not d.ready then return 0 end
 local entry=(d.entrances or {})[e.door:GetDoorDBID()]
 if not entry or e.door:GetDoorID()~=entry.door_id then return 0 end
 local selected
 for _,b in ipairs(entry.missions) do
  if e.self:IsTaskActive(b.task) then
   if selected then return 1 end -- never choose between conflicting runs
   selected=b
  end
 end
 if not selected then
  e.self:SodFieldOfScaleEnterMission(0,e.door:GetDoorDBID(),e.door:GetDoorID())
  return 1 -- restored original public dungeon route for sidequests
 end
 local instance=e.self:GetSharedTaskInstanceID(selected.task,selected.zone,selected.version)
 if instance==0 then return 1 end
 -- Native MovePC uses the DZ's bound entry position and performs fresh
 -- MissionEntry membership, occurrence, zone/version and instance checks.
 e.self:SodFieldOfScaleEnterMission(selected.task,e.door:GetDoorDBID(),e.door:GetDoorID())
 return 1 -- consume original click; do not also follow the ordinary base door
end
return M
