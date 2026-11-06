-- RESERVED9342. PRIVATE DISABLED candidate; not installed or SQL-executed.
-- Boolean predicate native Alaran language >=100 is an emulator representation.
-- Qualifying primer/training trigger is unverified; do not enable on login or chat alone.
-- Existing parent, optional rows and all character/reward records are preserved.
CREATE PROCEDURE install_alaran_master_9342_candidate()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 IF (SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE()
     AND TABLE_NAME='achievement_criteria' AND ENGINE='InnoDB')<>1
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran Master requires transactional criteria'; END IF;
 START TRANSACTION;
 IF NOT EXISTS(SELECT 1 FROM achievements WHERE id=1900049
     AND name='Alaran Language Master' AND points=10 AND enabled=1)
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran Master parent identity mismatch'; END IF;
 IF (SELECT COUNT(*) FROM achievement_components WHERE achievement_id=1900049 AND component_type=1)<>1
 OR NOT EXISTS(SELECT 1 FROM achievement_components WHERE achievement_id=1900049
     AND component_type=1 AND sequence=1 AND component_id=1900049
     AND name=CONVERT(0x416c6172616e204c616e67756167653a20313030202d2055706461746520627920636c69636b696e67206c616e6775616765207072696d657220626f6f6b73 USING utf8mb4) AND description='')
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran Master required identity mismatch'; END IF;
 IF EXISTS(SELECT 1 FROM achievement_criteria WHERE achievement_id=1900049)
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran Master criterion collision'; END IF;
 IF EXISTS(SELECT 1 FROM reward_sources WHERE source_type=1 AND source_id=1900049)
 OR EXISTS(SELECT 1 FROM reward_source_entries WHERE source_type=1 AND source_id=1900049)
 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Alaran Master reward policy changed; preserve and review'; END IF;
 INSERT INTO achievement_criteria
 (achievement_id,component_type,component_sequence,component_id,event_type,progress_mode,
  behavior,target_id,target_id2,target_value,required_count,enabled)
 VALUES(1900049,1,1,1900049,0,3,0,0,0,0,1,0);
 COMMIT;
END;
CALL install_alaran_master_9342_candidate();
DROP PROCEDURE install_alaran_master_9342_candidate;
