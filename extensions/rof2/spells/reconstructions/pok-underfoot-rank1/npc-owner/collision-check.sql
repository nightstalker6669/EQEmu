SELECT 'npc_id' AS namespace,id AS occupied_id FROM npc_types WHERE id BETWEEN 202900 AND 202911
UNION ALL SELECT 'npc_merchant_reference',merchant_id FROM npc_types WHERE merchant_id BETWEEN 202900 AND 202911
UNION ALL SELECT 'merchantlist',merchantid FROM merchantlist WHERE merchantid BETWEEN 202900 AND 202911
UNION ALL SELECT 'spawn2_id',id FROM spawn2 WHERE id BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawn2_group_reference',spawngroupID FROM spawn2 WHERE spawngroupID BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawngroup',id FROM spawngroup WHERE id BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawnentry_group_reference',spawngroupID FROM spawnentry WHERE spawngroupID BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawnentry_npc_reference',npcID FROM spawnentry WHERE npcID BETWEEN 202900 AND 202911;
SELECT npcid,slot,itemid FROM merchantlist_temp WHERE npcid BETWEEN 202900 AND 202911;
SELECT 'spawn2_disabled' AS namespace,spawn2_id AS occupied_id FROM spawn2_disabled WHERE spawn2_id BETWEEN 304050000 AND 304050011 UNION ALL SELECT 'respawn_times',id FROM respawn_times WHERE id BETWEEN 304050000 AND 304050011;
