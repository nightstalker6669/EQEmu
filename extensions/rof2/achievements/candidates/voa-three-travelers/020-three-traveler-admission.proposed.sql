-- SOURCE PROPOSAL ONLY. Not installed, run, or updater-registered.
-- Reuse three exact existing ZoneEnter/Boolean rules; insert no new criteria.
-- Requires separately reviewed normal access first. Parent1900050 stays unchanged.
DELIMITER $$
CREATE PROCEDURE admit_voa_three_travelers()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 IF (SELECT COUNT(*)FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE()
 AND TABLE_NAME IN('achievements','achievement_components','achievement_associations','achievement_criteria','reward_sources','reward_source_entries')AND ENGINE='InnoDB')<>6
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler admission requires transactional catalogue'; END IF;
 START TRANSACTION;
 IF (SELECT COUNT(*)FROM achievements WHERE
 ((id=1973000 AND BINARY name=BINARY 'The Pillars of Alra Traveler')OR
  (id=1973100 AND BINARY name=BINARY 'The Windsong Sanctuary Traveler')OR
  (id=1973300 AND BINARY name=BINARY 'The Sepulcher of Order Traveler'))
 AND points=10 AND has_reward=0 AND enabled IN(0,1)AND version=0)<>3
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler definition identity or reward/version changed'; END IF;
 IF (SELECT COUNT(*)FROM achievement_components WHERE achievement_id IN(1973000,1973100,1973300))<>3
 OR (SELECT COUNT(*)FROM achievement_components WHERE component_type=1 AND sequence=1 AND component_id=achievement_id AND description=''AND
 ((achievement_id=1973000 AND BINARY name=BINARY 'Visit The Pillars of Alra')OR
  (achievement_id=1973100 AND BINARY name=BINARY 'Visit Windsong Sanctuary')OR
  (achievement_id=1973300 AND BINARY name=BINARY 'Visit Sepulcher of Order')))<>3
 OR EXISTS(SELECT 1 FROM achievement_associations WHERE component_id IN(1973000,1973100,1973300)AND required_count<>1)
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler required component shape changed'; END IF;
 IF (SELECT COUNT(*)FROM achievement_criteria WHERE achievement_id IN(1973000,1973100,1973300))<>3
 OR (SELECT COUNT(*)FROM achievement_criteria WHERE component_type=1 AND component_sequence=1 AND component_id=achievement_id
 AND event_type=5 AND progress_mode=3 AND behavior=0 AND target_id2=0 AND target_value=0 AND required_count=1 AND enabled IN(0,1)AND
 ((achievement_id=1973000 AND target_id=730)OR(achievement_id=1973100 AND target_id=731)OR(achievement_id=1973300 AND target_id=733)))<>3
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler existing ZoneEnter criteria changed'; END IF;
 IF EXISTS(SELECT 1 FROM reward_sources WHERE source_type=1 AND source_id IN(1973000,1973100,1973300))
 OR EXISTS(SELECT 1 FROM reward_source_entries WHERE source_type=1 AND source_id IN(1973000,1973100,1973300))
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler no-reward policy changed'; END IF;
 IF (SELECT COUNT(*)FROM zone WHERE version=0 AND zoneidnumber IN(730,731,733))<>3
 OR (SELECT COUNT(*)FROM zone WHERE version=0 AND min_status=0 AND
 ((zoneidnumber=730 AND BINARY short_name=BINARY 'pillarsalra')OR(zoneidnumber=731 AND BINARY short_name=BINARY 'windsong')OR(zoneidnumber=733 AND BINARY short_name=BINARY 'sepulcher')))<>3
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler normal zone access not reviewed/applied'; END IF;
 UPDATE achievement_criteria SET enabled=1 WHERE achievement_id IN(1973000,1973100,1973300);
 UPDATE achievements SET enabled=1 WHERE id IN(1973000,1973100,1973300);
 COMMIT;
END$$
DELIMITER ;
CALL admit_voa_three_travelers();
DROP PROCEDURE admit_voa_three_travelers;
