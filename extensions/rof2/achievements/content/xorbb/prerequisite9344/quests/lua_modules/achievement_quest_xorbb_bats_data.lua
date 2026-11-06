-- Four actors / three required kills: dated Dec6,2012 and May22,2013 reports.
-- Level96 is the current bestiary value, not a preserved2012 stat export.
-- Actual NPC profile and branch-adjacent anchor must be installed/verified.
return {npc=753900,profile_ready=false,geometry_ready=false,count=4,
 timer='rof2_xorbb_disturbed_bats',interval=2,lifetime=600,
 activation_xy=80,activation_z=40,social_xy=60,social_z=30,
 -- Dec6,2012 report: whirlwinds assist bats, bears/snakes do not. Exact
 -- assist range is a configurable default; IDs bind existing local types only.
 social_whirlwinds={[753010]=true,[753014]=true,[753015]=true,[753022]=true,
  [753106]=true,[753130]=true,[753160]=true,[753234]=true}}
-- Timer, lifetime and activation bounds are configurable emulator defaults.
