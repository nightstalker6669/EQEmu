-- AA-001: extend the two DoN faction critical-hit rewards to 2H Piercing.
-- Apply inside an owner-controlled transaction after exact preimage checks.
-- Require both rank effect sets unchanged and both slot15 entries absent.
-- Do not use REPLACE, IGNORE, or ON DUPLICATE KEY UPDATE.
INSERT INTO aa_rank_effects (rank_id, slot, effect_id, base1, base2) VALUES
    (1364, 15, 169, 40, 77),
    (1369, 15, 169, 40, 77);
-- Require exactly two inserted rows and full postimage equality before COMMIT.
