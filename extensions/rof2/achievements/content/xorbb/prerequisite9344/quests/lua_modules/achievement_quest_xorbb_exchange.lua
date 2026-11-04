-- Four carried facets become two rings at Thistlebrush's researched hail step.
-- Native task SpeakWith owns progression. No fabricated hand-in or reward payout.
local D=require('achievement_quest_xorbb_data')
local I=require('achievement_quest_mesa_inventory')
local M={}
local key='achievement.xorbb.facet_exchange.v1'
local task,facet,ring=3040475,66516,66517
local function yes(v)return v==true or v==1 end
local function state(c)
 local raw=c:GetBucket(key)
 if raw==''then return nil end
 local phase,f,r=raw:match('^v1|(%a+)|(%d+)|(%d+)$')
 if(phase~='pending'and phase~='done')or not f or not r then return {phase='unresolved'}end
 f,r=tonumber(f),tonumber(r)
 if not f or not r or f<4 or f>2147483647 or r>2147483645 then return {phase='unresolved'}end
 return {phase=phase,facets=f,rings=r}
end
local function write(c,phase,f,r)
 local raw=string.format('v1|%s|%d|%d',phase,f,r)
 c:SetBucket(key,raw)
 return c:GetBucket(key)==raw
end
local function active(c)
 return D.enabled==true and D.ready[task]==true and eq.get_zone_id()==753 and c and c.valid
  and yes(c:IsTaskActive(task))and yes(c:IsTaskActivityActive(task,10))
end
local function review(c)
 c:Message(13,'This facet exchange needs review before it can be retried. Your recorded exchange state has been retained.')
 return false
end
function M.requestable(c,id)
 if id~=task then return true end
 local s=state(c)
 return not s or s.phase=='done'
end
function M.accepted(c,id)
 if id~=task or not D.enabled or D.ready[task]~=true then return end
 local s=state(c)
 if s and s.phase~='done'then review(c);return end
 -- A genuine new accepted task may start a new exchange. Never clear an
 -- interrupted/unknown record merely because the player requests again.
 c:SetBucket(key,'')
end
function M.ready(c)
 local s=state(c)
 return active(c)and s and s.phase=='done'and I.count(c,facet)==s.facets-4
  and I.count(c,ring)==s.rings+2
end
function M.hail(e)
 local c=e.other
 if not active(c)or not e.self or not e.self.valid or e.self:GetNPCTypeID()~=D.giver then return false end
 if not tostring(e.message or ''):lower():find('hail',1,true)then return false end
 local s=state(c)
 if s then
  if s.phase=='done'then return M.ready(c)end
  return review(c)
 end
 if c:GetInventory():GetItem(Slot.Cursor).valid then c:Message(13,'Please empty your cursor first.');return false end
 local f,r=I.count(c,facet),I.count(c,ring)
 if f<4 then c:Message(13,'Bring the four Multiphasic Facets with you.');return false end
 if not write(c,'pending',f,r)then return review(c)end
 -- Confirm the new ring stack before consuming facets. An unsuccessful grant
 -- must leave the original four facets intact, with no automatic reissue.
 if not I.give(c,ring,2)or I.count(c,ring)~=r+2 then return review(c)end
 -- Only these four carried quest items are consumed. No bank/shared bank,
 -- unrelated items, currency or task history is edited.
 for _=1,4 do if not I.take_one(c,facet)then return review(c)end end
 if I.count(c,facet)~=f-4 then return review(c)end
 if not write(c,'done',f,r)then return review(c)end
 return M.ready(c)
end
return M
