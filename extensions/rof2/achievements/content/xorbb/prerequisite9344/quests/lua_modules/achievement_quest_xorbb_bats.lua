-- Real bat actors; native kill attribution owns every task increment.
-- Ownership tags restrict scene cleanup, not ordinary group combat credit.
local D=require('achievement_quest_xorbb_data')
local B=require('achievement_quest_xorbb_bats_data')
local M={}
local NS='rof2.xorbb.bats.v1'
local function yes(v)return v==true or v==1 end
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<math.huge end
local function active(c)
 return c and c.valid and eq.get_zone_id()==753 and D.enabled==true
  and D.ready[3040474]==true and yes(c:IsTaskActive(3040474))
  and yes(c:IsTaskActivityActive(3040474,8))
end
local function owned(n,c)
 return n and n.valid and n:GetNPCTypeID()==B.npc
  and n:GetEntityVariable(NS..'.owner')==tostring(c:CharacterID())
  and n:GetEntityVariable(NS..'.token')~=''
end
local function actors(c)
 local out={}
 for n in eq.get_entity_list():GetNPCList().entries do
  if owned(n,c)then out[#out+1]=n end
 end
 return out
end
function M.clear(c)
 if not c or not c.valid or eq.get_zone_id()~=753 then return end
 for _,n in ipairs(actors(c))do if n:GetHP()>0 then n:Depop(false)end end
 c:StopTimer(B.timer)
end
local function fail(c,message)
 M.clear(c)
 c:SetEntityVariable(NS..'.issued','failed')
 c:Message(13,message)
end
function M.start(c)
 if not active(c)or B.geometry_ready~=true or B.profile_ready~=true then return end
 if not(finite(B.x)and finite(B.y)and finite(B.z)and finite(B.heading))then return end
 local dx,dy,dz=c:GetX()-B.x,c:GetY()-B.y,c:GetZ()-B.z
 if dx*dx+dy*dy>B.activation_xy^2 or math.abs(dz)>B.activation_z then return end
 -- Reuse actual actors after a Lua reload. Never create another wave merely
 -- because script-local state was discarded, or replace a failed partial wave.
 if #actors(c)>0 then c:SetEntityVariable(NS..'.issued','yes');return end
 if c:GetEntityVariable(NS..'.issued')~=''then return end
 c:SetEntityVariable(NS..'.issued','pending')
 local token=tostring(c:CharacterID())..':'..os.time()..':'..c:GetID()
 for i=1,B.count do
  local m=eq.spawn2(B.npc,0,0,B.x,B.y,B.z,B.heading)
  if not m or not m.valid then
   fail(c,'The bat nest could not be created. Please report this before retrying.');return
  end
  local n=m:CastToNPC()
  if not n or not n.valid or n:GetNPCTypeID()~=B.npc then
   fail(c,'The new bat actor could not be identified. Please report this before retrying.');return
  end
  n:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()))
  n:SetEntityVariable(NS..'.token',token)
  n:SetEntityVariable(NS..'.expires',tostring(os.time()+B.lifetime))
  if not owned(n,c)then
   -- Only this just-created actor is eligible for immediate failed-tag cleanup.
   n:Depop(false);fail(c,'The bat scene ownership could not be retained.');return
  end
  n:SetSpecialAbility(4,1) -- Native AreaRampage; attack rate remains profile/default.
  n:SetSpecialAbility(13,1) -- Native MesmerizeImmunity, period May22,2013 report.
  n:SetTimer(B.timer,B.interval)
 end
 c:SetEntityVariable(NS..'.issued','yes')
 -- Aggro all four only after the complete wave exists; no partial successful
 -- nest and no timer-based kills or task updates.
 for _,n in ipairs(actors(c))do n:AddToHateList(c,1)end
 -- Period reports distinguish whirlwind social assistance from nearby bears
 -- and snakes. Use the existing live actors and native hate/combat, not extra
 -- summons or task credit. These actors never acquire scene-ownership tags
 -- and are never removed by bat cleanup. Range is an explicit server default.
 for n in eq.get_entity_list():GetNPCList().entries do
  if n and n.valid and B.social_whirlwinds[n:GetNPCTypeID()]and n:GetHP()>0 then
   local sx,sy,sz=n:GetX()-B.x,n:GetY()-B.y,n:GetZ()-B.z
   if sx*sx+sy*sy<=B.social_xy^2 and math.abs(sz)<=B.social_z and yes(n:CheckLoS(c))then
    n:AddToHateList(c,1)
   end
  end
 end
end
function M.stage(e)
 if tonumber(e.task_id)==3040474 and tonumber(e.activity_id)==7 then M.start(e.self)end
 -- Three kills advance the task, but the fourth real bat remains in combat.
 -- It is not silently killed/despawned to manufacture a cleaner completion.
end
function M.enter(e)
 local c=e.self
 if not active(c)then return end
 -- A reconnect can recover a retired scene by returning to the tree; existing
 -- native task kill counts remain unchanged. Refresh never replenishes a wave.
 if #actors(c)==0 then c:SetEntityVariable(NS..'.issued','')end
 c:SetTimer(B.timer,B.interval)
 M.start(c)
end
function M.timer(e)
 if e.timer~=B.timer then return false end
 local c=e.self
 if not active(c)then c:StopTimer(B.timer)else M.start(c)end
 return true
end
function M.actor_timer(e)
 if e.timer~=B.timer or not e.self or not e.self.valid then return end
 local n=e.self
 if n:GetNPCTypeID()~=B.npc then return end
 local owner=tonumber(n:GetEntityVariable(NS..'.owner'))
 local expires=tonumber(n:GetEntityVariable(NS..'.expires'))
 if not owner or n:GetEntityVariable(NS..'.token')==''then return end
 local c=eq.get_entity_list():GetClientByCharID(owner)
 -- Keep the fourth bat after the third kill while its owner remains on task.
 if not expires or os.time()>=expires or not c or not c.valid
  or not yes(c:IsTaskActive(3040474))then n:Depop(false)end
end
function M.closed(e)
 if tonumber(e.task_id)==3040474 then M.clear(e.self)end
end
function M.accepted(c,id)
 if id==3040474 and c and c.valid then M.clear(c);c:SetEntityVariable(NS..'.issued','')end
end
return M
