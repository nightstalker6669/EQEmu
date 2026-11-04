-- A real original Ring of Binding action, custody-verified result, normal source
-- respawn. No invented corpse, kill, extra charge deletion or scripted reward.
local D=require('achievement_quest_xorbb_mercenary_data')
local I=require('achievement_quest_mesa_inventory')
local W=require('achievement_quest_xorbb_world_eye')
local M={};local TASK=3040487;local NS='rof2.xorbb.binding.v1'
local function yes(v)return v==true or v==1 end
function M.cast(e)
 local c,t=e.self,e.target
 if not D.enabled or D.ready[TASK]~=true or eq.get_zone_id()~=753
  or tonumber(e.cast_item_id)~=17750 or not yes(e.cast_item_charge_consumed)
  or not c or not c.valid or not c:IsClient()or tonumber(e.caster_id)~=c:GetID()
  or not yes(c:IsTaskActive(TASK))or not yes(c:IsTaskActivityActive(TASK,8))
  or not e.spell or not e.spell.valid or e.spell:ID()~=37147
  or not t or not t.valid or not t:IsNPC()then return end
 local n=t:CastToNPC()
 if tonumber(e.target_id)~=n:GetID()or not W.allowed(c,n,30,200)
  or n:GetEntityVariable(NS..'.state')~=''then return end
 -- Completed-cast context survives native consumption of the original ring;
 -- do not demand that the used-up item still occupies its old slot.
 if I.count(c,17893)~=0 or c:GetInventory():GetItem(Slot.Cursor).valid then
  c:Message(13,'The empowered ring could not be received. Empty your cursor and recover a binding ring from Zerkis before trying again.');return
 end
 n:SetEntityVariable(W.lock_key,'binding:'..c:CharacterID()..':'..n:GetID())
 n:SetEntityVariable(NS..'.state','pending')
 local before=I.count(c,17893)
 local given=I.give(c,17893,1);local after=I.count(c,17893)
 if not given or after~=before+1 then
  if after==before then n:SetEntityVariable(NS..'.state','');n:SetEntityVariable(W.lock_key,'')
  else eq.debug('Xorbb binding result delta ambiguous; source locked, no replay.')end
  c:Message(13,'Binding custody could not be verified; no progress was granted.');return
 end
 n:SetEntityVariable(NS..'.state','committed')
 n:SetEntityVariable(NS..'.owner',tostring(c:CharacterID()))
 -- Depop(true) ONCE starts the original native Spawn2 timer immediately.
 -- Never disable it with false and never attempt a second deferred teardown.
 -- The original type/spawn/loot definitions remain unchanged; there is no kill.
 n:Depop(true)
 c:UpdateTaskActivity(TASK,8,1)
end
return M
