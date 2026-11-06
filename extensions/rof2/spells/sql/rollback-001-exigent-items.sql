-- Rollback only the three rows added by this audit. Verify no characters possess these items before removal.
START TRANSACTION;
DELETE FROM items WHERE id=64981 AND clickeffect=29923 AND source='RoF-audit-20261101';
DELETE FROM items WHERE id=64983 AND clickeffect=29988 AND source='RoF-audit-20261101';
DELETE FROM items WHERE id=64984 AND clickeffect=29989 AND source='RoF-audit-20261101';
COMMIT;
