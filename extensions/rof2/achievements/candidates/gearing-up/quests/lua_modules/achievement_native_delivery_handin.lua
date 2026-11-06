-- Exclusive EVENT_TRADE owner for task-only deliveries. Native task delivery
-- runs first. Never call alongside another physical hand-in consumer.
local M={}
local function integer(n)return type(n)=='number'and n==n and n<math.huge and n>=0 and n==math.floor(n)end
function M.trade(e)
 local offered,required,instances={},{},{}
 local success,reason=pcall(function()
  for slot=1,4 do
   local inst=e.trade['item'..slot]
   if inst and inst.valid then
    local id=tostring(inst:GetID())
    local count=inst:IsStackable()and inst:GetCharges()or 1
    local used=inst:GetTaskDeliveredCount()
    if not integer(count)or count<1 or not integer(used)or used>count then error('invalid native delivered count')end
    instances[#instances+1]=inst
    offered[id]=(offered[id]or 0)+count
    if used>0 then required[id]=(required[id]or 0)+used end
   end
  end
  for _,coin in ipairs({'copper','silver','gold','platinum'})do offered[coin]=e.trade[coin]or 0 end
  -- Completion can already have removed the task. Use native receipts only.
  -- Known core gate: CheckHandin exact money equality and mutation-before-false
  -- require shared-owner review for item deliveries accompanied by coins.
  if next(required)then
   local accepted=e.self:CheckHandin(e.other,offered,required,instances)
   if accepted~=true and accepted~=1 then error('native CheckHandin rejected recorded task delivery')end
  end
 end)
 -- Exactly ONE return attempt, including read/check exceptions. A native
 -- exception may already have changed custody; never replay CheckHandin or
 -- return a guessed item count, and never manufacture a task-credit rollback.
 local returned,return_error=pcall(function()e.self:ReturnHandinItems(e.other)end)
 if not success or not returned then
  pcall(function()e.other:Message(13,'Your delivery needs administrator review. Do not repeat the hand-in.')end)
  return false,not success and tostring(reason)or tostring(return_error)
 end
 return true
end
return M
