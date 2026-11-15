-- New local hook; native dispatcher also preserves existing global enter callbacks.
function event_enter_zone(e)
  local c=e.self
  local z=eq.get_zone()
  -- Touch objectives are not emitted automatically by an ATP zone transition.
  -- Bind this actual arrival to the active canonical mission, not an old ID.
  if z:GetZoneID()==468 and z:GetInstanceVersion()==2 and z:GetInstanceID()>0 and
    c:IsTaskActive(920180) and c:IsTaskActivityActive(920180,0) and
    c:GetSharedTaskInstanceID(920180,468,2)==z:GetInstanceID() then
    c:UpdateTaskActivity(920180,0,1)
  end
  require('rof2_sod_scene').enter(c)
end
