-- PROPOSED permanent training integration. Review/deploy with all three Lua files.
-- No updater/version registration; no character, optional component or reward writes.
-- Accepts fresh/disabled research candidates or an exact already-enabled rerun.
-- Preflight requires this routine name to be absent; never overwrite a foreign routine.
DELIMITER $$
CREATE PROCEDURE install_alaran_language_training()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION
 BEGIN
  ROLLBACK;
  DROP TEMPORARY TABLE IF EXISTS alaran_training_expected;
  RESIGNAL;
 END;
 IF (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE()
 AND TABLE_NAME IN('achievements','achievement_components','achievement_associations',
                  'achievement_criteria','reward_sources','reward_source_entries')
 AND ENGINE='InnoDB')<>6
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran training requires six transactional catalogue tables'; END IF;
 START TRANSACTION;
 CREATE TEMPORARY TABLE alaran_training_expected(
  achievement_id INT UNSIGNED PRIMARY KEY,
  achievement_name VARCHAR(128) NOT NULL,
  component_name VARCHAR(255) NOT NULL
 );
 INSERT INTO alaran_training_expected VALUES
 (1900045,'Alaran Language Aware','Alaran Language: 25'),
 (1900046,'Alaran Language Novice','Alaran Language: 50'),
 (1900047,'Alaran Language Fluent','Alaran Language: 71'),
 (1900048,'Alaran Language Expert','Alaran Language: 90'),
 (1900049,'Alaran Language Master','Alaran Language: 100 - Update by clicking language primer books');
 IF (SELECT COUNT(*) FROM achievements a JOIN alaran_training_expected e
 ON e.achievement_id=a.id WHERE BINARY a.name=BINARY e.achievement_name
 AND a.points=10 AND a.has_reward=0 AND a.enabled IN(0,1) AND a.version=0)<>5
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran training parent identity or reward/version changed'; END IF;
 IF (SELECT COUNT(*) FROM achievement_components WHERE achievement_id BETWEEN 1900045 AND 1900049 AND component_type=1)<>5
 OR (SELECT COUNT(*) FROM achievement_components c JOIN alaran_training_expected e
 ON e.achievement_id=c.achievement_id WHERE c.component_type=1 AND c.sequence=1
 AND c.component_id=e.achievement_id AND BINARY c.name=BINARY e.component_name AND c.description='')<>5
 OR EXISTS(SELECT 1 FROM achievement_associations a JOIN alaran_training_expected e ON a.component_id=e.achievement_id WHERE a.required_count<>1)
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran training required component shape changed'; END IF;
 IF EXISTS(SELECT 1 FROM achievement_criteria WHERE achievement_id BETWEEN 1900045 AND 1900049
 AND NOT(component_type=1 AND component_sequence=1 AND component_id=achievement_id
 AND event_type=0 AND progress_mode=3 AND behavior=0 AND target_id=0 AND target_id2=0
 AND target_value=0 AND required_count=1 AND enabled IN(0,1)))
 OR EXISTS(SELECT achievement_id FROM achievement_criteria WHERE achievement_id BETWEEN 1900045 AND 1900049 GROUP BY achievement_id HAVING COUNT(*)>1)
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran training foreign or duplicate criterion'; END IF;
 IF EXISTS(SELECT 1 FROM reward_sources WHERE source_type=1 AND source_id BETWEEN 1900045 AND 1900049)
 OR EXISTS(SELECT 1 FROM reward_source_entries WHERE source_type=1 AND source_id BETWEEN 1900045 AND 1900049)
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran training reward policy changed'; END IF;
 INSERT INTO achievement_criteria(achievement_id,component_type,component_sequence,
 component_id,event_type,progress_mode,behavior,target_id,target_id2,target_value,required_count,enabled)
 SELECT e.achievement_id,1,1,e.achievement_id,0,3,0,0,0,0,1,1 FROM alaran_training_expected e
 WHERE NOT EXISTS(SELECT 1 FROM achievement_criteria c WHERE c.achievement_id=e.achievement_id) ORDER BY e.achievement_id;
 UPDATE achievement_criteria SET enabled=1 WHERE achievement_id BETWEEN 1900045 AND 1900049;
 UPDATE achievements SET enabled=1 WHERE id BETWEEN 1900045 AND 1900049;
 DROP TEMPORARY TABLE alaran_training_expected;
 COMMIT;
END$$
DELIMITER ;
CALL install_alaran_language_training();
DROP PROCEDURE install_alaran_language_training;
