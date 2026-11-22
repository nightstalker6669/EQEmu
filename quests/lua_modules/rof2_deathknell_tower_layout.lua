-- Coordinates follow the retained theatera placements and sampled tower floors.
local function voices(npc, p, emote)
    local points = {}
    for _, off in ipairs({{0,0},{3,0},{-3,0},{0,3},{0,-3}}) do
        points[#points + 1] = {p[1]+off[1],p[2]+off[2],p[3],p[4]}
    end
    return {npc=npc, points=points, emote=emote}
end

return {
    lockout_seconds = 432000,
    abandon_seconds = 45,
    host = {npc=381970, spawn=304088000},
    report_host = {npc=381970, spawn=304088001},
    doors = {[1]=304086537, [2]=304086538, [3]=304086539, [4]=304086540, [5]=304086541},
    vertigo_door = {[1]=true, [2]=true},
    lower_door = {[3]=true, [4]=true},
    events = {
        {name="The Tolling of Dissonance Bells", chest=381978, reward={28,-31,3,0},
         presence={0,0,4}, radius=195, height=36},
        {name="Vertigo", chest=381979, reward={110,6,-447,128},
         presence={0,0,-245}, radius=175, height=258},
        {name="Unwelcome Guests", chest=381980, reward={10,138,-627,0},
         presence={110,45,-595}, radius=200, height=80, proximity={112.787,-1.691,-547}},
        {name="The Mad Choir", chest=381981, reward={10,46,-757,0},
         presence={14.74,121,-756}, radius=133, height=22, proximity={14.74,41.76,-757}},
    },
    bells = {
        bell={npc=381972,spawn=304088002},
        interval=25, hold_pct=10, push_pct=20, steps_to_bell=5,
        add_interval=32, enforcer_interval=30, maximum_seconds=1800,
        vampire=381951,follower=381952,
        add_points={{-24,-74,4,0},{24,74,4,256},{74,-24,4,384},{-74,24,4,128}},
        watchers = {
            {npc=381946,point={-20,-130,7.75,0},stop={-20,-30}},
            {npc=381947,point={130,-20,7.75,384},stop={30,-20}},
            {npc=381948,point={20,130,7.75,256},stop={20,30}},
            {npc=381949,point={-130,20,7.75,128},stop={-30,20}},
        },
        enforcers = {
            {npc=381950,point={20,-130,7.75,0}}, {npc=381950,point={130,20,7.75,384}},
            {npc=381950,point={-20,130,7.75,256}}, {npc=381950,point={-130,-20,7.75,128}},
        },
    },
    vertigo = {
        keeper={npc=381953,point={-0.819,-117.218,-147,0}},
        sneak={npc=381955,point={-111.640,-1.124,-247,128}},
        bat={npc=381954,points={{-8,-116,-147,0},{8,-116,-147,0}},range=82},
        undead={
            {npc=381956,points={{-6,114,-347,256}},range=90},
            {npc=381971,points={{6,114,-347,256}},range=90},
        },
        orc={npc=381957,points={{119.324,-1.074,-447,384}},range=92},
        undead_interval=14, undead_quota=14, orc_interval=25, orc_quota=6, orc_distance=110,
    },
    guests = {
        add_interval=32, banshee_interval=20, maximum_seconds=1500, banshee=381964,
        bosses={
            {npc=381958,point={112.787,-1.691,-547,384},add=381961,add_point={119,-2,-547,384}},
            {npc=381959,point={-4,23,-627,0},add=381962,add_point={-10,23,-627,0}},
            {npc=381960,point={139,163,-627,256},add=381963,add_point={145,163,-627,256}},
        },
        passages={
            {{187.375,-28.625,-547,0},{187.375,0,-547,0,false}},
            {{72.125,-92.375,-754,128},{98,-92.375,-754,128,false}},
            {{-1.125,-66.25,-627,128},{14,-66.25,-627,128,false}},
        },
    },
    choir = {
        boss={npc=381965,point={19.059,202.960,-749,256}},
        fetters_interval=40, fetters_window=8, voice_interval=120, maximum_seconds=2700,
        parts={
            voices(381966,{-16,72,-757,0},"begins a complex harmony. The altos prepare to join her."),
            voices(381967,{16,72,-757,0},"sings over a low rumble. The basses prepare to join her."),
            voices(381968,{-16,100,-757,256},"builds to a disturbing crescendo. The sopranos prepare to join her."),
            voices(381969,{16,100,-757,256},"lets her melody soften. The tenors prepare to join her."),
        },
    },
}
