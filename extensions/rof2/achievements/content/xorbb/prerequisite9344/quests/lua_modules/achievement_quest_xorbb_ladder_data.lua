-- The model is researched; its scene pose and climb route are not established.
-- Supply verified coordinates/heading/type/size together before enabling this.
-- No ground-landmark coordinates or model-origin guesses are placement defaults.
return {geometry_ready=false,model='OBJ_LADDER',
 -- Current configured dynamic quest-tree reference; retain its existing door.
 -- This is not the large decorative OBP_fob_tree model or a terrain floor.
 tree={native_door_id=1,model='OBJ_CAMP_TREE_A',x=-1198.07,y=703.542,
  z=235.9,heading=472,incline=125,size=25,open_type=55,tolerance=0.05}}
-- Verified use_x/use_y/use_z (ground interaction) must be supplied separately
-- from x/y/z (off-centre ladder model pivot). No implicit pivot-to-feet guess.
-- Incline is an independent raw native door pose field. Existing tree125 and
-- pickup ladder130 were omitted by earlier upright mesh diagnostics. Neither
-- a degree conversion nor a standing-ladder incline is assumed here.
