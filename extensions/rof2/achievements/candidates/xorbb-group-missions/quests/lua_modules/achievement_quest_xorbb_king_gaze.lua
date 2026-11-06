-- King Xorbb facing observations only. No cue, spell, progress or reward grants.
-- Native generic IsFacingMob uses80heading units; using that as the encounter
-- half-width is a labelled configurable emulator default, not recovered LiveFOV.
local Context=require('achievement_quest_xorbb_mission_context')
local M={}
local Config=require('achievement_quest_xorbb_mission_data').missions.king.gaze
local function finite(x)return type(x)=='number'and x==x and math.abs(x)<math.huge end
function M.heading_separation(heading,toward)
 if not finite(heading)or not finite(toward)then return nil end
 return math.abs((heading-toward+256)%512-256)
end
function M.snapshot(c,king)
 local face_half_width=Config and Config.face_half_width
 local original_spell_range=Config and Config.spell_range
 if not finite(face_half_width)or face_half_width<0 or face_half_width>256
  or not finite(original_spell_range)or original_spell_range<=0 then return nil,'invalid_configuration'end
 local id,instance=Context.actor(c,king,'king','king_xorbb')
 if not id then return nil,'wrong_occurrence_or_actor'end
 local cx,cy,cz=c:GetX(),c:GetY(),c:GetZ()
 local x,y,z=king:GetX(),king:GetY(),king:GetZ()
 for _,v in ipairs({cx,cy,cz,x,y,z})do if not finite(v)then return nil,'invalid_position'end end
 local dx,dy,dz=cx-x,cy-y,cz-z
 if dx*dx+dy*dy<0.000001 then return nil,'undefined_horizontal_direction'end
 if dx*dx+dy*dy+dz*dz>original_spell_range^2 then return nil,'outside_spell_range'end
 -- Use the native heading-to-target function, preserving its coordinate and
 -- heading convention; this does not rotate the player or operate a client.
 local heading=c:GetHeading()
 local toward=c:CalculateHeadingToTarget(x,y)
 local separation=M.heading_separation(heading,toward)
 if not separation then return nil,'invalid_heading'end
 return {occurrence=id,instance=instance,character_id=c:CharacterID(),
  heading=heading,toward=toward,separation=separation,
  facing=separation<=face_half_width,averted=separation>face_half_width,
  facing_half_width=face_half_width,threshold_provenance='configurable emulator default'}
end
-- The instance controller still needs an actual recorded cue/affected roster,
-- deadline resolution, real Dominating Gaze application to failures, checked
-- sticky failure persistence and terminal ordering. A snapshot alone never
-- passes an episode or2075375; nil/unknown must never become a clean result.
return M
