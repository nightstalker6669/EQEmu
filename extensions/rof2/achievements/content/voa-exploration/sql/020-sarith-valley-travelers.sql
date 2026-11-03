-- Enable existing Sarith and Valley traveler achievements and ordinary zone admission.
-- Requires the installed achievement engine/catalogue; exact policy guards fail closed.
-- No NPC, task, reward, new criterion or player-state mutation.
DELIMITER $$
CREATE PROCEDURE apply_voa_sarith_valley_001()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 IF (SELECT COUNT(*)FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='zone' AND ENGINE='InnoDB')<>1
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler access requires transactional zone data'; END IF;
 IF (SELECT COUNT(*)FROM zone WHERE version=0 AND zoneidnumber IN(726,725))<>2
 OR (SELECT COUNT(*)FROM zone WHERE version=0 AND
 ((zoneidnumber=726 AND BINARY short_name=BINARY 'sarithcity')OR
  (zoneidnumber=725 AND BINARY short_name=BINARY 'arelis'))
 AND min_status IN(0,80) AND min_level=0 AND max_level=255 AND expansion=18
 AND flag_needed='' AND bypass_expansion_check=0 AND ruleset=1
 AND min_expansion=-1 AND max_expansion=-1
 AND content_flags IS NULL AND content_flags_disabled IS NULL)<>2
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler zone identity or existing admission policy changed'; END IF;
 UPDATE zone SET min_status=0 WHERE version=0 AND zoneidnumber IN(726,725)AND min_status=80;
 IF (SELECT COUNT(*)FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE()
 AND TABLE_NAME IN('achievements','achievement_components','achievement_associations','achievement_criteria','reward_sources','reward_source_entries')AND ENGINE='InnoDB')<>6
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler admission requires transactional catalogue'; END IF;
 IF (SELECT COUNT(*)FROM achievements WHERE
 ((id=1972600 AND BINARY name=BINARY 'Sarith, City of Tides Traveler')OR
  (id=1972500 AND BINARY name=BINARY 'Valley of Lunanyn Traveler'))
 AND points=10 AND has_reward=0 AND enabled IN(0,1)AND version=0)<>2
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler definition identity or reward/version changed'; END IF;
 IF (SELECT COUNT(*)FROM achievement_components WHERE achievement_id IN(1972600,1972500))<>2
 OR (SELECT COUNT(*)FROM achievement_components WHERE component_type=1 AND sequence=1 AND component_id=achievement_id AND description=''AND
 ((achievement_id=1972600 AND BINARY name=BINARY 'Visit Sarith, City of Tides')OR
  (achievement_id=1972500 AND BINARY name=BINARY 'Visit the Valley of Lunanyn')))<>2
 OR EXISTS(SELECT 1 FROM achievement_associations WHERE component_id IN(1972600,1972500)AND required_count<>1)
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler required component shape changed'; END IF;
 IF (SELECT COUNT(*)FROM achievement_criteria WHERE achievement_id IN(1972600,1972500))<>2
 OR (SELECT COUNT(*)FROM achievement_criteria WHERE component_type=1 AND component_sequence=1 AND component_id=achievement_id
 AND event_type=5 AND progress_mode=3 AND behavior=0 AND target_id2=0 AND target_value=0 AND required_count=1 AND enabled IN(0,1)AND
 ((achievement_id=1972600 AND target_id=726)OR(achievement_id=1972500 AND target_id=725)))<>2
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler existing ZoneEnter criteria changed'; END IF;
 IF EXISTS(SELECT 1 FROM reward_sources WHERE source_type=1 AND source_id IN(1972600,1972500))
 OR EXISTS(SELECT 1 FROM reward_source_entries WHERE source_type=1 AND source_id IN(1972600,1972500))
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler no-reward policy changed'; END IF;
 IF (SELECT COUNT(*)FROM zone WHERE version=0 AND zoneidnumber IN(726,725))<>2
 OR (SELECT COUNT(*)FROM zone WHERE version=0 AND min_status=0 AND
 ((zoneidnumber=726 AND BINARY short_name=BINARY 'sarithcity')OR(zoneidnumber=725 AND BINARY short_name=BINARY 'arelis')))<>2
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler normal zone access not reviewed/applied'; END IF;
 UPDATE achievement_criteria SET enabled=1 WHERE achievement_id IN(1972600,1972500);
 UPDATE achievements SET enabled=1 WHERE id IN(1972600,1972500);
 COMMIT;
END$$
DELIMITER ;
CALL apply_voa_sarith_valley_001();
DROP PROCEDURE apply_voa_sarith_valley_001;
