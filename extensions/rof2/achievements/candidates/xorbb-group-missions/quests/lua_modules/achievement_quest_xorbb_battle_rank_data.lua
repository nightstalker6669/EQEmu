-- Configurable emulator rank defaults; not recovered original thresholds/stats.
-- Four-cycle tiers and300s replacements are defaults, NOT the June2postcutoff
-- comment promoted to historical proof. Actual controller respawn receipts required.
return {enabled=false,replacement_seconds=300,
 challenge_roles={enemy_trooper=true,enemy_commander=true,enemy_lord=true},
 ranks={
  [0]={prefix="",minimum_camp_defeats=0,scale=1},
  [1]={prefix="Lordly",minimum_camp_defeats=4,scale=1.25,achievement=2075370},
  [2]={prefix="Continent Subjugator",minimum_camp_defeats=8,scale=1.5,achievement=2075371},
  [3]={prefix="Dimensional Conqueror",minimum_camp_defeats=12,scale=1.75,achievement=2075372},
  [4]={prefix="Supreme God Killer",minimum_camp_defeats=16,scale=2,achievement=2075373},
 },roles={
  enemy_bruiser={name="a bruiser",family="soldier",max_hp=650000,min_hit=800,max_hit=6500,ac=1600,atk=1500},
  enemy_commander={name="a commander",family="commander",max_hp=1200000,min_hit=1200,max_hit=9000,ac=1800,atk=1700},
  enemy_defender={name="a defender",family="soldier",max_hp=650000,min_hit=800,max_hit=6500,ac=1600,atk=1500},
  enemy_flame_ranger={name="a flame ranger",family="soldier",max_hp=650000,min_hit=800,max_hit=6500,ac=1600,atk=1500},
  enemy_healer={name="a healer",family="soldier",max_hp=650000,min_hit=800,max_hit=6500,ac=1600,atk=1500},
  enemy_lord={name="a lord",family="commander",max_hp=1200000,min_hit=1200,max_hit=9000,ac=1800,atk=1700},
  enemy_trooper={name="a trooper",family="soldier",max_hp=650000,min_hit=800,max_hit=6500,ac=1600,atk=1500},
  enemy_exploder={name="an exploder",family="soldier",max_hp=650000,min_hit=800,max_hit=6500,ac=1600,atk=1500},
  powershard_golem={name="a powershard golem",family="powershard_golem",max_hp=4000000,min_hit=2200,max_hit=15500,ac=2000,atk=1900},
}}
