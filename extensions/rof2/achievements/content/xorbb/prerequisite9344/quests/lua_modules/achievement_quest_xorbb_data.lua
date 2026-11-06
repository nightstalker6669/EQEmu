-- Disabled source candidate: all scene producers must be completed before enabling.
return {
 enabled=false,
 ready={[3040474]=false,[3040475]=false,[3040476]=false},
 minimum_level=85, -- Current guide header; historical provenance in research fact graph.
 ground_decay_ms=600000, -- Configurable emulator lifetime, not recovered original timing.
 ground_pickup_xy=40,ground_pickup_z=20, -- Configurable authored interaction bounds.
 ground_refresh_seconds=2, -- Configurable emulator poll interval.
 door_interaction_xy=80,door_interaction_z=40, -- Authored interaction bounds.
 group_update_xy=100,group_update_z=40, -- Configurable group participation bounds.
 group_witness_seconds=10, -- Configurable maximum age of synchronous pickup evidence.
 gerbril={npc=753147,timer='rof2_xorbb_gerbril',spell_interval=20,
  spells={33527,33528}}, -- Tenderize/Meat Mallet; 20s is a labelled configurable default.
 cook={timer='rof2_xorbb_minotaur_cook',spell_interval=20}, -- Authored recast default.
 giver=753036,
 tasks={
  [3040474]={title='The Halfling Trail',keyword='help',prior={}},
  [3040475]={title='We Have to Go Deeper',keyword='right',prior={3040474}},
  [3040476]={title='Escape From the Prison of Xorbb',keyword='face',prior={3040474,3040475}}
 }
}
