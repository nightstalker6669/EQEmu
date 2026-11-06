local M = {}
local delivery = require("achievement_native_delivery_handin")
local tasks = {
  {id=920132, phrase="restored", prerequisites={}},
  {id=920133, phrase="eliminate", prerequisites={}},
  {id=920134, phrase="deeper", prerequisites={920132,920133}},
  {id=920135, phrase="agreement", prerequisites={920132,920133,920134}},
}
local speech = {
  [482005]={
    {task=920135, activities={0,8,18}},
  },
  [482144]={
    {task=920134, activities={1,2,3}},
  },
  [482192]={
    {task=920132, activities={6}},
    {task=920133, activities={3}},
    {task=920134, activities={10}},
    {task=920135, activities={19}},
  },
  [482244]={
    {task=920135, activities={9,17}},
  },
}
local function eligible(client, prerequisites)
  for _, id in ipairs(prerequisites) do
    if not client:IsTaskCompleted(id) then return false end
  end
  return true
end
function M.say(e, npc_id)
  local message = e.message:lower()
  if message:match("%f[%a]hail%f[%A]") then
    local pending = {}
    for _, task in ipairs(speech[npc_id] or {}) do
      if e.other:IsTaskActive(task.task) then
        for _, activity in ipairs(task.activities) do
          if e.other:IsTaskActivityActive(task.task, activity) then
            pending[#pending+1] = {task.task, activity}
            break
          end
        end
      end
    end
    for _, entry in ipairs(pending) do
      e.other:UpdateTaskActivity(entry[1], entry[2], 1)
    end
    if #pending > 0 then
      e.self:Say("Thank you for helping restore order in the Underquarry.")
    elseif npc_id == 482192 then
      e.self:Say("Help the metal mines be [restored], [eliminate] the stonequarry rebels, seek [deeper] understanding, or reach an [agreement] with the foremen.")
    else
      e.self:Say("Clontik needs your help. Return when your work brings you here.")
    end
    return
  end
  if npc_id ~= 482192 then return end
  for _, task in ipairs(tasks) do
    if message:match("%f[%a]"..task.phrase.."%f[%A]") then
      if eligible(e.other, task.prerequisites) then
        eq.task_selector({task.id})
      else
        e.self:Say("Complete the earlier work for me before taking this assignment.")
      end
      return
    end
  end
end
function M.trade(e)
  return delivery.trade(e)
end
return M
