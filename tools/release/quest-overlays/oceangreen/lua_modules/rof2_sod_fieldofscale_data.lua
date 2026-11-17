-- Existing item IDs from pinned Database content lookup; no item changes.
-- Actor, entrance and original policy bindings remain held.
local D={ready=true,phases={},entrances={},items={
 ["Akromid Crystal"]=17345,
 ["Bone Mask of the Ancient Iksar"]=80061,
 ["Chokidai Carcass"]=88307,
 ["Coated Iksar Ribcage"]=37763,
 ["Darkblade Longsword"]=17506,
 ["Darkened Drake Skull"]=88326,
 ["Dragonbane Enchantment"]=17346,
 ["Foot of the Master"]=37764,
 ["Iksar Bone"]=88306,
 ["Iksar Hip Bone"]=42773,
 ["Iksar Jawbone"]=42769,
 ["Iksar Left Foot Bones"]=42757,
 ["Iksar Left Hand Bones"]=42765,
 ["Iksar Left Lower Arm Bones"]=42762,
 ["Iksar Left Ribcage Bones"]=42763,
 ["Iksar Left Upper Arm Bones"]=42770,
 ["Iksar Left Upper Leg Bones"]=42774,
 ["Iksar Ribcage"]=17381,
 ["Iksar Right Hand Bones"]=42766,
 ["Iksar Right Lower Arm Bones"]=42768,
 ["Iksar Right Ribcage Bones"]=42764,
 ["Iksar Right Upper Arm Bones"]=42771,
 ["Iksar Right Upper Leg Bones"]=42775,
 ["Iksar Scourge Tail"]=88325,
 ["Iksar Tail Bones"]=42767,
 ["Petrified Skull of Commander Koth"]=17289,
 ["Purified Iksar Ribcage"]=17388,
 ["Scorpion Carcass"]=88308,
 ["Sebilisian Fire"]=40153,
 ["Shimmering Dragonbane Shield"]=17490,
 ["Skeletal Hand of Necropsy"]=40407,
 ["Snake Carcass"]=88309,
 ["Timeshear Red Fragment Azia"]=42869,
 ["Timeshear Red Fragment Beza"]=42870,
 ["Timeshear Red Fragment Caza"]=42871,
 ["Timeshear Red Fragment Dena"]=42872,
 ["Timeshear Red Fragment Ena"]=42873,
 ["Vial of Iksar Blood"]=88305,
 ["Viscid Tar"]=50004,
},messages={Raising=88318,SarnakGeneric=88310}}
-- Existing NPC-specific spell, selected from convergent original quest
-- behavior/name/effects; retained1000 range is not overwritten with live1200.
D.parley={cast_spell=17533,cast_range=1000,trakanon={npc=452079,spawn=129019},
 ganak={npc=452163,spawn=129251},guards={
 {npc=452159,spawn=129246},{npc=452160,spawn=129248},
 {npc=452161,spawn=129249},{npc=452167,spawn=129255}}}




-- Exact approved actor reservation and local policies. No entrance route invented.
local P=require("rof2_sod_fieldofscale_policy")
D.entrances={[18774]={["door_id"]=1,["missions"]={{["task"]=920211,["zone"]=455,["version"]=1},{["task"]=920214,["zone"]=455,["version"]=2}}},[18805]={["door_id"]=36,["missions"]={{["task"]=920212,["zone"]=454,["version"]=1},{["task"]=920215,["zone"]=454,["version"]=2}}},[18806]={["door_id"]=37,["missions"]={{["task"]=920212,["zone"]=454,["version"]=1},{["task"]=920215,["zone"]=454,["version"]=2}}},[18817]={["door_id"]=44,["missions"]={{["task"]=920213,["zone"]=453,["version"]=1}}}}
D.phases={["455:1"]={["key"]="Audience",["task"]=920211,["zone"]=455,["version"]=1,["initial"]={["guard"]={{["npc"]=452900,["spawn"]=304066000,["position"]={-225.5,-23.625,233.75,379.0}},{["npc"]=452900,["spawn"]=304066001,["position"]={-324.0,-74.125,234.97102189214846,459.0}},{["npc"]=452901,["spawn"]=304066002,["position"]={-130.875,-17.5,232.5898638964299,404.75}},{["npc"]=452901,["spawn"]=304066003,["position"]={-231.875,36.375,233.75,251.0}}}},["boss"]={["npc"]=452902,["position"]={-222.625,-23.25,263.75,378.0}},["transform_hp"]=60,["vitrik_race"]=620},["454:1"]={["key"]="Brood",["task"]=920212,["zone"]=454,["version"]=1,["initial"]={["egg"]={{["npc"]=452903,["spawn"]=304066004,["position"]={325.0,16.125,-131.63317229092493,382.0}},{["npc"]=452903,["spawn"]=304066005,["position"]={678.625,2.25,-23.374006271362305,378.0}},{["npc"]=452903,["spawn"]=304066006,["position"]={18.125,-34.75,2.609527526310719,426.0}},{["npc"]=452903,["spawn"]=304066007,["position"]={256.375,-275.125,-51.015869140625,251.0}},{["npc"]=452903,["spawn"]=304066008,["position"]={232.875,274.0,-55.18845748901367,439.0}},{["npc"]=452903,["spawn"]=304066009,["position"]={516.0,184.375,-51.490189437074875,381.0}},{["npc"]=452903,["spawn"]=304066010,["position"]={521.375,-164.25,-51.490699768066406,378.0}},{["npc"]=452903,["spawn"]=304066011,["position"]={104.875,131.625,-21.534927368164062,380.0}},{["npc"]=452903,["spawn"]=304066012,["position"]={134.0,-147.0,-21.534927368164062,501.0}},{["npc"]=452903,["spawn"]=304066013,["position"]={487.0,-10.875,-87.1919937133789,380.0}},{["npc"]=452903,["spawn"]=304066014,["position"]={318.5,-131.5,-87.9441146850586,310.0}},{["npc"]=452903,["spawn"]=304066015,["position"]={365.125,157.5,-87.9441146850586,129.0}},{["npc"]=452903,["spawn"]=304066016,["position"]={197.0,40.0,-96.83992767333984,254.0}},{["npc"]=452903,["spawn"]=304066017,["position"]={417.875,-102.25,-51.48392066662195,317.0}},{["npc"]=452903,["spawn"]=304066018,["position"]={410.0,67.75,-97.64300537109375,321.0}},{["npc"]=452903,["spawn"]=304066019,["position"]={142.5,3.75,-30.43932342529297,127.0}},{["npc"]=452903,["spawn"]=304066020,["position"]={589.875,-13.875,-25.874000549316406,355.0}},{["npc"]=452903,["spawn"]=304066021,["position"]={625.375,65.5,-23.374006271362305,274.0}},{["npc"]=452903,["spawn"]=304066022,["position"]={469.0,-219.625,-51.4907078280127,508.0}},{["npc"]=452903,["spawn"]=304066023,["position"]={17.625,35.125,2.506998916390618,331.0}},{["npc"]=452903,["spawn"]=304066024,["position"]={485.875,-29.125,-22.985084533691406,380.0}}}},["waves"]={{{["npc"]=452904,["spawn"]=nil,["position"]={18.125,-34.75,3.8595275263107194,426.0}},{["npc"]=452904,["spawn"]=nil,["position"]={34.5,-5.25,3.823935950935937,377.0}},{["npc"]=452904,["spawn"]=nil,["position"]={32.125,6.0,3.7930321374841567,378.0}},{["npc"]=452904,["spawn"]=nil,["position"]={17.625,35.125,3.7569989163906183,331.0}}},{{["npc"]=452905,["spawn"]=nil,["position"]={18.125,-34.75,5.1095275263107185,426.0}}},{{["npc"]=452906,["spawn"]=nil,["position"]={18.125,-34.75,3.8595275263107194,426.0}},{["npc"]=452906,["spawn"]=nil,["position"]={34.5,-5.25,3.823935950935937,377.0}}},{{["npc"]=452907,["spawn"]=nil,["position"]={18.125,-34.75,3.8595275263107194,426.0}}}},["wave_origin"]={18.125,-34.75,3.8595275263107194,426.0}},["453:1"]={["key"]="Knowledge",["task"]=920213,["zone"]=453,["version"]=1,["initial"]={["scholar"]={{["npc"]=452908,["spawn"]=304066025,["position"]={283.0,-2031.0,-156.25051635782776,217.0}},{["npc"]=452908,["spawn"]=304066026,["position"]={121.875,-2034.75,-164.26065234562995,275.0}},{["npc"]=452908,["spawn"]=304066027,["position"]={36.625,-2123.0,-196.25639941969933,399.0}},{["npc"]=452908,["spawn"]=304066028,["position"]={217.0,-2105.25,-196.25564575195312,2.0}},{["npc"]=452908,["spawn"]=304066029,["position"]={-2425.0,-1447.0,-756.259033203125,111.0}},{["npc"]=452908,["spawn"]=304066030,["position"]={-2434.875,-877.75,-766.2511496825828,134.0}}}},["beguilers"]={{["npc"]=452909,["spawn"]=nil,["position"]={199.5,-1853.0,-166.25137329101562,113.0}},{["npc"]=452909,["spawn"]=nil,["position"]={277.5,-2124.5,-196.25564575195312,144.0}},{["npc"]=452909,["spawn"]=nil,["position"]={-2505.375,-766.875,-766.2510375976562,458.0}}},["gordak"]={["npc"]=452910,["position"]={-2284.0,-991.0,-767.0027771765219,0.0}}},["455:2"]={["key"]="Phalanx",["task"]=920214,["zone"]=455,["version"]=2,["initial"]={["trooper"]={{["npc"]=452911,["spawn"]=304066031,["position"]={-243.5,-139.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066032,["position"]={-239.10137939453125,-139.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066033,["position"]={-248.5,-139.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066034,["position"]={-243.5,-144.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066035,["position"]={-243.5,-134.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066036,["position"]={-239.10137939453125,-144.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066037,["position"]={-239.10137939453125,-134.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066038,["position"]={-248.5,-144.875,45.75,0.0}},{["npc"]=452911,["spawn"]=304066039,["position"]={-244.25,120.75,45.75,0.0}},{["npc"]=452911,["spawn"]=304066040,["position"]={-249.25,120.75,45.75,0.0}},{["npc"]=452911,["spawn"]=304066041,["position"]={-244.25,115.75,45.75,0.0}},{["npc"]=452911,["spawn"]=304066042,["position"]={-244.25,125.75,45.75,0.0}},{["npc"]=452911,["spawn"]=304066043,["position"]={-239.25,120.75,45.75,0.0}},{["npc"]=452911,["spawn"]=304066044,["position"]={-249.25,115.75,45.75,0.0}},{["npc"]=452911,["spawn"]=304066045,["position"]={-249.25,125.75,45.75,0.0}}}}},["454:2"]={["key"]="Shield",["task"]=920215,["zone"]=454,["version"]=2,["initial"]={["protector"]={{["npc"]=452912,["spawn"]=304066046,["position"]={18.125,-34.75,3.8595275263107194,426.0}},{["npc"]=452913,["spawn"]=304066047,["position"]={516.0,184.375,-50.240189437074875,381.0}},{["npc"]=452914,["spawn"]=304066048,["position"]={487.0,-10.875,-85.9419937133789,380.0}},{["npc"]=452915,["spawn"]=304066049,["position"]={410.0,67.75,-95.76800537109375,321.0}}}},["guardian"]={["npc"]=452916,["position"]={672.625,-3.75,-20.374006094974114,0.0}},["shield_center"]={678.625,2.25,-17.874006271362305},["shield_half"]={20,20,12},["shield_object"]=304066000}}
D.drake={list={{["npc"]=453003,["spawn"]=125368},{["npc"]=453011,["spawn"]=125376},{["npc"]=453013,["spawn"]=125379},{["npc"]=453022,["spawn"]=125388},{["npc"]=453026,["spawn"]=125394},{["npc"]=453035,["spawn"]=125408},{["npc"]=453037,["spawn"]=125410},{["npc"]=453047,["spawn"]=125423},{["npc"]=453047,["spawn"]=125436},{["npc"]=453035,["spawn"]=125455},{["npc"]=453074,["spawn"]=125461},{["npc"]=453086,["spawn"]=125478},{["npc"]=453037,["spawn"]=125480},{["npc"]=453092,["spawn"]=125487},{["npc"]=453105,["spawn"]=125503},{["npc"]=453106,["spawn"]=125504},{["npc"]=453112,["spawn"]=125512},{["npc"]=453086,["spawn"]=125517},{["npc"]=453117,["spawn"]=125520},{["npc"]=453138,["spawn"]=125555},{["npc"]=453037,["spawn"]=125562},{["npc"]=453146,["spawn"]=125574},{["npc"]=453086,["spawn"]=125580},{["npc"]=453105,["spawn"]=125593},{["npc"]=453163,["spawn"]=125607},{["npc"]=453026,["spawn"]=125617},{["npc"]=453092,["spawn"]=125637},{["npc"]=453092,["spawn"]=125640},{["npc"]=453170,["spawn"]=125641},{["npc"]=453187,["spawn"]=125676},{["npc"]=453194,["spawn"]=125688},{["npc"]=453195,["spawn"]=125689},{["npc"]=453202,["spawn"]=125703}},actors={}}
for _,a in ipairs(D.drake.list) do D.drake.actors[a.npc]=D.drake.actors[a.npc] or {};D.drake.actors[a.npc][a.spawn]=true end
D.drake.selected_necromancer=function(c,n) return P.drake_selected(c,n,D.drake.list) end
D.messages={Raising=88318,SarnakSokokar=88310,SarnakBurynai=199900,SarnakKurn=199901}
D.world={calethos={["npc"]=452918,["position"]={-425.0,2858.0,-146.25,0.0}},lost={{["npc"]=452920,["spawn"]=nil,["position"]={-371.0,2318.0,-38.61526966094971,0.0}},{["npc"]=452920,["spawn"]=nil,["position"]={-355.0,2318.0,-42.40257930755615,0.0}},{["npc"]=452920,["spawn"]=nil,["position"]={-363.0,2334.0,-39.93337154388428,0.0}}}}
for _,b in pairs(D.phases) do
 if b.key=="Knowledge" then b.scholar_success=P.scholar_success end
 if b.key=="Brood" then b.first_wave_target_policy=function(e,c) local clients={};for x in eq.get_entity_list():GetClientList().entries do clients[#clients+1]=x end;return P.wave_target(clients,b.wave_origin,function(x) return x:IsTaskActive(b.task) and x:GetSharedTaskIdentity()==c:GetSharedTaskIdentity() and x:GetSharedTaskInstanceID(b.task,b.zone,b.version)==eq.get_zone():GetInstanceID() end) end end
 if b.key=="Shield" then b.verify_shield_source=function(c,s) return P.shield_source(c,s,b.shield_center,b.shield_half) end end
end
return D
