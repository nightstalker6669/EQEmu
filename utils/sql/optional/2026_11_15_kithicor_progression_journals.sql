-- Additive CHARACTER database runtime journals; Database owner applies.
-- These are encounter receipts, never player entitlement or task history.
-- Existing Oceangreen journals and player-earned state remain untouched.
CREATE TABLE kithicor_mission_phases (
 shared_task_id BIGINT UNSIGNED NOT NULL,
 phase TINYINT UNSIGNED NOT NULL,
 task_id INT UNSIGNED NOT NULL,
 model_version INT UNSIGNED NOT NULL,
 ready TINYINT UNSIGNED NOT NULL DEFAULT 0,
 dynamic_zone_id INT UNSIGNED NOT NULL DEFAULT 0,
 instance_id INT UNSIGNED NOT NULL DEFAULT 0,
 scene_state TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0 idle,1 active,2 success,3 failed',
 started_at INT UNSIGNED NOT NULL DEFAULT 0,
 seed CHAR(64) CHARACTER SET ascii COLLATE ascii_bin NOT NULL,
 updated_at INT UNSIGNED NOT NULL,
 PRIMARY KEY(shared_task_id,phase)
) ENGINE=InnoDB;

CREATE TABLE kithicor_mission_actions (
 shared_task_id BIGINT UNSIGNED NOT NULL,
 phase TINYINT UNSIGNED NOT NULL,
 action_id SMALLINT UNSIGNED NOT NULL,
 character_id INT UNSIGNED NOT NULL,
 item_id INT UNSIGNED NOT NULL DEFAULT 0,
 status TINYINT UNSIGNED NOT NULL DEFAULT 0,
 updated_at INT UNSIGNED NOT NULL,
 PRIMARY KEY(shared_task_id,phase,action_id)
) ENGINE=InnoDB;

-- Actual Firebrand target receipts, bound to native personal reward occurrence.
-- Private runtime schema only. Never export player rows into a public release.
CREATE TABLE kithicor_personal_burns (
 character_id INT UNSIGNED NOT NULL,
 occurrence_id BIGINT UNSIGNED NOT NULL,
 task_id INT UNSIGNED NOT NULL,
 spawn_id INT UNSIGNED NOT NULL,
 activity_id TINYINT UNSIGNED NOT NULL,
 cast_at INT UNSIGNED NOT NULL,
 PRIMARY KEY(character_id,occurrence_id,spawn_id)
) ENGINE=InnoDB;
