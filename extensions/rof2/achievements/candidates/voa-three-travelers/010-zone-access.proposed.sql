-- SOURCE PROPOSAL ONLY. Not installed, run, or updater-registered.
-- Review normal-world admission before use; no raid/task/DZ flags are granted.
-- Version0 zone data may also be used by instances; request/member gates remain
-- owned by their existing task/DZ paths and must not be bypassed during testing.
DELIMITER $$
CREATE PROCEDURE admit_voa_traveler_zone_access()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 IF (SELECT COUNT(*)FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='zone' AND ENGINE='InnoDB')<>1
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler access requires transactional zone data'; END IF;
 START TRANSACTION;
 IF (SELECT COUNT(*)FROM zone WHERE version=0 AND zoneidnumber IN(730,731,733))<>3
 OR (SELECT COUNT(*)FROM zone WHERE version=0 AND
 ((zoneidnumber=730 AND BINARY short_name=BINARY 'pillarsalra')OR
  (zoneidnumber=731 AND BINARY short_name=BINARY 'windsong')OR
  (zoneidnumber=733 AND BINARY short_name=BINARY 'sepulcher'))
 AND min_status IN(0,80) AND min_level=0 AND max_level=255 AND expansion=18
 AND flag_needed='' AND bypass_expansion_check=0 AND ruleset=1
 AND min_expansion=-1 AND max_expansion=-1
 AND content_flags IS NULL AND content_flags_disabled IS NULL)<>3
 THEN SIGNAL SQLSTATE '45000'SET MESSAGE_TEXT='Traveler zone identity or existing admission policy changed'; END IF;
 UPDATE zone SET min_status=0 WHERE version=0 AND zoneidnumber IN(730,731,733)AND min_status=80;
 COMMIT;
END$$
DELIMITER ;
CALL admit_voa_traveler_zone_access();
DROP PROCEDURE admit_voa_traveler_zone_access;
