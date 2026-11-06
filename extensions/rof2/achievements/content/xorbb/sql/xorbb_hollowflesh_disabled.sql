-- OFFLINE DISABLED recipe proposal. No migration registration/activation.
-- Formula/container/output: researched mutable RoF guide, Nov2012 topic.
-- Skill75/zero skill and trivial/nofail/keep container/no learning:
-- labelled configurable emulator defaults; original exact flags unknown.
CREATE PROCEDURE xorbb_hollowflesh_disabled_candidate()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK;RESIGNAL;END;
 IF(SELECT COUNT(*)FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE()
 AND TABLE_NAME IN('tradeskill_recipe','tradeskill_recipe_entries')AND ENGINE='InnoDB')<>2
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Transactional recipe tables required';END IF;
 START TRANSACTION;
 IF EXISTS(SELECT 1 FROM tradeskill_recipe WHERE id=3040485)
 OR EXISTS(SELECT 1 FROM tradeskill_recipe_entries WHERE recipe_id=3040485 OR(item_id=68484 AND successcount>0)OR(item_id=68480 AND iscontainer=1))
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Hollowflesh namespace/output/container collision';END IF;
 IF(SELECT COUNT(*)FROM items WHERE(id=68480 AND Name='Runed Coffin'AND bagtype=13 AND bagslots=10)
 OR(id=68484 AND Name='Hollowflesh Golem')OR(id=68482 AND Name='Minotaur Head')
 OR(id=68481 AND Name='Minotaur Torso')OR(id=68483 AND Name='Minotaur Limb')
 OR(id=68485 AND Name='Wiry Pondweed'))<>6
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Hollowflesh original item identity mismatch';END IF;
 INSERT INTO tradeskill_recipe(id,name,tradeskill,skillneeded,trivial,nofail,replace_container,notes,must_learn,quest,enabled,min_expansion,max_expansion)
 VALUES(3040485,'Hollowflesh Golem',75,0,0,1,0,'Researched formula; configurable emulator skill/nofail/container flags, original unverified',0,0,0,19,-1);
 INSERT INTO tradeskill_recipe_entries(recipe_id,item_id,successcount,failcount,componentcount,salvagecount,iscontainer)VALUES
 (3040485,68480,0,0,0,0,1),
 (3040485,68482,0,0,3,0,0),
 (3040485,68481,0,0,1,0,0),
 (3040485,68483,0,0,3,0,0),
 (3040485,68485,0,0,3,0,0),
 (3040485,68484,1,0,0,0,0);
 COMMIT;
END;
CALL xorbb_hollowflesh_disabled_candidate();
DROP PROCEDURE xorbb_hollowflesh_disabled_candidate;
