-- Labelled configurable emulator offsets/timers, not original Live coordinates.
-- geometry_ready is reviewed source/test-activation readiness, not native proof.
-- Keep false until controlled package review; record native acceptance separately.
local function geometry(size,anchor)return {geometry_ready=false,model_size=size,anchor_offset=anchor,
 max_anchor_floor_delta=16,max_floor_delta=3,max_floor_step=1,sample_spacing=1}end
return {profiles_ready=false,
 golem={task=3040485,giver=753030,npc=753913,timer='rof2.xorbb.golem.v1',
  offsets={{x=6,y=0}},geometry=geometry(8),cue_delay_seconds=5,window_seconds=10,
  lifetime_seconds=120,interval_seconds=1,owner_xy=50,owner_z=20},
 gronk={task=3040486,giver=753284,npc=753914,timer='rof2.xorbb.gronk.v1',
  duck_timer='rof2.xorbb.gronk.duck.v1',offsets={{x=-9,y=12},{x=-6,y=16},{x=-3,y=12}},
  geometry=geometry(3.5,{x=-6,y=12}),lifetime_seconds=600,interval_seconds=1,owner_xy=25,owner_z=10}}
