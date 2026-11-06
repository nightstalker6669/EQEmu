-- Shared roaming crystals, not personal substitute mobs or automatic facets.
-- All three readiness fields require the complete NPC/route/visual installation.
return {npc=753253,aura_npc=753901,profile_ready=false,aura_profile_ready=false,
 mines_geometry_ready=false,visible_race=615,hidden_race=127,
 timer='rof2_xorbb_crystal_aura',interval=1,lifetime=120,
 radius_xy=60,radius_z=30,max_owned_auras=4,head_interval=5,
 min_facets=1,max_facets=3}
-- Radius, vertical extent, lifetime, aura cap, head interval and uniform drop
-- roll are configurable emulator defaults. Current guide:1–3 facets/kill.
-- HydraCrystal615 is a visual candidate, NOT native-verified original appearance.
-- No mine bounds are guessed here: min/max XYZ must be populated and checked.
