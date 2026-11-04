-- Source geometry guard. Continuous floor/ray checks are useful but do not prove
-- a model capsule fits or a native NPC route works; explicit acceptance gate stays.
local M={}
local function finite(v)return type(v)=='number'and v==v and math.abs(v)<99990 end
function M.points(giver,offsets,policy)
 if policy.geometry_ready~=true then return nil,'scene geometry has not been reviewed for activation'end
 if not finite(policy.sample_spacing)or policy.sample_spacing<0.1 or policy.sample_spacing>4
  or not finite(policy.max_anchor_floor_delta)or policy.max_anchor_floor_delta<=0
  or not finite(policy.max_floor_delta)or policy.max_floor_delta<=0
  or not finite(policy.max_floor_step)or policy.max_floor_step<=0
  or not finite(policy.model_size)or policy.model_size<=0 then return nil,'invalid geometry policy'end
 if not giver or not giver.valid or type(offsets)~='table'or #offsets==0 or #offsets>16 then return nil,'missing placement source'end
 local gx,gy,gz=giver:GetX(),giver:GetY(),giver:GetZ()
 if not(finite(gx)and finite(gy)and finite(gz))then return nil,'invalid giver position'end
 local shift=policy.anchor_offset or {x=0,y=0}
 if not finite(shift.x)or not finite(shift.y)or shift.x^2+shift.y^2>10000 then return nil,'invalid floor anchor'end
 local ax,ay=gx+shift.x,gy+shift.y
 local base=giver:FindGroundZ(ax,ay,0) -- third argument is caller-relative OFFSET
 if not finite(base)or math.abs(base-gz)>policy.max_anchor_floor_delta then return nil,'invalid anchor floor'end
 local out={}
 for _,offset in ipairs(offsets)do
  if not finite(offset.x)or not finite(offset.y)then return nil,'invalid configured offset'end
  local dx,dy=gx+offset.x-ax,gy+offset.y-ay
  local length=math.sqrt(dx^2+dy^2)
  if length>100 then return nil,'placement offset exceeds bounded scene scope'end
  local steps=math.max(1,math.ceil(length/policy.sample_spacing));local last=base
  for i=0,steps do
   local x,y=ax+dx*i/steps,ay+dy*i/steps
   local z=giver:FindGroundZ(x,y,0)
   if not finite(z)or math.abs(z-base)>policy.max_floor_delta or math.abs(z-last)>policy.max_floor_step
    or not giver:CheckLoSToLoc(x,y,z+0.1,policy.model_size)then return nil,'blocked or discontinuous placement segment'end
   last=z
  end
  out[#out+1]={x=gx+offset.x,y=gy+offset.y,z=last+0.1,heading=giver:GetHeading()}
 end
 return out
end
return M
