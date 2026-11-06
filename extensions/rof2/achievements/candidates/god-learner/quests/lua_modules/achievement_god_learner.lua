-- Sequential, language-gated acquisition only. Achievement trigger awaits
-- observed grant-vs-read evidence; neither possession nor click attempt credits.
local D=require('achievement_god_learner_data')
local Books=require('achievement_god_learner_books')
local M={}
local KEY='achievement.god_learner.v1.acquisition_stage'
local function owned(c,id)
 local inv=c:GetInventory()
 if not inv then return nil end
 for _,name in ipairs({'Personal','Bank','SharedBank','Trading','Cursor'})do
  local where=InventoryWhere and InventoryWhere[name]
  if type(where)~='number'then return nil end
  local slot=inv:HasItem(id,1,where)
  if type(slot)~='number'or slot~=math.floor(slot)or slot< -1 then return nil end
  if slot>=0 then return true end
 end
 return false
end
function M.say(e)
 local c=e.other
 if not D.enabled or not c or not c.valid or not e.message:lower():find('hail',1,true)then return false end
 local binding,index
 for i,s in ipairs(D.stages)do
  if s.zone==eq.get_zone_short_name()and s.npc==e.self:GetNPCTypeID()and s.spawn==e.self:GetSpawnPointID()then binding,index=s,i;break end
 end
 if not binding or eq.get_zone_instance_id()~=0 then return false end
 if c:GetLanguageSkill(D.language_id)<D.language_required then c:Message(15,'There is no response from the stone.');return true end
 local raw=c:GetBucket(KEY)
 local stage=(raw==nil or raw=='')and 0 or tonumber(raw)
 if not stage or stage~=math.floor(stage)or stage<0 or stage>#D.stages then
  c:Message(13,'Your sentry stone history needs administrator review.');return true
 end
 if stage~=index-1 then c:Message(15,'There is no response from the stone.');return true end
 local held=owned(c,binding.item)
 if held==nil then c:Message(13,'Your existing shard could not be checked.');return true end
 if not held then c:SummonItem(binding.item)end
 -- Includes queued cursor; no new item if an existing Lore shard is banked.
 if owned(c,binding.item)~=true then c:Message(13,'The shard could not be delivered.');return true end
 c:SetBucket(KEY,tostring(index))
 -- Readback is cache confirmation ONLY, not a durable database receipt.
 -- No item is consumed or achievement credit manufactured on this path.
 if c:GetBucket(KEY)~=tostring(index)then c:Message(13,'Your sentry stone history could not be saved.');return true end
 c:Message(15,'We wish you to have this. We wish to be heard.')
 return true
end
-- Validates the concrete item instance in the native supplied click slot.
-- This is lore output ONLY, never achievement credit or a completed-spell claim.
function M.read_output(e)
 if not D.enabled or not e.owner or not e.owner.valid or not e.self or not e.self.valid then return false end
 if type(e.slot_id)~='number'or e.slot_id<0 or e.slot_id~=math.floor(e.slot_id)then return false end
 local id=e.self:GetID();local binding
 for _,s in ipairs(D.stages)do if s.item==id then binding=s;break end end
 if not binding then return false end
 local inv=e.owner:GetInventory();if not inv then return false end
 local actual=inv:GetItem(e.slot_id)
 if not actual or not actual.valid or actual:GetID()~=id or actual:GetSerialNumber()~=e.self:GetSerialNumber()then return false end
 local book=Books[id]
 if not book or book.verified~=true or type(book.text)~='string'or book.text==''or book.text:find(string.char(0),1,true)then return false end
 -- Renderer must be explicitly verified. Neither API return is a UI-read ACK.
 if book.renderer=='popup'and type(book.title)=='string'and book.title~=''and #book.title<=127 and #book.text<=4095 and not book.title:find(string.char(0),1,true)then
  e.owner:Popup(book.title,book.text)
 elseif book.renderer=='book'and type(book.book_type)=='number'and book.book_type>=0 and book.book_type<=2 and #book.text<=8193 and book.book_type==math.floor(book.book_type)then
  e.owner:QuestReadBook(book.text,book.book_type)
 else return false end
 -- Infinite component shards and script-only Three are never consumed here.
 -- No SummonItem/DeleteItem/SetBucket/SetAchievementProgress on reading.
 return true
end
return M
