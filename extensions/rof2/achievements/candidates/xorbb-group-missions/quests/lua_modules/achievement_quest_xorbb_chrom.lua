-- Chrom's six once-per-occurrence health waves. Content-specific, inactive.
-- The instance controller owns creation, geometry, delivery and corpse loot.
local D=require('achievement_quest_xorbb_mission_data')
local A=require('achievement_quest_xorbb_mission_actors')
local M={}
local waves={
 {hp=75,role='golem_diamond'}, {hp=65,role='golem_sapphire'},
 {hp=55,role='golem_emerald'}, {hp=45,role='golem_ruby'},
 {hp=35,role='golem_amethyst'}, {hp=25,role='golem_onyx'}
}
-- Six thresholds/two adds are researched; color order is an explicitly
-- configurable emulator default. Do not infer original order from this table.
local function enabled()return D.enabled and D.missions.gems.ready and A.enabled
 and type(eq.xorbb_chrom_state)=='function'end
-- eq.set_next_hp_event applies to the current quest OWNER, not the NPC argument.
-- Initialization/resume must be dispatched in Chrom's own NPC event context.
local function owns_context(n)
 local owner=eq.get_owner()
 return n and n.valid and n:GetID()>0 and owner and owner.valid
  and owner:IsNPC()and owner:GetID()==n:GetID()
end
local function yes(v)return v==true or v==1 end
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<math.huge end
local persistence_uncertain={}
local function empty_client()return eq.get_entity_list():GetClientByID(0)end
local function checked(c,n,id,instance,op,field,value)
 local r=eq.xorbb_chrom_state(c or empty_client(),n,id,tonumber(instance),op,field or '',value or '')
 if type(r)~='table'or r.ok~=true then persistence_uncertain[id]=true;return nil end
 return r
end
local function scope(c,n)
 if not enabled()or not owns_context(n)or not c or not c.valid or not c:IsClient()
  or not yes(c:IsTaskActive(3040490))or not yes(c:IsTaskActivityActive(3040490,13))
  or not n or not n.valid or not n:IsNPC()or n:GetNPCTypeID()~=A.missions.gems.actors.chrom
  or eq.get_zone_id()~=753 or eq.get_zone_instance_version()~=3 then return nil end
 local id=c:GetSharedTaskIdentity()
 local instance=eq.get_zone_instance_id()
 if type(id)~='string'or not id:match('^[1-9]%d*$')or not instance or instance<=0
  or c:GetSharedTaskInstanceID(3040490,753,3)~=instance then return nil end
 return id,tostring(instance)
end
local function attach(n,id,instance)
 local prior=n:GetEntityVariable('xorbb_chrom_occurrence')
 local prior_instance=n:GetEntityVariable('xorbb_chrom_instance')
 if(prior~=''and prior~=id)or(prior_instance~=''and prior_instance~=instance)then return false end
 n:SetEntityVariable('xorbb_chrom_occurrence',id)
 n:SetEntityVariable('xorbb_chrom_instance',instance)
 return true
end
local function schedule(values)
 for _,w in ipairs(waves)do
  local state=values['wave'..w.hp]
  if state=='armed'then eq.set_next_hp_event(w.hp);return true end
  if state~='done'then return false end
 end
 return true
end
-- Only Chrom's own initialization timer, dispatched after the controller's
-- first actual creation following six physical gem deliveries, may call this. Never call it on spawn/reload/aggro automatically.
-- Existing or partial state refuses initialization rather than resetting waves.
function M.begin_new(c,n)
 local id,instance=scope(c,n)
 if not id or n:GetHPRatio()<100 or persistence_uncertain[id]then return false end
 local r=checked(c,n,id,instance,'chrom_initialize')
 if not r then return false end
 if not attach(n,id,instance)then return false end
 return schedule(r.values)
end
-- Healing, wipe, depop/recreation and module reload reuse the same state.
-- Missing/pending state is an unresolved write/spawn window, not a fresh fight.
function M.resume(c,n)
 local id,instance=scope(c,n)
 if not id or persistence_uncertain[id]then return false end
 if not attach(n,id,instance)then return false end
 local r=checked(c,n,id,instance,'chrom_read')
 return r and schedule(r.values)or false
end
local function point(p)
 return type(p)=='table'and finite(p.x)and finite(p.y)and finite(p.z)and finite(p.h)
end
-- Called in Chrom's real EVENT_HP context with owner-reviewed spawn points.
-- A large damage hit may cross several thresholds: materialize every crossed
-- armed wave. Never fabricate threshold activity or use a timer for combat.
function M.hp(e,positions)
 local n=e.self
 if not enabled()or not owns_context(n)or not n or not n.valid or n:GetNPCTypeID()~=A.missions.gems.actors.chrom
  or eq.get_zone_id()~=753 or eq.get_zone_instance_version()~=3 then return false end
 local id=n:GetEntityVariable('xorbb_chrom_occurrence')
 local instance=n:GetEntityVariable('xorbb_chrom_instance')
 if not id:match('^[1-9]%d*$')or instance~=tostring(eq.get_zone_instance_id())
  or persistence_uncertain[id]then return false end
 local snapshot=checked(nil,n,id,instance,'chrom_read')
 if not snapshot then return false end
 local hp=n:GetHPRatio()
 if not finite(hp)or hp<0 or hp>100 then return false end
 for _,w in ipairs(waves)do
  local k='wave'..w.hp
  local state=snapshot.values[k]
  if state~='armed'and state~='done'then return false end
  if hp<=w.hp and state=='armed'then
   local p=positions and positions[w.role]
   if not p or not point(p[1])or not point(p[2])then return false end
   -- Reserve before side effects. A crash/partial spawn stays pending and must
   -- be reconciled, never retried as a new pair and never silently marked done.
   -- Only this committed armed->pending winner may perform either spawn.
   if not checked(nil,n,id,instance,'chrom_reserve',k)then return false end
   for slot=1,2 do
    local v=p[slot]
    local mob=eq.spawn2(A.missions.gems.actors[w.role],0,0,v.x,v.y,v.z,v.h)
    if not mob or not mob.valid or not mob:IsNPC()then return false end
    local add=mob:CastToNPC()
    add:SetEntityVariable('xorbb_chrom_occurrence',id)
    add:SetEntityVariable('xorbb_chrom_instance',instance)
    add:SetEntityVariable('xorbb_chrom_wave',tostring(w.hp))
    add:SetEntityVariable('xorbb_chrom_slot',tostring(slot))
    if not checked(nil,n,id,instance,'chrom_slot',k..'_slot'..slot,tostring(add:GetID()))then return false end
    local target=n:GetTarget()
    if target and target.valid then add:AddToHateList(target,1)end
   end
   local done=checked(nil,n,id,instance,'chrom_done',k)
   if not done then return false end
   snapshot=done
  end
 end
 return schedule(snapshot.values)
end
return M
