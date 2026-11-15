-- Approved primary design; logical open state requires separate physical acceptance.
local M={}
function M.open_after_native_receipt(client,binding)
  if binding.review_status~='APPROVED_PHYSICAL_BINDING' then return false end
  local z=eq.get_zone()
  if z:GetZoneID()~=468 or z:GetInstanceVersion()~=2 or z:GetInstanceID()==0 or
    client:GetSharedTaskInstanceID(920180,468,2)~=z:GetInstanceID() then return false end
  local state=client:SodSceneState(920180)
  if state~=4 and state~=5 and state~=6 and state~=2 then return false end
  if type(binding.door_db_id)~='number' or binding.door_db_id<=0 or
    type(binding.door_id)~='number' or binding.door_id<=0 then return false end
  local door=eq.get_entity_list():GetDoorsByDBID(binding.door_db_id)
  if not door.valid or door:GetDoorDBID()~=binding.door_db_id or door:GetDoorID()~=binding.door_id or
    door:GetDoorName()~=binding.model or door:GetOpenType()~=75 or door:GetKeyItem()~=0 or
    door:GetTriggerType()~=255 then return false end
  door:SetDisableTimer(true)
  if not door:IsDoorOpen()then door:ForceOpen(client,false)end
  -- This proves native logical state only. The fixture separately must prove
  -- rendered pose and collision clearing by ordinary walking. No player move.
  return door:IsDoorOpen()
end
return M
