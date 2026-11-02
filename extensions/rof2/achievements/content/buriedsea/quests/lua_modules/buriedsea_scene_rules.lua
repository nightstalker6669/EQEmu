-- Missing encounter numbers are approved, configurable emulator defaults.
-- Original mechanics and the provenance of each reconstruction are recorded
-- in provenance/encounter-scenes.json. Do not describe these numbers as Live.
return {
 enabled=false,
 timer='buriedsea_scene_2013',
 coralisk={task=3040924,npc=423900,bucket=88244,dump_activity=0,kill_activity=1,
  -- Original /loc -1797.66,2890.62,296.85; ground Z from installed mesh.
  point={x=2890.620117,y=-1797.660034,z=290.885040},
  horizontal_radius=45,vertical_radius=35,
  spawn_chance_percent=50,unengaged_lifetime_seconds=300},
 silverlip={task=3040920,recipient=422074,package=88232,bill=88233,
  bill_activity=1,arrival_delay_seconds=3,unengaged_lifetime_seconds=90,
  aggro_distance=80,arrival_distance=25,
  destination={x=-940.125,y=1061.625,z=-3.310963,heading=357},
  actors={
   {npc=422900,x=-940.125,y=1211.625,z=1.839764,heading=256},
   {npc=422900,x=-940.125,y=1211.625,z=1.839764,heading=256},
   {npc=422901,x=-940.125,y=1211.625,z=1.839764,heading=256,bill=true}
  }}
}
