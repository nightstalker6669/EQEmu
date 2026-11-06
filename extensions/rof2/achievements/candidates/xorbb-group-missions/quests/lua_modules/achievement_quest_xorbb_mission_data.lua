-- Inactive source: coordinator-reserved task/template/version identities; reviewed geometry
-- are required before this module can admit or move any character.
return {enabled=false,zone=753,minimum_level=85,missions={
 battle={task=3040488,template=3040488,version=1,ready=false,geometry_ready=false,
  giver=753036,request='battle',enter='go',
  prerequisites={3040474,3040475,3040476},entry=false,safereturn=false},
 king={task=3040489,template=3040489,version=2,ready=false,geometry_ready=false,
  giver=753036,request='small',enter='enter',
  -- Half-width80 is a configurable emulator default; range1500 is client37201.
  gaze={face_half_width=80,spell_range=1500},
  prerequisites={3040474,3040475,3040476,'battle'},entry=false,safereturn=false},
 gems={task=3040490,template=3040490,version=3,ready=false,geometry_ready=false,
  giver=753191,request='help',enter='ready',
  prerequisites={},entry=false,safereturn=false}
}}
