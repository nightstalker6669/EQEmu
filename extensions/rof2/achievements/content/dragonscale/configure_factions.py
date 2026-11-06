#!/usr/bin/env python3
"""Build scoped faction SQL from editable labelled defaults. Does not contact a DB."""
import hashlib,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent
p=json.loads((ROOT/'data/faction-restoration.json').read_text())
assert {g['id']for g in p['groups']}==set(range(3041010,3041016))
assert {r['id']for r in p['bases']}=={1185,1194,1196,1197}
assert all(isinstance(r['after'],int)and -2000<=r['after']<=2000 for r in p['bases'])
assert all(isinstance(h['value'],int)and -1000<=h['value']<=1000 for g in p['groups']for h in g['hits'])
assert all(isinstance(r['id'],int)and 442000<=r['id']<=442900 and r['after']in range(3041010,3041016)for r in p['npc_bindings'])
def q(v):
 if isinstance(v,int):return str(v)
 return 'CONVERT(0x'+v.encode().hex()+' USING utf8mb4)'
def insert(table,rows):
 cols=list(rows[0]);name='_dragonscale_faction_'+table
 return [f'DROP TEMPORARY TABLE IF EXISTS `{name}`;',f'CREATE TEMPORARY TABLE `{name}` LIKE `{table}`;',
  f'INSERT INTO `{name}`('+','.join('`'+c+'`'for c in cols)+') VALUES'+','.join('('+','.join(q(r[c])for c in cols)+')'for r in rows)+';']
groups=[dict(id=g['id'],name=g['name'],primaryfaction=g['primaryfaction'],ignore_primary_assist=0)for g in p['groups']]
hits=[dict(npc_faction_id=g['id'],faction_id=h['faction_id'],value=h['value'],npc_value=0,temp=0)for g in p['groups']for h in g['hits']]
sql=['-- Restore native faction links for the complete Dragonscale batch.',
 '-- All unrecovered numeric values are labelled configurable defaults in data/faction-restoration.json.',
 '-- No character/player standing rows are modified. Whole-package transaction required.',
 'SET SESSION check_constraint_checks=1;']+insert('npc_faction',groups)+insert('npc_faction_entries',hits)
sql+=['DROP TEMPORARY TABLE IF EXISTS `_dragonscale_faction_guard`;',
 'CREATE TEMPORARY TABLE `_dragonscale_faction_guard`(ok INT NOT NULL CHECK(ok=1));']
def guard(condition):sql.append('INSERT INTO `_dragonscale_faction_guard` VALUES(IF(('+condition+'),1,0));')
guard('COALESCE(@dragonscale_whole_batch_reviewed,0)=1')
for table,rows,keys in [('npc_faction',groups,['id']),('npc_faction_entries',hits,['npc_faction_id','faction_id'])]:
 temp='_dragonscale_faction_'+table;cols=list(rows[0]);join=' AND '.join('a.`'+k+'`<=>e.`'+k+'`'for k in keys)
 same=' AND '.join('a.`'+k+'`<=>e.`'+k+'`'for k in cols)
 scope='a.id BETWEEN 3041010 AND 3041015'if table=='npc_faction'else'a.npc_faction_id BETWEEN 3041010 AND 3041015'
 guard(f'NOT EXISTS(SELECT 1 FROM `{table}` a LEFT JOIN `{temp}` e ON {join} WHERE ({scope})AND(e.`{keys[0]}`IS NULL OR NOT({same})))')
for row in p['npc_bindings']:
 guard(f"EXISTS(SELECT 1 FROM npc_types WHERE id={row['id']} AND name={q(row['name'])} AND npc_faction_id IN({row['before']},{row['after']}))")
for row in p['bases']:
 guard(f"EXISTS(SELECT 1 FROM faction_list WHERE id={row['id']} AND name={q(row['name'])} AND base IN({row['before']},{row['after']}))")
for table,rows,keys in [('npc_faction',groups,['id']),('npc_faction_entries',hits,['npc_faction_id','faction_id'])]:
 temp='_dragonscale_faction_'+table;cols=list(rows[0]);join=' AND '.join('a.`'+k+'`<=>e.`'+k+'`'for k in keys)
 sql.append(f'INSERT INTO `{table}`('+','.join('`'+c+'`'for c in cols)+') SELECT '+','.join('e.`'+c+'`'for c in cols)+f' FROM `{temp}` e WHERE NOT EXISTS(SELECT 1 FROM `{table}` a WHERE {join});')
for row in p['npc_bindings']:
 sql.append(f"UPDATE npc_types SET npc_faction_id={row['after']} WHERE id={row['id']};")
for row in p['bases']:
 sql.append(f"UPDATE faction_list SET base={row['after']} WHERE id={row['id']};")
sql.append('SET @dragonscale_faction_ok=1;')
(ROOT/'sql/040-faction-restoration.sql').write_text('\n'.join(sql)+'\n')
manifest=ROOT/'payload-manifest.json'
if manifest.exists():
 rows=json.loads(manifest.read_text());by={r['path']:r for r in rows}
 for name in ['data/faction-restoration.json','sql/040-faction-restoration.sql','configure_factions.py']:
  by[name]=dict(path=name,sha256=hashlib.sha256((ROOT/name).read_bytes()).hexdigest())
 manifest.write_text(json.dumps([by[k]for k in sorted(by)],indent=2)+'\n')
print('Generated guarded faction SQL and refreshed affected payload hashes. No database changed.')
