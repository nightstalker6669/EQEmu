-- Actual nearby duck -> army dialogue -> exactly three owned native attackers.
-- Kill progress comes only from their native merit callbacks, never a timer.
local D=require('achievement_quest_xorbb_mercenary_data')
local S=require('achievement_quest_xorbb_scenes_data')
local P=S.gronk
local G=require('achievement_quest_xorbb_scene_geometry')
local M={};local NS='rof2.xorbb.gronk.v1'
local function yes(v)return v==true or v==1 end
local function enabled()return D.enabled==true and D.ready[P.task]==true and S.profiles_ready==true and eq.get_zone_id()==753 end
local function active(c,a)return enabled()and c and c.valid and yes(c:IsTaskActive(P.task))and yes(c:IsTaskActivityActive(P.task,a))end
local function run(c)return c:GetBucket('rof2.xorbb.mercenary.run.'..P.task)end
local function nearby(c,n)
 local dx,dy,dz=c:GetX()-n:GetX(),c:GetY()-n:GetY(),c:GetZ()-n:GetZ()
 return dx*dx+dy*dy<=P.owner_xy^2 and math.abs(dz)<=P.owner_z and c:CheckLoS(n)
end
local function tagged(n)return n and n.valid and n:GetNPCTypeID()==P.npc and n:GetSpawnPointID()==0
 and n:GetEntityVariable(NS..'.owner')~=''and n:GetEntityVariable(NS..'.run')~=''end
local function remove(n)
 if tagged(n)and n:GetEntityVariable(NS..'.state')~='clearing'then n:SetEntityVariable(NS..'.state','clearing');n:Depop(false)end
end
local function wave(c)
 local out={}
 for n in eq.get_entity_list():GetNPCList().entries do
  if tagged(n)and n:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())and n:GetEntityVariable(NS..'.run')==run(c)then out[#out+1]=n end
 end
 return out
end
function M.arm(n)
 if enabled()and n and n.valid and n:GetNPCTypeID()==P.giver then n:SetTimer(P.duck_timer,P.interval_seconds)end
end
function M.giver_timer(e)
 local n=e.self
 if e.timer~=P.duck_timer or not n or not n.valid or n:GetNPCTypeID()~=P.giver then return end
 if not enabled()then n:StopTimer(P.duck_timer);return end
 local waiting=false
 for c in eq.get_entity_list():GetClientList().entries do
  if active(c,7)then
   waiting=true
   if nearby(c,n)and c:GetAppearance()==2 then -- native eaCrouching, not animation111
    local epoch=run(c)
    if tostring(epoch):match('^[1-9]%d*$')then
     c:SetBucket(NS..'.duck_run',epoch)
     if c:GetBucket(NS..'.duck_run')==epoch then
      c:UpdateTaskActivity(P.task,7,1)
      c:Message(15,'Gronk accepts your show of obedience. Ask him about his army.')
     end
    end
   end
  end
 end
 if not waiting then n:StopTimer(P.duck_timer)end
end
function M.say(e)
 local n,c=e.self,e.other
 if n and n.valid and n:GetNPCTypeID()==P.giver and active(c,7)then M.arm(n)end
 if not n or not n.valid or n:GetNPCTypeID()~=P.giver or not c or not c.valid
  or not tostring(e.message or ''):lower():find('%f[%a]army%f[%A]')or not active(c,8)then return false end
 local epoch=run(c)
 if not tostring(epoch):match('^[1-9]%d*$')or c:GetBucket(NS..'.duck_run')~=epoch or not nearby(c,n)then return true end
 if #wave(c)>0 or c:GetBucket(NS..'.quarantine_run')==epoch then return true end
 -- This function runs in the real NPC SAY initiator context, unlike the duck
 -- timer. Read the owner's authoritative native count; never invent lost kills.
 local done=tonumber(eq.get_task_activity_done_count(P.task,8))
 if not done or done~=math.floor(done)or done<0 or done>=3 then return true end
 local required=3-done -- labelled recovery default after actual owned teardown
 if c:GetBucket(NS..'.credit_pending_run')==epoch then return true end
 local positions,why=G.points(n,P.offsets,P.geometry)
 if not positions or #positions~=3 then eq.debug('Gronk placement refused: '..tostring(why or 'three positions required'));c:Message(13,'Gronk cannot summon his army here right now.');return true end
 local prepared={}
 for i=1,required do
  local p=positions[i]
  local m=eq.spawn2(P.npc,0,0,p.x,p.y,p.z,p.heading)
  if not m or not m.valid or not m:IsNPC()then for _,a in ipairs(prepared)do remove(a)end;return true end
  local a=m:CastToNPC()
  if a:GetNPCTypeID()~=P.npc or a:GetSpawnPointID()~=0 then c:SetBucket(NS..'.quarantine_run',epoch);eq.debug('Gronk output identity unknown; stop, no wave credit.');return true end
  a:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()));a:SetEntityVariable(NS..'.run',epoch)
  a:SetEntityVariable(NS..'.index',tostring(i));a:SetEntityVariable(NS..'.born',tostring(os.time()))
  a:SetEntityVariable(NS..'.state','prepare');a:SetInvul(true);a:SetTargetable(false)
  a:SetSpecialAbility(24,1);a:SetSpecialAbility(25,1);a:SetTimer(P.timer,P.interval_seconds);prepared[#prepared+1]=a
 end
 -- The live owned wave blocks overlapping army requests. A later recovery
 -- wave uses only remaining native kills after verified teardown; no reset or
 -- manufactured progress. Ambiguous credit/identity quarantines this run.
 c:SetBucket(NS..'.wave_run',epoch)
 if c:GetBucket(NS..'.wave_run')~=epoch then for _,a in ipairs(prepared)do remove(a)end;return true end
 for _,a in ipairs(prepared)do
  a:SetEntityVariable(NS..'.state','active');a:SetInvul(false);a:SetTargetable(true)
  a:SetSpecialAbility(24,0);a:SetSpecialAbility(25,0);a:AddToHateList(c,1)
 end
 return true
end
function M.timer(e)
 local n=e.self
 if e.timer~=P.timer or not tagged(n)then return end
 local c=eq.get_entity_list():GetClientByCharID(tonumber(n:GetEntityVariable(NS..'.owner')))
 local born=tonumber(n:GetEntityVariable(NS..'.born'))
 if not enabled()or not c or not c.valid or run(c)~=n:GetEntityVariable(NS..'.run')
  or not yes(c:IsTaskActive(P.task))or not born or os.time()-born>=P.lifetime_seconds
  or n:GetEntityVariable(NS..'.state')=='prepare'then remove(n)end
end
function M.merit(e)
 local n,c=e.self,e.other
 if not tagged(n)or n:GetEntityVariable(NS..'.state')~='active'or not c or not c.valid or not c:IsClient()then return end
 c=c:CastToClient()
 if n:GetEntityVariable(NS..'.owner')~=tostring(c:CharacterID())or n:GetEntityVariable(NS..'.run')~=run(c)
  or not active(c,8)or n:GetEntityVariable(NS..'.credited')~=''then return end
 -- Native KILLED_MERIT has this real owner's initiator context.
 local before=tonumber(eq.get_task_activity_done_count(P.task,8))
 if not before or before~=math.floor(before)or before<0 or before>=3 then return end
 local epoch=run(c);c:SetBucket(NS..'.credit_pending_run',epoch)
 if c:GetBucket(NS..'.credit_pending_run')~=epoch then return end
 n:SetEntityVariable(NS..'.credited','1');c:UpdateTaskActivity(P.task,8,1)
 local after=tonumber(eq.get_task_activity_done_count(P.task,8))
 if after==before+1 then c:SetBucket(NS..'.credit_pending_run','')
 else eq.debug('Gronk native kill progress unconfirmed; no automatic credit/recovery replay.')end
end
function M.player_enter(c)
 if not active(c,7)then return end
 for n in eq.get_entity_list():GetNPCList().entries do
  if n.valid and n:GetNPCTypeID()==P.giver and n:GetSpawnPointID()>0 then M.arm(n)end
 end
end
function M.clear(c)
 if not c or not c.valid then return end
 for _,n in ipairs(wave(c))do remove(n)end
end
return M
