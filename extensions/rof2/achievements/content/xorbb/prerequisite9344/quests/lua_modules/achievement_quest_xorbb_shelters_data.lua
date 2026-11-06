return {profile_ready=false,geometry_ready=false,shelter=753086,waterbrigade=753081,
 task=3040475,timer='rof2_xorbb_waterbrigade',interval=1,cold_interval=20,
 spawn_count=2,lifetime=600,heal_interval=12,heal_amount=200000,
 heal_xy=40,heal_z=20}
-- Existing original spell IDs define torch vulnerability and Cold Water damage/
-- movement. Spawn count, interval and lifetime are labelled configurable server
-- defaults; original add count/timing is not established. Full combat profiles
-- and actual shelter floor geometry must be ready before enabling this scene.
-- Healing attempts are documented by the current quest guide. Amount200000,
-- interval12s and proximity40/20 are configurable emulator defaults, not
-- recovered original spell/timing values. Use native HealDamage, not SetHP.
