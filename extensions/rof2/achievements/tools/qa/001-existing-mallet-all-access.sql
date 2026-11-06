-- Local QA utility only; never part of the historical achievement content installer.
-- Requires the existing named QA item; does not create/replace an item or player copy.
DELIMITER //
BEGIN NOT ATOMIC
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK;RESIGNAL;END;
 START TRANSACTION;
 UPDATE items SET classes=65535,races=65535
 WHERE id=147495 AND Name='JARVIS Achievement Test Mallet' AND classes=16841 AND races=65515;
 IF ROW_COUNT()<>1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Existing QA mallet access differs; stop';END IF;
 COMMIT;
END//
DELIMITER ;
