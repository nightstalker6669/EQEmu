-- Original giver phrases and native task offers. No direct items, currency,
-- completion flags, scripted kill credit or fallback zone ports are awarded.
-- The combined ten-task cohort remains disabled until routed local readiness.
local M = {}
local T = {
  ManDefenses = 920175, NoCureDeath = 920176, FamiliarityKey = 920177,
  StopContamination = 920178, DreadChallenger = 920179, BlackburrowBoss = 920180,
  SummaryExecution = 920181, EvilLurks = 920182, Freedom = 920183,
  InvadingInvaders = 920190,
  SecretHidden = 920184, CorruptedPriest = 920185,
  RemoveHead = 920186, Bubonians = 920187,
}
M.tasks = T

local function contains(message, phrase)
  return message:lower():find(phrase, 1, true) ~= nil
end

function M.say(e, expected_type)
  if e.self:GetNPCTypeID() ~= expected_type then return end
  if e.other:GetSodVoidZone() == 0 then return end -- inactive/read unavailable
  local m = e.message or ""
  if contains(m, "hail") then
    if expected_type == 466141 then
      e.self:Say("The village needs help. Can you make yourself [useful]?")
    elseif expected_type == 466035 then
      e.self:Say("Ask about the [plague] or the [work] still to be done.")
    elseif expected_type == 466154 then
      e.self:Say("Tell me [I will hunt] if you will help our clan.")
    elseif expected_type == 466157 then
      e.self:Say("Our supplies were stolen. Tell me [I will get supplies] if you can help.")
    elseif expected_type == 466160 then
      e.self:Say("The Darkpaw clan needs your [help].")
    elseif expected_type == 466155 then
      e.self:Say("Our clan needs assistance. Tell me [I will help] if you are willing.")
    elseif expected_type == 467066 then
      e.self:Say("You may [investigate] the temple, prepare for the [storm], or ask about [work].")
    elseif expected_type == 467069 then
      e.self:Say("If you will confront the threat, tell me [destroy them].")
    end
    -- Native SpeakWith processing runs after the actual NPC say event. Do not
    -- duplicate its activity update or treat the hail as persisted completion.
    return
  end
  local offers
  if expected_type == 466141 and contains(m, "useful") then
    offers = {T.ManDefenses, T.NoCureDeath}
  elseif expected_type == 466035 and contains(m, "plague") then
    offers = {T.FamiliarityKey}
  elseif expected_type == 466035 and contains(m, "work") then
    if e.other:GetSodTaskCompletionState(T.FamiliarityKey) == 1 then
      offers = {T.StopContamination, T.DreadChallenger}
    else
      offers = {T.FamiliarityKey}
    end
  elseif expected_type == 466154 and contains(m, "hunt") then
    offers = {T.SummaryExecution}
  elseif expected_type == 466157 and (contains(m, "get supplies") or contains(m, "get the supplies")) then
    offers = {T.EvilLurks}
  elseif expected_type == 466160 and (contains(m, "help") or contains(m, "not be crazy")) then
    offers = {T.Freedom, T.BlackburrowBoss}
  elseif expected_type == 466155 and contains(m, "i will help") then
    offers = {T.InvadingInvaders}

  end
  if offers then
    -- Uses the actual nearby giver, native cooldown/level checks and the new
    -- authoritative requester gate at both offer and acceptance, including
    -- tasks.enabled. Participant admission remains the native shared path.
    e.other:TaskSelectorFromNPC(e.self, offers)
  end
end

return M
