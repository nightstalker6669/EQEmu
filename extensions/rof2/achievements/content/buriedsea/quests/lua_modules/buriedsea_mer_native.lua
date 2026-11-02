local D=require('achievement_quest_buriedsea_data')
local R=require('buriedsea_mer_rules')
local json=require('json')
local M={}
local PREFIX='rof2_mer_trip_v1_'
local CURRENT='rof2_mer_current_trip_v1'
local function yes(v)return v==true or v==1 end
local function valid(v)return v and v.valid end
local function enabled()return D.enabled and R.enabled end
local function active(c)return valid(c)and yes(c:IsTaskActive(R.task))end
local function uid(c)
 local id=c:GetSharedTaskIdentity()
 if type(id)=='string'and id:match('^[1-9]%d*$')then return id end
end
local function key(id)return PREFIX..id end
local function read(c,id)
 if not id then return end
 local raw=c:GetBucket(key(id));if not raw or raw==''then return end
 local ok,s=pcall(json.decode,raw)
 assert(ok and type(s)=='table'and s.identity==id and s.owner==c:CharacterID()and
  s.task==R.task and type(s.instance)=='number'and s.instance>0 and s.instance<=65535,
  'Corrupt Mer travel state; refusing to consume a replacement ticket')
 return s
end
local function save(c,s)c:SetBucket(key(s.identity),json.encode(s));c:SetBucket(CURRENT,s.identity)end
local function near(n,p,xy,z)
 local dx,dy=n:GetX()-p.x,n:GetY()-p.y
 return dx*dx+dy*dy<=xy*xy and math.abs(n:GetZ()-p.z)<=z
end
local function slot(c,id)
 local value=c:GetInventory():HasItem(id,1,34)
 return value and value>=0 and value or nil
end
local function activity(c,id)return yes(c:IsTaskActivityActive(R.task,id))end
local function owned(c,id)
 return active(c)and uid(c)==id and c:GetSharedTaskInstanceID(R.task,R.zone,R.version)or 0
end
local function find_ship(rule)
 local found
 for n in eq.get_entity_list():GetNPCList().entries do
  if n:GetNPCTypeID()==rule.npc and n:GetSpawnPointID()==rule.spawn then
   assert(not found,'Duplicate Mer ferry spawn');found=n
  end
 end
 return found
end
local function move_to_mission(c,s)
 if owned(c,s.identity)~=s.instance then return end
 local p=R.entry;c:MovePCInstance(R.zone,s.instance,p.x,p.y,p.z,p.h)
end
function M.say(e)
 if not enabled()or eq.get_zone_id()~=422 or eq.get_zone_instance_id()~=0 then return end
 if e.self:GetNPCTypeID()==R.mer and not active(e.other)then
  if e.message:lower():find('help',1,true)then eq.task_selector({R.task})
  elseif e.message:lower():find('hail',1,true)or e.message:lower():find('renne',1,true)then
   e.other:Message(15,'Mer is searching for news of Renne. Ask her how you can help.')
  end
 end
end
function M.accepted(e)
 if not enabled()or e.task_id~=R.task or e.self:GetNPCTypeID()~=R.mer or not active(e.other)then return end
 -- Native world admission gives NPC context only to the actual requester.
 if e.other:CountItem(R.seal)==0 then e.other:SummonItem(R.seal,1)end
 M.resume({self=e.other})
end
function M.trade(e)
 local npc=e.self:GetNPCTypeID()
 if not enabled()or eq.get_zone_id()~=422 or eq.get_zone_instance_id()~=0 or(npc~=R.mer and npc~=R.jor)then
  e.self:ReturnHandinItems(e.other);return
 end
 local handin,required,items={},{},{}
 for i=1,4 do local item=e.trade['item'..i]
  if valid(item)then
   items[#items+1]=item
   local id=tostring(item:GetID());local count=item:IsStackable()and item:GetCharges()or 1
   handin[id]=(handin[id]or 0)+count
   local used=item:GetTaskDeliveredCount();if used>0 then required[id]=(required[id]or 0)+used end
  end
 end
 for _,coin in ipairs({'copper','silver','gold','platinum'})do if(e.trade[coin]or 0)>0 then handin[coin]=e.trade[coin]end end
 if next(required)then e.self:CheckHandin(e.other,handin,required,items)end
 e.self:ReturnHandinItems(e.other)
end
local function sail(c,boat)
 if not active(c)or c:GetPassengerBoatID()~=boat:GetID()or
  not(activity(c,1)or activity(c,2)or activity(c,3))then return end
 local id=uid(c);if not id then return end
 local instance=owned(c,id);if instance==0 then return end
 local s=read(c,id)
 if s then
  if s.instance~=instance then return end
  if s.phase=='ticket_consumed'then move_to_mission(c,s)end
  return
 end
 local at=slot(c,R.ticket);if not at then return end
 local item=c:GetInventory():GetItem(at)
 if not valid(item)or item:GetID()~=R.ticket then return end
 s={owner=c:CharacterID(),task=R.task,identity=id,instance=instance,
  phase='ticket_consuming',slot=at,serial=item:GetSerialNumber(),created_at=os.time()}
 save(c,s)
 -- Never replay an ambiguous consuming intent after disconnect/reload.
 local current=c:GetInventory():GetItem(at)
 if not valid(current)or current:GetID()~=R.ticket or current:GetSerialNumber()~=s.serial then return end
 c:DeleteItemInInventory(at,1,true)
 current=c:GetInventory():GetItem(at)
 if valid(current)and current:GetID()==R.ticket and current:GetSerialNumber()==s.serial then return end
 s.phase='ticket_consumed';save(c,s);move_to_mission(c,s)
end
local function offset(c,boat)
 local angle=boat:GetHeading()*math.pi*2/512
 local dx,dy=c:GetX()-boat:GetX(),c:GetY()-boat:GetY()
 return{x=dx*math.cos(angle)-dy*math.sin(angle),y=dx*math.sin(angle)+dy*math.cos(angle),
  z=c:GetZ()-boat:GetZ(),h=(c:GetHeading()-boat:GetHeading())%512,size=boat:GetSize()}
end
local function sail_back(c,boat)
 if c:GetPassengerBoatID()~=boat:GetID()then return end
 local s=read(c,c:GetBucket(CURRENT))
 if not s or(s.phase~='arrived'and s.phase~='return_ready'and s.phase~='return_expired')then return end
 s.phase='return_sailing';s.return_started=os.time();s.offset=offset(c,boat);save(c,s)
 -- Incoming ferry already exists. Its own original grid carries the player
 -- to Weatherby's dock; retain the observed same-model deck offset on arrival.
 local p=R.incoming;c:MovePC(422,p.x,p.y,p.z+s.offset.z,(p.h+s.offset.h)%512)
end
function M.waypoint(e)
 if not enabled()or eq.get_zone_instance_id()~=0 then return end
 local rule,fn
 if eq.get_zone_id()==422 then rule=R.outward;fn=sail
 elseif eq.get_zone_id()==423 then rule=R.returning;fn=sail_back end
 if not rule or e.self:GetNPCTypeID()~=rule.npc or e.self:GetSpawnPointID()~=rule.spawn or
  not near(e.self,rule.endpoint,10,10)then return end
 -- Position, not a guessed zero/one-based waypoint number, identifies exit.
 for c in eq.get_entity_list():GetClientList().entries do fn(c,e.self)end
end
function M.resume(e)
 if enabled()and(eq.get_zone_id()==R.zone or eq.get_zone_id()==422)and
  (active(e.self)or(e.self:GetBucket(CURRENT)or'')~='')then e.self:SetTimer(R.timer,1)end
end
function M.timer(e)
 if e.timer~=R.timer then return false end
 local c=e.self
 if not enabled()then c:StopTimer(R.timer);return true end
 local id=uid(c);local s=read(c,id)
 if eq.get_zone_id()==R.zone and s and owned(c,id)==eq.get_zone_instance_id()and s.instance==eq.get_zone_instance_id()then
  if s.phase=='ticket_consumed'then s.phase='arrived';save(c,s)end
  if s.phase=='arrived'and activity(c,1)then c:UpdateTaskActivity(R.task,1,1)end
  c:StopTimer(R.timer)
 elseif eq.get_zone_id()==422 and eq.get_zone_instance_id()==0 then
  s=read(c,c:GetBucket(CURRENT))
  if s and(s.phase=='return_sailing'or s.phase=='return_positioning'or s.phase=='return_positioned')and
   (type(s.return_started)~='number'or os.time()-s.return_started>120 or os.time()<s.return_started)then
   s.phase='return_expired';save(c,s);c:StopTimer(R.timer);return true
  end
  if s and s.phase=='return_sailing'then
   local boat=find_ship(R.incoming)
   if valid(boat)then
    local p=s.offset;local angle=boat:GetHeading()*math.pi*2/512
    local ratio=(p.size and p.size>0 and boat:GetSize()>0)and boat:GetSize()/p.size or 1
    s.phase='return_positioning';save(c,s)
    c:GMMove(boat:GetX()+ratio*(p.x*math.cos(angle)+p.y*math.sin(angle)),
     boat:GetY()+ratio*(-p.x*math.sin(angle)+p.y*math.cos(angle)),
     boat:GetZ()+ratio*p.z,(boat:GetHeading()+p.h)%512)
    s.phase='return_positioned';save(c,s)
   end
  elseif s and s.phase=='return_positioned'then
   local boat=find_ship(R.incoming)
   if valid(boat)and c:GetPassengerBoatID()==boat:GetID()then s.phase='returned';save(c,s);c:StopTimer(R.timer)end
  else c:StopTimer(R.timer)end
 end
 return true
end
function M.before_update(e)
 if e.task_id~=R.task then return 0 end
 if not enabled()then return 1 end
 local c=e.self;local a=e.activity_id
 if a==0 or a==3 then return(eq.get_zone_id()==422 and eq.get_zone_instance_id()==0)and 0 or 1 end
 local id=uid(c);local s=read(c,id)
 return(s and s.phase=='arrived'and eq.get_zone_id()==R.zone and
  s.instance==eq.get_zone_instance_id()and owned(c,id)==s.instance)and 0 or 1
end
function M.click_door(e)
 if not enabled()or eq.get_zone_id()~=R.zone or eq.get_zone_instance_id()==0 or
  e.door:GetDoorID()~=R.rowboat.id or e.door:GetDoorName()~=R.rowboat.name or
  not near(e.self,R.rowboat,35,30)then return false end
 local c=e.self;local s=read(c,uid(c))
 if s and s.phase=='arrived'then s.phase='return_ready';save(c,s)end
 local p=R.hook;c:MovePC(423,p.x,p.y,p.z,p.h)
 return true
end
function M.npc_death(e)
 if not enabled()or eq.get_zone_id()~=R.zone or eq.get_zone_instance_id()==0 or
  R.wreckage_sources[e.self:GetSpawnPointID()]~=e.self:GetNPCTypeID()then return 0 end
 local other=e.other
 local c=valid(other)and yes(other:IsClient())and other:CastToClient()or nil
 if not active(c)or owned(c,uid(c))~=eq.get_zone_instance_id()then return 0 end
 local limit=e.self:CountLoot()
 for _=1,limit do local before=e.self:CountLoot();e.self:RemoveItem(R.wreckage);if e.self:CountLoot()==before then break end end
 e.self:AddItem(R.wreckage,1)
 return 0 -- Native corpse, loot and shared delivery own all remaining credit.
end
return M
