"""Prepare reserved PoK source payload only; no DB client/import/execution."""
from pathlib import Path
import argparse,json,copy,hashlib
out=Path(__file__).resolve().parent
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--preimages',type=Path,required=True,help='Retained selected15-provider-preimages.json input')
parser.add_argument('--research',type=Path,required=True,help='NPC owner providers.json input')
args=parser.parse_args()
prepath=args.preimages
researchpath=args.research
pre=json.loads(prepath.read_text());research=json.loads(researchpath.read_text())
providers={p['name']:p for p in research['providers']}
template=next(r for r in pre['actors'] if r['id']==202158)
assert len(pre['missing_selected_names'])==12
payload={'status':'SOURCE ONLY; unregistered/unapplied; stock integration and coordinator authorization required','database_writes':0,'template':{'npc_id':202158,'name':template['name'],'sha256':hashlib.sha256(json.dumps(template,sort_keys=True,separators=(',',':')).encode()).hexdigest(),'basis':'retained full-row normal PoK merchant template; current emulator fields, not original new-provider stats'},'preserve_existing_ids':[202158,202288,202310],'held_rogue':True,'tables':{'npc_types':[],'spawngroup':[],'spawnentry':[],'spawn2':[]},'bindings':[]}
for i,name in enumerate(pre['missing_selected_names']):
 p=providers[name];npc_id=202900+i;spawn_id=304050000+i
 n=copy.deepcopy(template)
 n.update(id=npc_id,name=name.replace(' ','_'),lastname=p['proposed_role'],merchant_id=npc_id,
          race=1,gender=0,texture=0,helmtexture=0,size=6.0,face=0)
 model_note='human1/gender0/texture0/helm0/size6/face0 configurable fallback, exact original model unresolved'
 if name=='Pathfinder Sweeneph':
  n.update(race=156,gender=2)
  model_note='ratman156/gender2/texture0/helm0/size6/face0 tentative undated image-informed candidate; not period identity proof'
 payload['tables']['npc_types'].append(n)
 payload['tables']['spawngroup'].append({'id':spawn_id,'name':f'pok81_{npc_id}','spawn_limit':1,'dist':0,'max_x':0,'min_x':0,'max_y':0,'min_y':0,'delay':45000,'mindelay':15000,'despawn':0,'despawn_timer':100,'wp_spawns':0})
 payload['tables']['spawnentry'].append({'spawngroupID':spawn_id,'npcID':npc_id,'chance':100,'condition_value_filter':1,'min_time':0,'max_time':0,'min_expansion':-1,'max_expansion':-1,'content_flags':None,'content_flags_disabled':None})
 x,y,z=p['candidate_db_xyz']
 payload['tables']['spawn2'].append({'id':spawn_id,'spawngroupID':spawn_id,'zone':'poknowledge','version':0,'x':x,'y':y,'z':z,'heading':0,'respawntime':640,'variance':0,'pathgrid':0,'path_when_zone_idle':0,'_condition':0,'cond_value':1,'animation':0,'min_expansion':18,'max_expansion':-1,'content_flags':None,'content_flags_disabled':None})
 payload['bindings'].append({'name':name,'npc_id':npc_id,'merchant_id':npc_id,'spawngroup_id':spawn_id,'spawn2_id':spawn_id,'native_target_class_id':p['target_class_id'],'stock_expected_rows':len([t for t in p['native_target_references'] if not ((name=='Cavalier Tummings' and t['item_id']==119709) or (name=='Reaver Enold' and t['item_id']==119633))]),'stock_item_scope':[t['item_id'] for t in p['native_target_references'] if not ((name=='Cavalier Tummings' and t['item_id']==119709) or (name=='Reaver Enold' and t['item_id']==119633))],'period_source':p['period_revision_url'],'db_xyz':[x,y,z],'model_default':model_note,'heading_default':0,'placement_accepted_natively':False,'route_min_expansion':18,'era_basis':'March2012/VoA route introduction, not UF spell identity era','other_defaults':'level40/full stats and protection template202158; group defaults/source repository;640srespawn,version0,stationarypath0 local operational defaults, not historical numeric proof'})
payload['stock_scope_status']='final named-stock reconciliation:43new-provider rows; two existing-tome additions separate, originalexisting9unchanged'
payload['stock_only_redirect']={'item_id':119709,'spell_id':22665,'npc_id':202161,'merchant_id':202161,'name':'Ulin Velnik','owner':'Spells','external_npc_key':26168,'basis':'coordinator relays public spell22665SoldBy/NPC26168; no new actor'}
payload['stock_only_redirects']=[payload.pop('stock_only_redirect'),{'item_id':119633,'spell_id':22662,'npc_id':None,'merchant_id':None,'name':'Zhao V`karin','owner':'Spells','external_npc_key':13960,'basis':'final named-stock reconciliation, public spell22662SoldBy/NPC13960; exact native preimage pending from Spells'}]
assert sum(b['stock_expected_rows'] for b in payload['bindings'])==43
(out/'payload.json').write_text(json.dumps(payload,indent=2)+'\n')
collision_sql="""SELECT 'npc_id' AS namespace,id AS occupied_id FROM npc_types WHERE id BETWEEN 202900 AND 202911
UNION ALL SELECT 'npc_merchant_reference',merchant_id FROM npc_types WHERE merchant_id BETWEEN 202900 AND 202911
UNION ALL SELECT 'merchantlist',merchantid FROM merchantlist WHERE merchantid BETWEEN 202900 AND 202911
UNION ALL SELECT 'spawn2_id',id FROM spawn2 WHERE id BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawn2_group_reference',spawngroupID FROM spawn2 WHERE spawngroupID BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawngroup',id FROM spawngroup WHERE id BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawnentry_group_reference',spawngroupID FROM spawnentry WHERE spawngroupID BETWEEN 304050000 AND 304050011
UNION ALL SELECT 'spawnentry_npc_reference',npcID FROM spawnentry WHERE npcID BETWEEN 202900 AND 202911;"""
(out/'collision-check.sql').write_text(collision_sql+'\nSELECT npcid,slot,itemid FROM merchantlist_temp WHERE npcid BETWEEN 202900 AND 202911;\nSELECT \'spawn2_disabled\' AS namespace,spawn2_id AS occupied_id FROM spawn2_disabled WHERE spawn2_id BETWEEN 304050000 AND 304050011 UNION ALL SELECT \'respawn_times\',id FROM respawn_times WHERE id BETWEEN 304050000 AND 304050011;\n')
(out/'collision-receipt.json').write_text(json.dumps({'observed_date':'2026-11-04','target':'peq_housing_test','tool':'read-only EQEmu connector','scope':'only coordinator proposed ranges/namespaces','main_query_result':'Query returned 0 rows.','temporary_merchant_query_result':'Query returned 0 rows.','spawn_state_reference_query_result':'Query returned 0 rows.','occupied':[],'replacement_required':False,'writes':0,'limit':'point-in-time namespace evidence only; future installer must refuse intervening collision'},indent=2)+'\n')
def val(v):
 if v is None:return 'NULL'
 if isinstance(v,str):return "'"+v.replace("'","''")+"'"
 return str(v)
# Guarded source component; deliberately no invocation or migration registration.
sql='''-- SOURCE ONLY / DISABLED: no CALL, no migration registration, no live application.
-- Coordinator reservation: NPC+merchant202900..202911, spawn+group304050000..304050011.
-- Integrate Spells43new-provider merchant rows; two existing-tome additions separately in the SAME reviewed transaction; existing3/9rows separate.
-- New12 spawn2 min_expansion18: March2012/VoA route, not UF spell identity era.
-- Dedicated namespace guards below assume new stock has NOT yet been inserted.
-- Do not execute as an actor-only live install. Never renumber or overwrite existing actors.
DELIMITER $$
CREATE PROCEDURE source_pok81_provider_component()
BEGIN
 DECLARE EXIT HANDLER FOR SQLEXCEPTION BEGIN ROLLBACK; RESIGNAL; END;
 IF COALESCE(@coordinator_authorized_integrated_pok81,0)<>1 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='PoK81 source component not authorized for integrated apply';
 END IF;
 IF (SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE()
     AND table_name IN ('npc_types','spawngroup','spawnentry','spawn2') AND engine='InnoDB')<>4 THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='PoK81 source component requires transactional tables';
 END IF;
 IF EXISTS(SELECT 1 FROM npc_types WHERE id BETWEEN 202900 AND 202911 OR merchant_id BETWEEN 202900 AND 202911)
 OR EXISTS(SELECT 1 FROM merchantlist WHERE merchantid BETWEEN 202900 AND 202911)
 OR EXISTS(SELECT 1 FROM merchantlist_temp WHERE npcid BETWEEN 202900 AND 202911)
 OR EXISTS(SELECT 1 FROM spawngroup WHERE id BETWEEN 304050000 AND 304050011)
 OR EXISTS(SELECT 1 FROM spawn2 WHERE id BETWEEN 304050000 AND 304050011 OR spawngroupID BETWEEN 304050000 AND 304050011)
 OR EXISTS(SELECT 1 FROM spawnentry WHERE spawngroupID BETWEEN 304050000 AND 304050011 OR npcID BETWEEN 202900 AND 202911)
 OR EXISTS(SELECT 1 FROM spawn2_disabled WHERE spawn2_id BETWEEN 304050000 AND 304050011)
 OR EXISTS(SELECT 1 FROM respawn_times WHERE id BETWEEN 304050000 AND 304050011) THEN
  SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='PoK81 exact reserved namespace collision';
 END IF;
 -- Caller owns the integrated transaction and must capture all48full postimages.
 -- This procedure intentionally does not START/COMMIT independently of stock.
'''
for table,rows in payload['tables'].items():
 cols=list(rows[0]);sql+=' INSERT INTO `'+table+'` ('+', '.join('`'+k+'`' for k in cols)+') VALUES\n'
 sql+=',\n'.join(' ('+', '.join(val(r[k]) for k in cols)+')' for r in rows)+';\n'
 sql+=" IF ROW_COUNT()<>12 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='PoK81 "+table+" row count'; END IF;\n"
sql+='END$$\nDELIMITER ;\n-- No CALL or COMMIT here. Integrated installer/publication remains owner-routed.\n'
(out/'providers.sql.disabled').write_text(sql)
# Binding table gives Spells exact destination IDs without editing their stock source.
lines=['# Reserved missing PoK81 providers','', '| Order | Provider | NPC / merchant | Group / spawn | DB x,y,z | Stock rows |','|---|---|---|---|---|---|']
for i,b in enumerate(payload['bindings']):lines.append(f"| {i} | {b['name']} | {b['npc_id']} | {b['spawn2_id']} | "+', '.join(map(str,b['db_xyz']))+f" | {b['stock_expected_rows']} |")
(out/'BINDINGS.md').write_text('\n'.join(lines)+'\n')
(out/'manifest.json').write_text(json.dumps({'source_only':True,'target_database':'peq_housing_test','row_counts':{k:len(v) for k,v in payload['tables'].items()},'stock_rows_owned_here':0,'new_stock_rows_expected_from_Spells':43,'new_stock_count_qualification':'final Spells named inventory reconciliation','existing_tome_additions_expected_separately':2,'existing_stock_only_rows_expected_separately':9,'rogue_stock_held':3,'inputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in [prepath,researchpath]},'outputs_sha256':{n:hashlib.sha256((out/n).read_bytes()).hexdigest() for n in ['payload.json','providers.sql.disabled','BINDINGS.md','collision-check.sql','collision-receipt.json']},'native_validation':False,'writes':0},indent=2)+'\n')
print(json.dumps({'row_counts':{k:len(v) for k,v in payload['tables'].items()},'reserved_order':[(b['npc_id'],b['name']) for b in payload['bindings']],'stock_expected':43,'source_only':True},indent=2))
