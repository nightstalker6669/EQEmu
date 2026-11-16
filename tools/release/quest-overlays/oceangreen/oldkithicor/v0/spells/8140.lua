-- Original Firebrand item effect; preserve the actual native spell effect.
function event_spell_effect(e)
  if e.spell_id~=8140 or not e.target.valid or not e.target:IsNPC() then return 0 end
  local caster=eq.get_entity_list():GetMobID(e.caster_id)
  if caster.valid and caster:IsClient() then
    caster:CastToClient():SodKithicorBurn(e.target:CastToNPC())
  end
  return 0
end
