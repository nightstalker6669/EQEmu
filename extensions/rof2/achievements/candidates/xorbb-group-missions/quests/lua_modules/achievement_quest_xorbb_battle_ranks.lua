-- Apply a genuine owned replacement's rank; award only on its native death.
-- The mission controller owns durable camp-cycle and spawn-generation state.
local D=require('achievement_quest_xorbb_mission_data')
local P=require('achievement_quest_xorbb_battle_rank_data')
local Context=require('achievement_quest_xorbb_mission_context')
local M={}
local stats={'max_hp','max_hit','min_hit','ac','atk'}
local function yes(v)return v==true or v==1 end
local function expected(role,rank)
 local base,tier=P.roles[role],P.ranks[rank]
 if not base or not tier then return nil end
 local out={}
 for _,stat in ipairs(stats)do out[stat]=math.floor(base[stat]*tier.scale+0.5)end
 return out,tier,base
end
local function matches(n,profile)
 for _,stat in ipairs(stats)do if n:GetNPCStat(stat)~=profile[stat]then return false end end
 return true
end
-- Selection never grants achievements. Its input must be the controller's
-- checked durable count for this camp, not a client message/global kill tally.
function M.next_rank(camp_defeats)
 if type(camp_defeats)~='number'or camp_defeats<0 or camp_defeats~=math.floor(camp_defeats)then return nil end
 local selected=0
 for rank=1,4 do if camp_defeats>=P.ranks[rank].minimum_camp_defeats then selected=rank end end
 return selected
end
function M.apply(c,n,role,rank,generation)
 if not P.enabled then return false end
 local id=Context.actor(c,n,'battle',role)
 local profile,tier,base=expected(role,rank)
 if not id or not profile or type(generation)~='string'or not generation:match('^[1-9]%d*$')then return false end
 local old=n:GetEntityVariable('xorbb_battle_rank')
 if old~=''then
  -- A duplicate callback must not heal or re-rank an already fighting actor.
  return old==tostring(rank)and n:GetEntityVariable('xorbb_battle_generation')==generation and matches(n,profile)
 end
 if n:IsEngaged()or n:GetHPRatio()<100 then return false end
 for _,stat in ipairs(stats)do n:ModifyNPCStat(stat,tostring(profile[stat]))end
 if not matches(n,profile)then return false end
 n:SetHP(n:GetMaxHP())
 local name=tier.prefix==''and base.name or(tier.prefix..' '..base.name)
 n:TempName(name:gsub(' ','_'))
 if n:GetCleanName():gsub('_',' ')~=name then return false end
 -- Set the rank witness only after this actor's real stats/name were applied.
 n:SetEntityVariable('xorbb_battle_generation',generation)
 n:SetEntityVariable('xorbb_battle_rank',tostring(rank))
 return true
end
local function witness(n,role)
 local rank=tonumber(n:GetEntityVariable('xorbb_battle_rank'))
 local profile,tier,base=expected(role,rank)
 local generation=n:GetEntityVariable('xorbb_battle_generation')
 if not profile or not generation:match('^[1-9]%d*$')or not matches(n,profile)then return nil end
 local achievement=tier.achievement
 if base.family=='powershard_golem'then
  achievement=rank==4 and 2075374 or nil
 elseif not P.challenge_roles[role]then achievement=nil end
 return achievement,generation,rank
end
-- Native NPC::Death sends KILLED_MERIT to the actual experience recipient's
-- group/raid/self before DEATH_COMPLETE. The final blow alone is not merit.
-- Retain one exact eligible member for this actor generation, never a nearby
-- player/name match or a fabricated kill/camp-rank counter.
function M.merit(e,role)
 if not P.enabled or not P.roles[role]or not e or not e.self or not e.self.valid
  or not e.other or not e.other.valid or not e.other:IsClient()then return false end
 local c=e.other:CastToClient()
 if not Context.actor(c,e.self,'battle',role)then return false end
 local achievement,generation,rank=witness(e.self,role)
 if not achievement then return false end
 local prior=e.self:GetEntityVariable('xorbb_battle_merit_character')
 if prior~=''then
  return e.self:GetEntityVariable('xorbb_battle_merit_generation')==generation
   and e.self:GetEntityVariable('xorbb_battle_merit_rank')==tostring(rank)
 end
 e.self:SetEntityVariable('xorbb_battle_merit_character',tostring(c:CharacterID()))
 e.self:SetEntityVariable('xorbb_battle_merit_generation',generation)
 e.self:SetEntityVariable('xorbb_battle_merit_rank',tostring(rank))
 return true
end
-- Called only by EVENT_DEATH_COMPLETE after a real native merit callback, not
-- scheduling, timers, last-blow ownership alone or mission finish.
function M.death(e,role)
 if not P.enabled or not P.roles[role]or not e or not e.self or not e.self.valid
  or not e.corpse or not e.corpse.valid
  or type(e.killed_entity_id)~='number'or e.killed_entity_id<=0
  or e.corpse:GetID()~=e.killed_entity_id then return false end
 local character=e.self:GetEntityVariable('xorbb_battle_merit_character')
 if not character:match('^[1-9]%d*$')then return false end
 local c=eq.get_entity_list():GetClientByCharID(tonumber(character))
 local id=Context.actor(c,e.self,'battle',role)
 if not id then return false end
 local achievement,generation,rank=witness(e.self,role)
 if not achievement or e.self:GetEntityVariable('xorbb_battle_merit_generation')~=generation
  or e.self:GetEntityVariable('xorbb_battle_merit_rank')~=tostring(rank)then return false end
 if e.self:GetEntityVariable('xorbb_battle_rank_award_submitted')=='1'then return true end
 if not yes(c:AdvanceSharedTaskAchievementProgress(achievement,1,achievement,1))then return false end
 e.self:SetEntityVariable('xorbb_battle_rank_award_submitted','1')
 -- Absolute count1 submission; world persistence/title/native rendering remain
 -- separate. No fake CompleteAchievement or direct title grants.
 return true
end
return M
