SET NAMES utf8mb4;
DELIMITER //
BEGIN NOT ATOMIC
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK;RESIGNAL;END;
 START TRANSACTION;

UPDATE task_activities SET target_name='a mesa widow',description_override='Kill 4 mesa widows' WHERE taskid=3040461 AND activityid=2 AND target_name='' AND description_override='Defeat the requested creatures';
IF ROW_COUNT()<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Centaur journal-label row conflict';END IF;
UPDATE task_activities SET target_name='a highland snake',description_override='Kill 8 highland snakes' WHERE taskid=3040461 AND activityid=3 AND target_name='' AND description_override='Defeat the requested creatures';
IF ROW_COUNT()<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Centaur journal-label row conflict';END IF;
UPDATE task_activities SET target_name='harpies',description_override='Kill 2 harpies' WHERE taskid=3040461 AND activityid=4 AND target_name='' AND description_override='Defeat the requested creatures';
IF ROW_COUNT()<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Centaur journal-label row conflict';END IF;
UPDATE task_activities SET target_name='centaur thugs',description_override='Reduce the Thug Menace' WHERE taskid=3040462 AND activityid=0 AND target_name='' AND description_override='Defeat the requested creatures';
IF ROW_COUNT()<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Centaur journal-label row conflict';END IF;
UPDATE task_activities SET target_name='shade of corruption',description_override='Destroy the source of the corruption' WHERE taskid=3040464 AND activityid=0 AND target_name='' AND description_override='Defeat the requested creatures';
IF ROW_COUNT()<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Centaur journal-label row conflict';END IF;
COMMIT;
END//
DELIMITER ;
