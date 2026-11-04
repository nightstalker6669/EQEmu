-- Generated from settings.json. Original numerical restrictions/rewards unknown.
-- These are labelled configurable server defaults, not original historical facts.
local P={enabled=true,with_oculus=false,minimum_level=75}
function P.admit(c)
 if not c or not c.valid then return false end
 if c:GetLevel()<P.minimum_level then
  c:Message(15,'This server currently offers Gearing Up from level '..P.minimum_level..'.')
  return false
 end
 return true -- No unverified faction admission threshold.
end
return P
