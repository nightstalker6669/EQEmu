-- Real Battle NPC combat only. No synthetic kill/task/challenge updates.
-- The existing occurrence controller must supply c and invoke this from its owned timer.
local Context=require('achievement_quest_xorbb_mission_context')
local Layout=require('achievement_quest_xorbb_battle_layout_data')
local P=require('achievement_quest_xorbb_battle_engagement_data')
local M={}
local allies={ally_bruiser=true,ally_commander=true,ally_defender=true,
 ally_healer=true,ally_ranger=true,ally_trooper=true,ally_exploder=true}
local enemies={enemy_bruiser=true,enemy_commander=true,enemy_defender=true,
 enemy_flame_ranger=true,enemy_healer=true,enemy_lord=true,enemy_trooper=true,enemy_exploder=true}
local function opponents(role)
 if allies[role]then return enemies,P.ally_radius end
 if role=='powershard_golem'then return allies,P.golem_radius end
end
local function alive(n)
 return n and n.valid and n:IsNPC()and n:GetHP()>0 and n:GetOwnerID()==0
end
local function yes(v)return v==true or v==1 end
-- Existing hate is never overwritten: a player pulling an enemy/golem retains
-- normal combat and native aggro behavior. Golems may pause to fight actual allies.
function M.tick(c,n,role)
 if not P.enabled or not Layout.enabled or not Layout.geometry_ready
  or not Layout.movement_ready or not alive(n)or n:IsEngaged()then return false end
 local allowed,radius=opponents(role)
 if not allowed or type(radius)~='number'or radius<=0 then return false end
 local id,instance=Context.actor(c,n,'battle',role)
 if not id then return false end
 local target,best
 for other in eq.get_entity_list():GetNPCList().entries do
  if alive(other)and other:GetID()~=n:GetID()then
   local other_role=other:GetEntityVariable('xorbb_mission_role')
   if allowed[other_role]then
    local oid,oi=Context.actor(c,other,'battle',other_role)
    if oid==id and oi==instance then
     local dx,dy,dz=n:GetX()-other:GetX(),n:GetY()-other:GetY(),n:GetZ()-other:GetZ()
     local distance=dx*dx+dy*dy
     if math.abs(dz)<=P.vertical_radius and distance<=radius*radius
      and(not best or distance<best or(distance==best and other:GetID()<target:GetID()))
      and yes(n:CheckLoS(other))and yes(n:IsAttackAllowed(other))then
      target,best=other,distance
     end
    end
   end
  end
 end
 if not target then return false end
 -- Recheck native/occurrence state immediately before the only combat mutation.
 local current,ci=Context.actor(c,n,'battle',role)
 local tid,ti=Context.actor(c,target,'battle',target:GetEntityVariable('xorbb_mission_role'))
 if current~=id or ci~=instance or tid~=id or ti~=instance
  or not alive(n)or not alive(target)or n:IsEngaged()
  or not yes(n:CheckLoS(target))or not yes(n:IsAttackAllowed(target))then return false end
 n:AddToHateList(target,1)
 return yes(n:IsEngaged())
end
return M
