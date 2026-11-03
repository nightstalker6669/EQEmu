-- Existing configured spawn identities, matched to researched /loc landmarks.
-- Database placement is not rendered/native acceptance. No NPCs are moved.
return {
 enabled=false, -- PRIVATE candidate, no deployment/credit trigger acceptance.
 achievement_id=1973361,language_id=25,language_required=100,
 stages={
  {zone='sepulcher',npc=733012,spawn=146673,item=66766,component=20000},
  {zone='windsong',npc=731031,spawn=148258,item=66767,component=20001},
  {zone='pillarsalra',npc=730226,spawn=147801,item=66768,component=20002},
  {zone='cityofbronze',npc=732006,spawn=145886,item=66791}, -- mandatory hidden Three
  {zone='resplendent',npc=729055,spawn=144922,item=66769,component=20003},
  {zone='sarithcity',npc=726049,spawn=145530,item=66770,component=20004},
  {zone='beastdomain',npc=728104,spawn=144804,item=66771,component=20005},
  {zone='arelis',npc=725026,spawn=144166,item=66772,component=20006},
  {zone='argath',npc=724031,spawn=142791,item=66773,component=20007}
 }
}
