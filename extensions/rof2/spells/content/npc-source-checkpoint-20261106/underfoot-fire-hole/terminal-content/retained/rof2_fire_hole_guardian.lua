-- Original Vault Guardian death producer SOURCE. Disabled and unregistered.
local M={}
local b=require('rof2_fire_hole_bindings')
local state={}
local function context()
 if not b.enabled or not b.readiness.guardian or not b.readiness.doors
  or not b.readiness.custody or not b.readiness.geometry or not b.readiness.terminal then return nil end
 local instance=eq.get_zone_instance_id()
 if instance==0 or eq.get_zone_id()~=b.zone or eq.get_instance_zone_id_by_id(instance)~=b.zone
  or eq.get_instance_version_by_id(instance)~=b.version
  or eq.get_instance_timer_by_id(instance)<=0 then return nil end
 if state.instance~=instance then state={instance=instance,entities={},dead={}} end
 if state.blocked then return nil end
 return instance
end
local function owned(npc)
 local instance=context()
 if not instance or not npc or not npc.valid or npc:GetNPCTypeID()~=b.npc_types.vault_guardian then return nil end
 local spawn=npc:GetSpawnPointID()
 if not b.original_guardian_spawns[spawn] then return nil end
 return spawn
end
local function member(client)
 local instance=context();local task=b.tasks.fire_hole
 if not instance or not client or not client.valid or not client:IsClient()
  or not client:IsTaskActive(task) or not eq.check_instance_by_char_id(instance,client:CharacterID())
  or client:GetSharedTaskInstanceID(task,b.zone,b.version)~=instance then return false end
 local run=client:GetSharedTaskIdentity()
 if type(run)~='string' or not run:match('^%d+$') or run=='0'
  or (state.run and state.run~=run) then return false end
 state.run=run -- uint64 decimal string; never tonumber
 return true
end
function M.spawn(e)
 local spawn=owned(e.self)
 if not spawn then return end
 if state.entities[spawn] and state.entities[spawn]~=e.self:GetID() then state.blocked=true;return end
 state.entities[spawn]=e.self:GetID()
end
function M.death(e)
 local spawn=owned(e.self)
 if not spawn or not e.killed_entity_id or e.killed_entity_id==0
  or state.entities[spawn]~=e.killed_entity_id or state.dead[spawn] then return end
 state.dead[spawn]=e.killed_entity_id -- record genuine death once, including premature/ineligible deaths
 local killer=e.other
 if killer and killer.valid and not killer:IsClient() then killer=killer:GetOwner() end
 if not killer or not killer.valid or not killer:IsClient() then return end
 local client=killer:CastToClient();local task=b.tasks.fire_hole
 if not member(client) or not client:IsTaskActivityActive(task,16) then return end
 state.submitted=true
 client:UpdateTaskActivity(task,16,1)
 -- No mold factory or direct lock: native objective16 acknowledgment activates17/lock.
 -- Original corpse loot owns47758 and genuine loot17; terminal Mold18 remains a separate held consumer.
end
function M.updated(e)
 if e.task_id~=b.tasks.fire_hole or e.activity_id~=16 or not member(e.self) then return end
 if state.submitted and e.count>=1 then state.observed_native_count=e.count end
 -- Current packet lacks originating run/cast identity. This is an observed count, NOT a correlated commit receipt.
 -- No manually unlocked doors, extra items/rewards or reconstructed deaths.
end
return M
