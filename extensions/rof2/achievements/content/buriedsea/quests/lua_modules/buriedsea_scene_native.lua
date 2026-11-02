-- Actual custody and native actors only; no fabricated kill/loot/delivery credit.
local D=require('achievement_quest_buriedsea_data')
local R=require('buriedsea_scene_rules')
local json=require('json')
local M={}
local PREFIX='rof2_buriedsea_scene_v1_'
local LEASE='buriedsea_scene_lease'
local SLOT='buriedsea_scene_slot'
local function yes(v)return v==true or v==1 end
local function valid(v)return v and v.valid end
local function config(task)
 for _,r in ipairs({R.coralisk,R.silverlip})do if r.task==task then return r end end
end
local function ready(c,r)
 local t=r and D.tasks[r.task]
 return D.enabled and R.enabled and t and t.enabled and t.runtime_dependencies_ready and
  eq.get_zone_instance_id()==0 and valid(c)and yes(c:IsTaskActive(r.task))
end
local function epoch(c,task)
 local v=c:GetBucket(PREFIX..'epoch_'..task)
 if not v or v==''then return end
 local n=tonumber(v);assert(n and n>=1 and n==math.floor(n),'Invalid Buried Sea acceptance epoch')
 return n
end
local function load(c,r)
 local ep=epoch(c,r.task);if not ep then return end
 local k=tostring(c:CharacterID())..'_'..r.task..'_'..ep
 local raw=eq.get_data(PREFIX..k);if not raw or raw==''then return end
 local ok,s=pcall(json.decode,raw)
 assert(ok and type(s)=='table'and s.key==k and s.owner==c:CharacterID()and
  s.task==r.task and s.epoch==ep and s.instance==0,'Corrupt Buried Sea encounter state; refusing replacement actors')
 return s
end
local function save(s)eq.set_data(PREFIX..s.key,json.encode(s))end
local function visible(c)return not yes(c:IsInvisible())and c:GetInvisibleUndeadLevel()==0 end
local function near(n,p,xy,z)
 local dx,dy=n:GetX()-p.x,n:GetY()-p.y
 return dx*dx+dy*dy<=xy*xy and math.abs(n:GetZ()-p.z)<=z
end
local function activity(c,task,id)return yes(c:IsTaskActivityActive(task,id))end
local function count(task,id)return eq.get_task_activity_done_count(task,id)end
local function actor(s,slot,npcid)
 local found
 for n in eq.get_entity_list():GetNPCList().entries do
  if n:GetEntityVariable(LEASE)==s.key and n:GetEntityVariable(SLOT)==tostring(slot)then
   assert(n:GetNPCTypeID()==npcid and not found,'Duplicate or invalid Buried Sea actor lease')
   found=n
  end
 end
 return found
end
local function spawn(s,slot,npcid,p,bill)
 local n=actor(s,slot,npcid)
 if n then return n end
 -- An interrupted spawning intent is deliberately not replayed. A crash
 -- cannot be resolved by minting another quest drop or another hostile NPC.
 local a=s.actors[tostring(slot)]
 if a.status~='planned'then return end
 a.status='spawning';save(s)
 local mob=eq.spawn2(npcid,0,0,p.x,p.y,p.z,p.heading or 0)
 if not valid(mob)then a.status='spawn_failed';save(s);return end
 n=mob:CastToNPC()
 assert(valid(n),'Buried Sea spawn was not an NPC')
 n:SetEntityVariable(LEASE,s.key);n:SetEntityVariable(SLOT,tostring(slot))
 n:SetEntityVariable('buriedsea_scene_owner',tostring(s.owner))
 n:SetEntityVariable('buriedsea_scene_task',tostring(s.task))
 n:SetEntityVariable('buriedsea_scene_epoch',tostring(s.epoch))
 -- Only the original troll supplies the bill, as a real corpse item.
 if bill then n:AddItem(R.silverlip.bill,1)end
 a.status='alive';a.entity_id=n:GetID();a.spawned_at=os.time();save(s)
 n:SetTimer(R.timer,1)
 return n
end
function M.accepted(c,task)
 local r=config(task);if not ready(c,r)then return end
 local ep=(epoch(c,task)or 0)+1
 c:SetBucket(PREFIX..'epoch_'..task,tostring(ep))
 local s={key=tostring(c:CharacterID())..'_'..task..'_'..ep,owner=c:CharacterID(),
  task=task,epoch=ep,instance=0,actors={},phase='accepted',attempts={}}
 save(s)
end
function M.needs_timer(c)
 for _,r in ipairs({R.coralisk,R.silverlip})do
  if ready(c,r)and epoch(c,r.task)then return true end
 end
 return false
end
function M.resupply(c,giver)
 local r=R.coralisk;local t=D.tasks[r.task]
 if eq.get_zone_id()~=423 or not ready(c,r)or not t or t.giver~=giver or
  not activity(c,r.task,r.dump_activity)then return end
 local s=load(c,r)
 if not s or(s.pending_dump and s.pending_dump.status=='consuming')then return end
 -- CountItem includes banks intentionally: never create duplicate lore buckets.
 if c:CountItem(r.bucket)==0 then c:SummonItem(r.bucket,1)end
end
local function advance_dump(c,r,s)
 local a=s.pending_dump;if not a then return end
 if a.status=='consuming'then return end -- Ambiguous custody: requires review.
 if a.status=='consumed'then
  local done=count(r.task,r.dump_activity)
  if done<a.target then
   if not activity(c,r.task,r.dump_activity)then return end
   c:UpdateTaskActivity(r.task,r.dump_activity,1)
  end
  if count(r.task,r.dump_activity)<a.target then return end
  a.status='credited';save(s)
 end
 if a.status=='credited'then
  if a.spawn then
   local slot='coralisk_'..a.target
   s.actors[slot]=s.actors[slot]or{status='planned',npc=r.npc};save(s)
   local n=spawn(s,slot,r.npc,r.point,false)
   if valid(n)and not yes(n:IsEngaged())then n:AddToHateList(c,1)end
  end
  s.attempts[tostring(a.target)]=a;s.pending_dump=nil;save(s)
 end
end
local function dump(c,r,s)
 if s.pending_dump then advance_dump(c,r,s);return end
 if not activity(c,r.task,r.dump_activity)or not visible(c)or yes(c:IsEngaged())or
  not near(c,r.point,r.horizontal_radius,r.vertical_radius)then return end
 local slot=c:GetInventory():HasItem(r.bucket,1,34)
 if not slot or slot<0 then return end
 local target=count(r.task,r.dump_activity)+1
 assert(target>=1 and target<=4,'Invalid slop activity count')
 if s.attempts[tostring(target)]then return end
 s.pending_dump={status='consuming',target=target,slot=slot,
  spawn=math.random(1,100)<=r.spawn_chance_percent};save(s)
 local item=c:GetInventory():GetItem(slot)
 if not valid(item)or item:GetID()~=r.bucket then return end
 local before=item:GetCharges()
 c:DeleteItemInInventory(slot,1,true)
 local after=c:GetInventory():GetItem(slot)
 if valid(after)and after:GetID()==r.bucket and after:GetCharges()>=before then return end
 s.pending_dump.status='consumed';save(s)
 advance_dump(c,r,s)
end
function M.package_handed(c,npc)
 local r=R.silverlip
 if eq.get_zone_id()~=422 or npc:GetNPCTypeID()~=r.recipient or not ready(c,r)or
  not activity(c,r.task,r.bill_activity)then return end
 local s=load(c,r);if not s or s.phase~='accepted'then return end
 s.phase='awaiting_ambush';s.due=os.time()+r.arrival_delay_seconds
 for i,a in ipairs(r.actors)do s.actors[tostring(i)]={status='planned',npc=a.npc}end
 save(s)
end
function M.recipient_trade(e)
 local r=R.silverlip;local handin,required,items={},{},{}
 local package_consumed=false
 for slot=1,4 do local item=e.trade['item'..slot]
  if valid(item)then
   items[#items+1]=item
   local id=tostring(item:GetID());local n=item:IsStackable()and item:GetCharges()or 1
   handin[id]=(handin[id]or 0)+n
   local used=item:GetTaskDeliveredCount()
   if used>0 then required[id]=(required[id]or 0)+used end
   if item:GetID()==r.package and used>0 then package_consumed=true end
  end
 end
 for _,coin in ipairs({'copper','silver','gold','platinum'})do if(e.trade[coin]or 0)>0 then handin[coin]=e.trade[coin]end end
 if next(required)and yes(e.self:CheckHandin(e.other,handin,required,items))and package_consumed then M.package_handed(e.other,e.self)end
 e.self:ReturnHandinItems(e.other)
 require('achievement_quest_buriedsea').resume({self=e.other})
end
function M.tick(c)
 for _,r in ipairs({R.coralisk,R.silverlip})do
  if ready(c,r)then local s=load(c,r)
   if s then
    if r==R.coralisk and eq.get_zone_id()==423 then dump(c,r,s)
    elseif r==R.silverlip and eq.get_zone_id()==422 and s.phase=='awaiting_ambush'and os.time()>=s.due then
     for i,a in ipairs(r.actors)do
      local n=spawn(s,tostring(i),a.npc,a,a.bill)
      if valid(n)and not yes(n:IsEngaged())then local p=r.destination;n:MoveTo(p.x,p.y,p.z,p.heading,false)end
     end
     s.phase='ambush_dispatched';save(s)
    end
   end
  end
 end
end
local function owned(n)
 local owner=tonumber(n:GetEntityVariable('buriedsea_scene_owner'))
 local task=tonumber(n:GetEntityVariable('buriedsea_scene_task'))
 local ep=tonumber(n:GetEntityVariable('buriedsea_scene_epoch'))
 if not owner or not task or not ep then return end
 local r=config(task);if not r then return end
 local key=tostring(owner)..'_'..task..'_'..ep
 if n:GetEntityVariable(LEASE)~=key then return end
 local raw=eq.get_data(PREFIX..key);if not raw or raw==''then return end
 local ok,s=pcall(json.decode,raw)
 if not ok or type(s)~='table'or s.key~=key or s.owner~=owner or s.task~=task or s.epoch~=ep then return end
 local slot=n:GetEntityVariable(SLOT);local a=s.actors[slot]
 if not a or a.npc~=n:GetNPCTypeID()or a.entity_id~=n:GetID()then return end
 return r,s,a
end
function M.npc_timer(e)
 if e.timer~=R.timer then return end
 local n=e.self;local r,s,a=owned(n)
 if not r then n:StopTimer(R.timer);return end
 -- Do not depop foreign actors or stop combat. Only our unengaged leased
 -- actors expire; original NPCs and other players' encounters are untouched.
 local now=os.time()
 if r==R.silverlip and not a.arrived and near(n,r.destination,r.arrival_distance,30)then a.arrived=now;save(s)end
 local c=eq.get_entity_list():GetClientByCharID(s.owner)
 if r==R.silverlip and a.arrived and ready(c,r)and epoch(c,r.task)==s.epoch and visible(c)and
  near(c,r.destination,r.aggro_distance,60)and not yes(n:IsEngaged())then n:AddToHateList(c,1)end
 local deadline=(r==R.silverlip and(a.arrived or a.spawned_at)or a.spawned_at)+r.unengaged_lifetime_seconds
 if now>=deadline and not yes(n:IsEngaged())then a.status='expired';save(s);n:Depop(false)end
end
function M.npc_death(e)
 local r,s,a=owned(e.self)
 if r then a.status='dead';save(s)end
 -- Never synthesize task kill credit or bill possession; native death/loot owns it.
 return 0
end
return M
