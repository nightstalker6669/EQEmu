-- Correct all original same-name Highland Snake variants; no player-state edits.
DELIMITER //
BEGIN NOT ATOMIC
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 START TRANSACTION;
 IF (SELECT COUNT(*) FROM task_activities WHERE taskid=3040461 AND activityid=3 AND goalcount=8 AND activitytype=2 AND goalmethod=0 AND npc_match_list='397172')<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Highland Snake activity preimage differs';
 END IF;
 UPDATE task_activities SET npc_match_list='397172|397210|397230' WHERE taskid=3040461 AND activityid=3 AND npc_match_list='397172';
 IF ROW_COUNT()<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Highland Snake update failed'; END IF;
 COMMIT;
END//
DELIMITER ;
