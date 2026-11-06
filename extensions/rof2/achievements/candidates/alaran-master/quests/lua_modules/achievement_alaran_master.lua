-- Call only from a subsequently verified qualifying primer/training interaction.
-- This helper does not establish that a primer was read or bind any callback.
local D=require('achievement_alaran_master_data')
local M={}
function M.credit(c)
 if not D.enabled then return false,'disabled' end
 if not D.trigger_verified then return false,'trigger_unbound' end
 if type(D.progress_value)~='number'or D.progress_value<1 or D.progress_value>4294967295 or D.progress_value~=math.floor(D.progress_value)or D.required_count~=D.progress_value then return false,'representation_unbound' end
 if not c or not c.valid then return false,'invalid_client' end
 local actual=c:GetLanguageSkill(D.language_id)
 if type(actual)~='number'or actual~=math.floor(actual)or actual<D.language_required then return false,'below_language_threshold' end
 -- Existing absolute setter: no class SkillValue25, language training, item
 -- consumption, task completion or direct completion of optional components.
 local result=c:SetAchievementProgress(D.achievement_id,D.component_type,D.component_id,D.progress_value,false)
 -- A true return can be a deferred RAM update. It is not a durable receipt.
 return result==true or result==1,'existing_setter_result'
end
return M
